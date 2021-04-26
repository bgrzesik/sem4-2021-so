#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>


#ifndef __CHAT_H__
#define __CHAT_H__

#define MAX_TEXT_LEN 128
#define MAX_CLIENTS 16
#define NO_CLIENT  -1


#if !(defined(USE_SYSV_QUEUE) || defined(USE_POSIX_QUEUE))
#error "Either USE_SYSV_QUEUE or USE_POSIX_QUEUE has to be defined"
#endif


enum client_state {
    CLIENT_STATE_DISCONNECTING,
    CLIENT_STATE_FREE,
    CLIENT_STATE_BUSY,
    CLIENT_STATE_DEAD,
};

enum message_type {
    MESSAGE_TYPE_TEXT           = 0x05,
    MESSAGE_TYPE_CONNECT        = 0x04,
    MESSAGE_TYPE_LIST           = 0x03,
    MESSAGE_TYPE_DISCONNECT     = 0x02,
    MESSAGE_TYPE_STOP           = 0x01,
};

struct message {
    /* enum message_type */
    long mtype;
    int sender;

    union {
        /* mtype = MESSAGE_LIST */
        pid_t list[MAX_CLIENTS];
        
        /* mtype = MESSAGE_CONNECT | MESSAGE_DISCONNECT */
        struct {
            union {
                /* DISCONNECT: server -> client */
                /*    CONNECT: client -> server */
                int id;

                /* DISCONNECT: client -> server */
                /*    CONNECT: server -> client */
                pid_t pid;
            };
        } conn;

        /* mtype = MESSAGE_TEXT */
        char text[MAX_TEXT_LEN];
    };
};


void
message_debug_print(struct message *msg, FILE *f);


struct msg_queue {
    pid_t id;

#if defined(USE_SYSV_QUEUE)
    int msqid;
#elif defined(USE_POSIX_QUEUE)
    mqd_t mqdes;
#endif

};

int mqueue_open(struct msg_queue *queue, pid_t id);

int mqueue_receive(struct msg_queue *queue, struct message *msg, int block);

int mqueue_send(struct msg_queue *queue, struct message *msg);

int mqueue_close(struct msg_queue *queue);

int mqueue_delete(struct msg_queue *queue);


#endif
