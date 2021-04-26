#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "chat.h"


struct client {
    enum client_state state;
    pid_t pid;
    struct msg_queue queue;
};


struct msg_queue server;
struct client clients[MAX_CLIENTS];

void 
dispose(void) 
{
    mqueue_delete(&server);

    struct message msg;
    msg.mtype = MESSAGE_TYPE_STOP;
    msg.sender = 0;

    for (size_t id = 0; id < MAX_CLIENTS; id++) {
        if (clients[id].state == CLIENT_STATE_DEAD) {
            continue;
        }

        mqueue_send(&clients[id].queue, &msg);
        mqueue_close(&clients[id].queue);
        clients[id].state = CLIENT_STATE_DEAD;
    }

    exit(0);
}

void
sigint(int sig)
{
    dispose();
}

static int
handle_disconnect(struct message *msg)
{
    int sender = msg->sender;
    if (sender == -1) {
        for (size_t id = 0; id < MAX_CLIENTS; id++) {
            if (clients[id].state == CLIENT_STATE_DEAD) {
                sender = id;
                break;
            }
        }

        if (mqueue_open(&clients[sender].queue, msg->conn.pid) != 0) {
            return -1;
        }
    }

    clients[sender].pid = msg->conn.pid;
    clients[sender].state = CLIENT_STATE_FREE;

    struct message res;
    res.mtype = MESSAGE_TYPE_DISCONNECT;
    res.sender = 0;
    res.conn.id = sender;

    if (mqueue_send(&clients[sender].queue, &res) != 0) {
        return -1;
    }

    return 0;
}

static int
handle_stop(struct message *msg)
{
    if (clients[msg->sender].state != CLIENT_STATE_DEAD) {
        clients[msg->sender].pid = NO_CLIENT;
        clients[msg->sender].state = CLIENT_STATE_DEAD;
        return mqueue_close(&clients[msg->sender].queue);
    }

    return 0;
}

static int
handle_list(struct message *msg)
{
    struct message res;
    res.mtype = MESSAGE_TYPE_LIST;
    res.sender = 0;

    for (size_t id = 0; id < MAX_CLIENTS; id++) {
        res.list[id] = clients[id].pid;
    }

    if (mqueue_send(&clients[msg->sender].queue, &res) != 0) {
        return -1;
    }

    return 0;
}

static int
handle_connect(struct message *msg)
{
    if (clients[msg->conn.id].state != CLIENT_STATE_FREE) {
        return 0;
    }


    struct message res;
    res.mtype = MESSAGE_TYPE_CONNECT;
    res.sender = 0;

    res.conn.pid = clients[msg->conn.id].pid;
    if (mqueue_send(&clients[msg->sender].queue, &res) != 0) {
        return -1;
    }

    res.conn.pid = clients[msg->sender].pid;
    if (mqueue_send(&clients[msg->conn.id].queue, &res) != 0) {
        return -1;
    }

    return 0;
}

int
main(int argc, const char *argv[])
{
    for (size_t id = 0; id < MAX_CLIENTS; id++) {
        clients[id].pid = NO_CLIENT;
        clients[id].state = CLIENT_STATE_DEAD;
    }

    if (mqueue_open(&server, 0) == -1) {
        return -1;
    }

    atexit(dispose);
    signal(SIGINT, sigint);

    int ret = 0;
    struct message msg;
    while ((ret = mqueue_receive(&server, &msg, 1)) != -1) {
        message_debug_print(&msg, stdout);
        fputs("\n", stdout);

        switch (msg.mtype) {

            case MESSAGE_TYPE_LIST:
                handle_list(&msg);
                break;
            
            case MESSAGE_TYPE_DISCONNECT:
                handle_disconnect(&msg);
                break;

            case MESSAGE_TYPE_CONNECT:
                handle_connect(&msg);
                break;
                
            case MESSAGE_TYPE_STOP:
                handle_stop(&msg);
                break;

            default:
                fprintf(stderr, "invalid message type %ld\n", msg.mtype);
                break;
        }

    }


    dispose();

    return 0;
}
