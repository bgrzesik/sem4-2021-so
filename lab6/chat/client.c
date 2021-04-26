#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "chat.h"

enum client_state client_state = CLIENT_STATE_DISCONNECTING;

int client_id = -1;
struct msg_queue server;
struct msg_queue client;
struct msg_queue recipient;

static void 
dispose(void) 
{
    if (client_id == -1) {
        return;
    }

    struct message msg;
    msg.mtype = MESSAGE_TYPE_STOP;
    msg.sender = client_id;

    mqueue_send(&server, &msg);
    mqueue_close(&server);

    if (client_state == CLIENT_STATE_BUSY) {
        printf("LOL");
        msg.mtype = MESSAGE_TYPE_DISCONNECT;

        mqueue_send(&recipient, &msg);
        mqueue_close(&recipient);
    }

    client_id = -1;
    mqueue_delete(&client);

    exit(0);
}

static void
sigint(int sig)
{
    dispose();
}

static int
handle_msg_state_disconnecting(struct message *msg)
{
    if (msg->mtype == MESSAGE_TYPE_DISCONNECT) {
        client_id = msg->conn.id;
        client_state = CLIENT_STATE_FREE;

        printf("Client id is %d\n", client_id);
        printf("Commands: /list /connect /exit\n");

    } else if (msg->mtype == MESSAGE_TYPE_CONNECT) {
        client_state = CLIENT_STATE_BUSY;
    
    } else if (msg->mtype == MESSAGE_TYPE_STOP) {
        client_state = CLIENT_STATE_DEAD;

    } else {
        fprintf(stderr, "invalid message type %ld\n", msg->mtype);
        return -1;
    }

    return 0;
}

static int
handle_msg_state_free(struct message *msg)
{
    if (msg->mtype == MESSAGE_TYPE_LIST) {
        printf("Client id is %d\n", client_id);

        for (size_t id = 0; id < MAX_CLIENTS; id++) {
            if (msg->list[id] != NO_CLIENT && id != client_id) {
                printf("\t %ld -> %d\n", id, msg->list[id]);
            }
        }
    
    } else if (msg->mtype == MESSAGE_TYPE_STOP) {
        client_state = CLIENT_STATE_DEAD;

    } else if (msg->mtype == MESSAGE_TYPE_CONNECT) {
        printf("Connected\n");
        printf("Commands: /disconnect /exit\n");

        client_state = CLIENT_STATE_BUSY;

        if (mqueue_open(&recipient, msg->conn.pid) != 0) {
            return -1;
        }
    } else {
        fprintf(stderr, "invalid message type %ld\n", msg->mtype);
        return -1;
    }

    return 0;
}

static int
handle_msg_state_busy(struct message *msg)
{
    if (msg->mtype == MESSAGE_TYPE_STOP) {
        client_state = CLIENT_STATE_DEAD;

    } else if (msg->mtype == MESSAGE_TYPE_DISCONNECT) {
        printf("Disconnected\n");

        client_state = CLIENT_STATE_DISCONNECTING;
        mqueue_close(&recipient);
        
        struct message msg;
        msg.mtype = MESSAGE_TYPE_DISCONNECT;
        msg.sender = client_id;
        msg.conn.pid = getpid();

        if (mqueue_send(&server, &msg) != 0) {
            return -1;
        }

    } else if (msg->mtype == MESSAGE_TYPE_TEXT) {
        printf("%d: %s", msg->sender, msg->text);

    } else {
        fprintf(stderr, "invalid message type %ld\n", msg->mtype);
        return -1;
    }

    return 0;
}

static int
handle_input(char *input)
{
    if (input[0] != '/' && client_state == CLIENT_STATE_BUSY) {
        struct message msg;
        msg.mtype = MESSAGE_TYPE_TEXT;
        msg.sender = client_id;
        
        strncpy(msg.text, input, sizeof(msg.text));

        if (mqueue_send(&recipient, &msg) != 0) {
            return -1;
        }

        printf("%d: %s", client_id, input);

        return 0;
    }


    if (strncmp("/exit", input, sizeof("/exit") - 1) == 0) {
        printf("Exited\n");


        if (client_state == CLIENT_STATE_BUSY) {
            struct message msg;
            msg.mtype = MESSAGE_TYPE_DISCONNECT;
            msg.sender = client_id;

            if (mqueue_send(&recipient, &msg) != 0) {
                return -1;
            }
        }

        client_state = CLIENT_STATE_DEAD;

    } else if (client_state == CLIENT_STATE_FREE &&
        strncmp("/list", input, sizeof("/list") - 1) == 0) {

        struct message msg;
        msg.mtype = MESSAGE_TYPE_LIST;
        msg.sender = client_id;

        memset(&msg.list, NO_CLIENT, sizeof(msg.list));

        if (mqueue_send(&server, &msg) != 0) {
            return -1;
        }

    } else if (client_state == CLIENT_STATE_FREE &&
               strncmp("/connect", input, sizeof("/connect") - 1) == 0) {

        struct message msg;
        msg.mtype = MESSAGE_TYPE_CONNECT;
        msg.sender = client_id;

        msg.conn.id = strtol(input + sizeof("/connect") - 1, NULL, 10);

        if (0 > msg.conn.id || msg.conn.id >= MAX_CLIENTS) {
            fprintf(stderr, "error: incorrect id\n");
            return 0;

        } else if (msg.conn.id == client_id) {
            fprintf(stderr, "error: can't connect to self\n");
            return 0;
        }

        if (mqueue_send(&server, &msg) != 0) {
            return -1;
        }

    } else if (client_state == CLIENT_STATE_BUSY &&
               strncmp("/disconnect", input, sizeof("/disconnect") - 1) == 0) {

        struct message msg;
        msg.mtype = MESSAGE_TYPE_DISCONNECT;
        msg.sender = client_id;
        msg.conn.pid = getpid();

        if (mqueue_send(&server, &msg) != 0) {
            return -1;
        }

    } else {
        fprintf(stderr, "error: unexpected %s\n", input);
    }

    return 0;
}


int
main(int argc, const char *argv[])
{
    if (mqueue_open(&client, getpid()) == -1 || mqueue_open(&server, 0) == -1) {
        return -1;
    }

    atexit(dispose);
    signal(SIGINT, sigint);

    int ret = 0;
    struct message msg;
    msg.mtype = MESSAGE_TYPE_DISCONNECT;
    msg.sender = client_id;
    msg.conn.pid = getpid();

    if (mqueue_send(&server, &msg) != 0) {
        return -1;
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0); 
    flags |= O_NONBLOCK; 
    fcntl(STDIN_FILENO, F_SETFL, flags); 

    while (1) {
        if ((ret = mqueue_receive(&client, &msg, 0)) == 0) {
            /* message_debug_print(&msg, stdout); */
            /* fputs("\n", stdout); */

            if (client_state == CLIENT_STATE_DISCONNECTING) {
                handle_msg_state_disconnecting(&msg);

            } else if (client_state == CLIENT_STATE_FREE) {
                handle_msg_state_free(&msg);

            } else if (client_state == CLIENT_STATE_BUSY) {
                handle_msg_state_busy(&msg);

            } 

        } else if (ret == -1) {
            break;
        }

        if (client_state == CLIENT_STATE_DEAD) {
            break;
        }

        char buf[256];
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            handle_input(buf);
        }

        struct timespec req;
        req.tv_sec = 0;
        req.tv_nsec = 1000;
        nanosleep(&req, NULL);
    }

    dispose();

    return 0;
}
