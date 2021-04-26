#include "chat.h"
 
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static inline void
write_dec(char *dst, size_t pp)
{
    static const char map[] = "0123456789";

    size_t iter = ((size_t) log10(pp)) + 1;
    if (pp == 0) {
        iter = 1;
    }

    char *cur = &dst[iter];
    *cur = 0;

    for (size_t i = 0; i < iter; i++) {
        cur--;
        *cur = map[pp % 10];
        pp /= 10;
    }
}

void
message_debug_print(struct message *msg, FILE *f)
{
    fprintf(f, "message { ");
    
    switch(msg->mtype) {
        case MESSAGE_TYPE_TEXT:

            fprintf(f, "MESSAGE_TYPE_TEXT");
            break;

        case MESSAGE_TYPE_CONNECT:
            fprintf(f, "MESSAGE_TYPE_CONNECT, id = %d, pid = %d", 
                       msg->conn.id, msg->conn.pid);
            break;

        case MESSAGE_TYPE_LIST:
            fprintf(f, "MESSAGE_TYPE_LIST");

            for (size_t id = 0; id < MAX_CLIENTS; id++) {
                if (msg->list[id] != NO_CLIENT) {
                    fprintf(f, "%zu: %d ", id, msg->list[id]);
                }
            }

            break;

        case MESSAGE_TYPE_DISCONNECT:
            fprintf(f, "MESSAGE_TYPE_DISCONNECT, client_pid = %d, id = %d", 
                       msg->conn.pid, msg->conn.id);
            break;

        case MESSAGE_TYPE_STOP:
            fprintf(f, "MESSAGE_TYPE_STOP");
            break;
    }

    fprintf(f, " }");
}


int mqueue_open(struct msg_queue *queue, pid_t id)
{
    queue->id = id;
#if defined(USE_SYSV_QUEUE)
#define QUEUE_DIR "./chatqs/"

    if (access(QUEUE_DIR, R_OK | W_OK) != 0) {
        if (mkdir(QUEUE_DIR, 0777) != 0) {
            perror("mkdir");
            return -1;
        }
    }


    char buf[255] = QUEUE_DIR;
    char *name = buf;

    if (id != 0) {
        write_dec(buf + sizeof(QUEUE_DIR) - 1, id);
    } else {
        name = "./chatqs/server";
    }

    if (access(name, R_OK | W_OK) != 0) {
        int fd;

        if ((fd = creat(name, 0666)) == -1) {
            perror("creat");
            return -1;
        }

        close(fd);
    }


    queue->msqid = msgget(ftok(name, 'a'), IPC_CREAT | 0666);
    
    /* printf("pid = %d, msqid = %d\n", id, queue->msqid); */

    if (queue->msqid == -1) {
        perror("msgget");
        return -1;
    }
 
#elif defined(USE_POSIX_QUEUE)
#define QUEUE_DIR "/chatqs_"

    char buf[255] = QUEUE_DIR;
    char *name = buf;

    if (id != 0) {
        write_dec(buf + sizeof(QUEUE_DIR) - 1, id);
    } else {
        name = "/chatqs_server";
    }

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(struct message);

    queue->mqdes = mq_open(name, O_RDWR | O_CREAT, 0666, &attr);
    if (queue->mqdes == -1) {
        perror("mq_open");
        return -1;
    }

#endif
    return 0;
}

int mqueue_receive(struct msg_queue *queue, struct message *msg, int block)
{
#if defined(USE_SYSV_QUEUE)
    int flags = 0;
    if (!block) {
        flags |= IPC_NOWAIT;
    }

    if (msgrcv(queue->msqid, msg, 
               sizeof(struct message) - sizeof(int), 0, flags) == -1) {

        if (errno == ENOMSG) {
            errno = 0;
            return 1;
        }

        perror("msgrcv");
        return -1;
    }

#elif defined(USE_POSIX_QUEUE)
    struct mq_attr attr;
    if (mq_getattr(queue->mqdes, &attr) == -1) {
        perror("mq_getattr");
        return -1;
    }

    if (!block) {
        attr.mq_flags = O_NONBLOCK;
    } else {
        attr.mq_flags = 0;
    }
    
    if (mq_setattr(queue->mqdes, &attr, NULL) == -1) {
        perror("mq_setattr");
        return -1;
    }

    if (mq_receive(queue->mqdes, 
                   (char *) msg, sizeof(struct message), NULL) == -1) {
        if (errno == EAGAIN) {
            return 1;
        }

        perror("mq_receive");
        return -1;
    } 
#endif
    return 0;
}

int mqueue_send(struct msg_queue *queue, struct message *msg)
{
#if defined(USE_SYSV_QUEUE)
    if (msgsnd(queue->msqid, msg, 
               sizeof(struct message) - sizeof(int), 0) == -1) {

        perror("msgsnd");
        return -1;
    }

#elif defined(USE_POSIX_QUEUE)
    if (mq_send(queue->mqdes, 
                (char *) msg, sizeof(struct message), msg->mtype) == -1) {
        perror("mq_send");
        return -1;
    }
    
#endif

    return 0;
}

int mqueue_close(struct msg_queue *queue)
{
#if defined(USE_SYSV_QUEUE)
    /* no op */
#elif defined(USE_POSIX_QUEUE)
    mq_close(queue->mqdes);
#endif
    return 0;
}

int mqueue_delete(struct msg_queue *queue)
{
#if defined(USE_SYSV_QUEUE)
    if (queue->msqid == -1) {
        return 0;
    }

    if (msgctl(queue->msqid, IPC_RMID, NULL) != 0) {
        perror("msgctl");
        return -1;
    }

    queue->msqid = -1;

    char buf[255] = QUEUE_DIR;
    char *name = buf;

    if (queue->id != 0) {
        write_dec(buf + sizeof(QUEUE_DIR) - 1, queue->id);
    } else {
        name = "./chatqs/server";
    }

    if (remove(name) != 0) {
        perror("remove");
    }

#elif defined(USE_POSIX_QUEUE)
#define QUEUE_DIR "/chatqs_"

    char buf[255] = QUEUE_DIR;
    char *name = buf;

    if (queue->id != 0) {
        write_dec(buf + sizeof(QUEUE_DIR) - 1, queue->id);
    } else {
        name = "/chatqs_server";
    }

    if (mq_unlink(name) == -1) {
        perror("mq_unlink");
    }
#endif
    return 0;
}

   
