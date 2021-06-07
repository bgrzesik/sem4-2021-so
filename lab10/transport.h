#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__


#if !(defined(TRANSPORT_DGRAM) ^ defined(TRANSPORT_STREAM))
#error "Either TRANSPORT_DATAGRAM or TRANSPORT_STREAM must be defined"
#endif

#define MESSAGE_PLAYER_NAME_LEN 30

enum message_identity {
    MESSAGE_IDENTITY_SERVER = -1,
    MESSAGE_IDENTITY_UNKNOWN = -2,
};

enum message_type {
    MESSAGE_TYPE_HANDSHAKE,
    MESSAGE_TYPE_GAME_START,
    MESSAGE_TYPE_PLAYER_MOVE,
    MESSAGE_TYPE_DROP,
};

enum tick_tac {
    TICK_TAC_EMPTY,
    TICK_TAC_X,
    TICK_TAC_O,
};

enum message_drop_cause {
    MESSAGE_DROP_CAUSE_NAME_CONFLICT,
    MESSAGE_DROP_CAUSE_SERVER_LIMIT,
    MESSAGE_DROP_CAUSE_GAME_END,
    MESSAGE_DROP_CAUSE_SECOND_PLAYER,
    MESSAGE_DROP_CAUSE_TIMEOUT,
    MESSAGE_DROP_CAUSE_UNKNOWN,
};

#define TICK_TACK_TOE_PORT 1238

struct message {
    uint32_t type;

    /* i wonder what could go wrong here... */
    int16_t sender; /* unused */
    int16_t recipient;

    union {
        struct {
            uint16_t client_id;
            char     name[MESSAGE_PLAYER_NAME_LEN];
        } handshake;

        struct {
            uint8_t  tick_tac;
            char     opponent[MESSAGE_PLAYER_NAME_LEN];
        } game_start;

        struct {
            uint16_t player;
            uint8_t  field;
        } player_move;

        struct {
            uint32_t cause;
        } drop;
    };
} __attribute__((packed));

#define err_chk(op)          \
    do {                     \
        if (!(op)) {         \
            perror(#op);     \
            return -1;       \
        }                    \
    } while (0)

#define non0_chk(op) err_chk((op) == 0)
#define npos_chk(op) err_chk((op) >= 0)

#define SERVER_MAX_CLIENTS 32
#define SERVER_TIMEOUT_TIME 1000

enum connected_client_type {
    CONNECTED_CLIENT_TYPE_NONE, /* not connected */
    CONNECTED_CLIENT_TYPE_UNIX, /* unix domain socket */
    CONNECTED_CLIENT_TYPE_INET, /* tcp/udp */
};

struct connected_client {
    int16_t id;
    int type;
    int alive;

    long last_message;

    union {
        int socket;
        const char *path;
        struct sockaddr_in addr_in;
        struct sockaddr_un addr_un;
    };
};

struct server {
    int inet_socket;

    char unix_path[sizeof(((struct sockaddr_un *) NULL)->sun_path)];
    int unix_socket;

    int epoll;

    struct connected_client clients[SERVER_MAX_CLIENTS];
};

enum server_event_type {
    SERVER_EVENT_TYPE_CLIENT_CONNECTED,
    SERVER_EVENT_TYPE_CLIENT_MESSAGE,
    SERVER_EVENT_TYPE_CLIENT_DROP,
};

struct server_event {
    enum server_event_type type;
    struct connected_client *client;

    union {
        struct message message;
    };
};

typedef int (*server_event_handler_t)(struct server *server,
                                      struct server_event *event);

int
server_init(struct server *server, int inet_port, const char *unix_path);

int
server_poll_events(struct server *server, server_event_handler_t event_handler);

int
server_destroy(struct server *server);

int
connected_client_send(struct server *server,
                      int16_t client_id,
                      struct message *message);

int 
connected_client_drop(struct server *server,
                      int16_t client_id,
                      enum message_drop_cause cause);

struct client {
    int server;
    int epoll;

    int16_t client_id;
    int alive;
};

enum client_event_type {
    CLIENT_EVENT_TYPE_SERVER_MESSAGE,
    CLIENT_EVENT_TYPE_SERVER_DROP,
    CLIENT_EVENT_TYPE_STDIN_INPUT,
};

struct client_event {
    enum client_event_type type;
    union {
        struct message message;

        struct {
            char text[MESSAGE_PLAYER_NAME_LEN];
            size_t read;
        } input;
    };
};

typedef int (*client_event_handler_t)(struct client *client,
                                      struct client_event *event);

int
client_connect_inet(struct client *client, const char *hostname, int port);

int
client_connect_unix(struct client *client, const char *path);

int
client_send(struct client *client, struct message *message);

int
client_handle_events(struct client *client, client_event_handler_t event_handler);

int
client_destroy(struct client *client);

#endif
