
#include "transport.h"

#include <string.h>
#include <errno.h>
#include <time.h>

#if !defined(TRANSPORT_DGRAM) || defined(TRANSPORT_STREAM)
#error "Invalid macro defined"
#endif

#define EPOLL_DATA_INET (-1)
#define EPOLL_DATA_UNIX (-2)

#define EPOLL_DATA_STDIN 0
#define EPOLL_DATA_SERVER 1


#define EPOLL_MAX_EVENTS SERVER_MAX_CLIENTS

#define CLIENT_HEARTRATE_MS 250
#define CLIENT_TIMEOUT_MS 2000
#define FPOLL_TIMEOUT_MS 250

static int
message_send(struct server *server, 
             enum client_type type, 
             struct sockaddr *sockaddr, 
             socklen_t socklen, 
             struct message *message)
{
    int sock;

    if (type == CLIENT_TYPE_INET) {
        sock = server->inet_socket;
    } else if (type == CLIENT_TYPE_UNIX) {
        sock = server->unix_socket;
    } else {
        return -1;
    }

    size_t sent = sendto(sock, message, sizeof(*message), 0, sockaddr, socklen);
    err_chk(sent == sizeof(*message));

    return 0;
}

static inline uint64_t 
get_time_ms(void)
{
    struct timespec timespec;
    clock_gettime(CLOCK_REALTIME, &timespec);

    uint64_t t = timespec.tv_sec * 1000000000 + timespec.tv_nsec;
    return t / 1000000;
}

int
server_init(struct server *server, int inet_port, const char *unix_path)
{
    /* inet */
    npos_chk(server->inet_socket = socket(AF_INET, SOCK_DGRAM, 0));

    int reuse_addr = 1;
    non0_chk(setsockopt(server->inet_socket, SOL_SOCKET, SO_REUSEADDR,
                        &reuse_addr, sizeof(reuse_addr)));

    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_in.sin_port = htons(inet_port);
    non0_chk(bind(server->inet_socket, (struct sockaddr *) &addr_in, sizeof(addr_in)));

    /* unix */
    npos_chk(server->unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0));

    if (access(unix_path, F_OK) == 0) {
        non0_chk(unlink(unix_path));
    }

    struct sockaddr_un addr_un;
    memset(&addr_un, 0, sizeof(addr_in));
    addr_un.sun_family = AF_UNIX;
    strncpy(addr_un.sun_path, unix_path, sizeof(addr_un.sun_path));
    non0_chk(bind(server->unix_socket, (struct sockaddr *) &addr_un, sizeof(addr_un)));

    /* epoll */
    npos_chk(server->epoll = epoll_create1(0));

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    event.data.u32 = EPOLL_DATA_INET;
    non0_chk(epoll_ctl(server->epoll, EPOLL_CTL_ADD, server->inet_socket, &event));

    event.data.u32 = EPOLL_DATA_UNIX;
    non0_chk(epoll_ctl(server->epoll, EPOLL_CTL_ADD, server->unix_socket, &event));

    /* game logic */
    for (size_t i = 0; i < SERVER_MAX_CLIENTS; ++i) {
        server->clients[i].id = i;
        server->clients[i].type = CLIENT_TYPE_NONE;
    }

    return 0;
}


static int
server_new_client(struct server *server, 
                  enum client_type type,
                  struct sockaddr *sockaddr,
                  socklen_t socklen,
                  struct message *message, 
                  server_event_handler_t event_handler)
{
    err_chk(message->sender == MESSAGE_IDENTITY_UNKNOWN);

    int16_t client_id;
    for (client_id = 0; client_id < SERVER_MAX_CLIENTS; client_id++) {
        if (server->clients[client_id].type == CLIENT_TYPE_NONE) {
            break;
        }
    }
    
    if (client_id >= SERVER_MAX_CLIENTS) {
        struct message message;
        message.sender = MESSAGE_IDENTITY_SERVER;
        message.recipient = MESSAGE_IDENTITY_UNKNOWN;
        message.type = MESSAGE_TYPE_DROP;
        message.drop.cause = MESSAGE_DROP_CAUSE_SERVER_LIMIT;

        non0_chk(message_send(server, type, sockaddr, socklen, &message));

        return -1;
    }

    printf("[%hd] new connection (%s)\n",
           client_id, type == CLIENT_TYPE_INET ? "inet" : "unix");
     

    struct connected_client *client = &server->clients[client_id];
    client->id = client_id;
    client->type = type;
    client->alive = 1;

    void *dest = NULL;
    if (type == CLIENT_TYPE_INET) {
        dest = &client->addr_in;
    } else if (type == CLIENT_TYPE_UNIX) {
        dest = &client->addr_un;
    }

    memcpy(dest, sockaddr, socklen);

    struct server_event server_event;
    server_event.type = SERVER_EVENT_TYPE_CLIENT_CONNECTED;
    server_event.client = client;

    non0_chk(event_handler(server, &server_event));

    message->sender = client_id;

    return 0;
}

static int
server_handle_incoming(struct server *server, 
                       enum client_type type,
                       server_event_handler_t event_handler)
{
    struct message message;
    struct sockaddr_in addr_in;
    struct sockaddr_un addr_un;

    int sock;
    struct sockaddr *addr;
    socklen_t addr_len;

    if (type == CLIENT_TYPE_INET) {
        sock = server->inet_socket;
        addr = (struct sockaddr *) &addr_in;
        addr_len = sizeof(addr_in);

    } else if (type == CLIENT_TYPE_UNIX) {
        sock = server->unix_socket;
        addr = (struct sockaddr *) &addr_un;
        addr_len = sizeof(addr_un);

    } else {
        return -1;
    }

    err_chk(recvfrom(sock, &message, sizeof(message), 0, addr, &addr_len) == sizeof(message));

    if (message.type == MESSAGE_TYPE_HANDSHAKE) {
        non0_chk(server_new_client(server, type, addr, addr_len, &message, event_handler));
    }

    err_chk(0 <= message.sender && message.sender < SERVER_MAX_CLIENTS);
    struct connected_client *client = &server->clients[message.sender];

    if (type == CLIENT_TYPE_INET) {
        non0_chk(memcmp(&addr_in, &client->addr_in, sizeof(addr_in)));
    } else if (type == CLIENT_TYPE_INET) {
        non0_chk(memcmp(&addr_un, &client->addr_un, sizeof(addr_un)));
    }

    client->last_message = get_time_ms();

    struct server_event server_event;
    server_event.type = SERVER_EVENT_TYPE_CLIENT_MESSAGE;
    server_event.client = client;
    memcpy(&server_event.message, &message, sizeof(message));

    if (message.type == MESSAGE_TYPE_HEARTBEAT) {
        struct message message;
        message.type = MESSAGE_TYPE_HEARTBEAT;
        non0_chk(connected_client_send(server, client->id, &message));
        return 0;
    }

    non0_chk(event_handler(server, &server_event));

    if (message.type == MESSAGE_TYPE_DROP) {
        non0_chk(connected_client_drop(server, client->id, 
                                       MESSAGE_DROP_CAUSE_TIMEOUT));
    }

    return 0;
}

int
server_poll_events(struct server *server, server_event_handler_t event_handler)
{
    int ret = -1;
    struct epoll_event epoll_events[EPOLL_MAX_EVENTS];

    npos_chk(ret = epoll_wait(server->epoll, epoll_events, EPOLL_MAX_EVENTS, FPOLL_TIMEOUT_MS));
    if (ret > EPOLL_MAX_EVENTS) {
        fprintf(stderr, "error: invalid number of events returned from epoll_wait\n");
        return -1;
    }

    for (int i = 0; i < ret; ++i) {
        struct epoll_event *event = &epoll_events[i];
        ret = -1;

        if (event->data.u32 == EPOLL_DATA_INET && (event->events & EPOLLIN) == EPOLLIN) {
            ret = server_handle_incoming(server, CLIENT_TYPE_INET, 
                                         event_handler);

        } else if (event->data.u32 == EPOLL_DATA_UNIX && (event->events & EPOLLIN) == EPOLLIN) {
            ret = server_handle_incoming(server, CLIENT_TYPE_UNIX, 
                                         event_handler);
        } 

        if (ret != 0) {
            fprintf(stderr, "warn: error while reading message\n");
        }
    }

    uint64_t now_ms = get_time_ms();
    for (uint16_t client_id = 0; client_id < SERVER_MAX_CLIENTS; client_id++) {
        if (server->clients[client_id].type == CLIENT_TYPE_NONE) {
            continue;
        }


        uint64_t time = now_ms - server->clients[client_id].last_message;
        if (time > CLIENT_TIMEOUT_MS) {
            printf("[%hd] timeout %lu\n", client_id, time);
            non0_chk(connected_client_drop(server, client_id, 
                                           MESSAGE_DROP_CAUSE_TIMEOUT));

            struct server_event server_event;
            server_event.type = SERVER_EVENT_TYPE_CLIENT_DROP;
            server_event.client = &server->clients[client_id];
            non0_chk(event_handler(server, &server_event));
        }
    }

    return 0;
}

int
server_destroy(struct server *server)
{
    if (server->inet_socket != -1) {
        shutdown(server->inet_socket, SHUT_RDWR);
        close(server->inet_socket);
        server->inet_socket = -1;
    }

    if (server->unix_socket != -1) {
        unlink(server->unix_path);
        shutdown(server->unix_socket, SHUT_RDWR);
        close(server->unix_socket);
        server->unix_socket = -1;
    }

    if (server->epoll != -1) {
        close(server->epoll);
        server->epoll = -1;
    }

    return 0;
}

int
connected_client_send(struct server *server,
                      int16_t client_id,
                      struct message *message)
{
    struct connected_client *client = &server->clients[client_id];

    if (client->type == CLIENT_TYPE_INET) {
        non0_chk(message_send(server, CLIENT_TYPE_INET,
                              (struct sockaddr *) &client->addr_in, 
                              sizeof(client->addr_in), message));

    } else if (client->type == CLIENT_TYPE_UNIX) {
        non0_chk(message_send(server, CLIENT_TYPE_UNIX,
                              (struct sockaddr *) &client->addr_un, 
                              sizeof(client->addr_un), message));

    } else {
        return -1;
    }

    return 0;
}

int 
connected_client_drop(struct server *server,
                      int16_t client_id,
                      enum message_drop_cause cause)
{
    struct connected_client *client = &server->clients[client_id];

    if (client->alive) {
        struct message message;
        message.type = MESSAGE_TYPE_DROP;
        message.drop.cause = cause;
        connected_client_send(server, client_id, &message);
    }

    client->type = CLIENT_TYPE_NONE;

    printf("[%hd] client disconnected\n", client_id);

    return 0;
}

static int
client_init_common(struct client *client)
{
    npos_chk(client->epoll = epoll_create1(0));

    struct epoll_event event;

    event.data.u32 = EPOLL_DATA_STDIN;
    event.events = EPOLLIN;
    non0_chk(epoll_ctl(client->epoll, EPOLL_CTL_ADD, STDIN_FILENO, &event));

    event.data.u32 = EPOLL_DATA_SERVER;
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    non0_chk(epoll_ctl(client->epoll, EPOLL_CTL_ADD, client->server, &event));

    client->last_message = get_time_ms();
    client->alive = 1;

    return 0;
}

int
client_connect_inet(struct client *client, const char *hostname, int port)
{
    client->alive = 0;
    client->client_id = MESSAGE_IDENTITY_UNKNOWN;
    client->type = CLIENT_TYPE_INET;
    npos_chk(client->server = socket(AF_INET, SOCK_DGRAM, 0));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    err_chk(inet_aton(hostname, &addr.sin_addr) != 0);

    non0_chk(connect(client->server,
                     (const struct sockaddr *) &addr, sizeof(addr)));

    non0_chk(client_init_common(client));

    return 0;
}

int
client_connect_unix(struct client *client, 
                    const char *path, 
                    const char *client_path)
{
    client->alive = 0;
    client->client_id = MESSAGE_IDENTITY_UNKNOWN;
    client->type = CLIENT_TYPE_UNIX;
    npos_chk(client->server = socket(AF_UNIX, SOCK_DGRAM, 0));

    struct sockaddr_un client_addr;
    client_addr.sun_family = AF_UNIX;

    strncpy(client_addr.sun_path, client_path,
            sizeof(client_addr.sun_path));

    non0_chk(bind(client->server, 
                  (const struct sockaddr *) &client_addr, sizeof(client_addr)));

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, path,
            sizeof(addr.sun_path));

    non0_chk(connect(client->server,
                     (const struct sockaddr *) &addr, sizeof(addr)));

    non0_chk(client_init_common(client));
    return 0;
}

int
client_send(struct client *client, struct message *message)
{
    message->sender = client->client_id;
    message->recipient = MESSAGE_IDENTITY_SERVER;

    err_chk(send(client->server, message, sizeof(*message), 0) == sizeof(*message));

    return 0;
}

static int 
client_handle_message(struct client *client, client_event_handler_t event_handler)
{
    struct client_event client_event;
    client_event.type = CLIENT_EVENT_TYPE_SERVER_MESSAGE;

    err_chk(recv(client->server, &client_event.message, 
                 sizeof(client_event.message), 0) == sizeof(client_event.message));

    struct message *message = &client_event.message;
    
    client->last_message = get_time_ms();

    if (message->type == MESSAGE_TYPE_HEARTBEAT) {
        return 0;
    }

    non0_chk(event_handler(client, &client_event));

    if (message->type == MESSAGE_TYPE_DROP) {
        client_event.type = CLIENT_EVENT_TYPE_SERVER_DROP;
        non0_chk(event_handler(client, &client_event));
    }

    return 0;
}

int
client_handle_events(struct client *client, client_event_handler_t event_handler)
{
    struct epoll_event epoll_events[2];
    int ret;

    npos_chk(ret = epoll_wait(client->epoll, epoll_events, 2, FPOLL_TIMEOUT_MS));

    for (size_t i = 0; i < ret; ++i) {
        struct epoll_event *event = &epoll_events[i];

        if (event->data.u32 == EPOLL_DATA_STDIN) {
            struct client_event client_event;
            client_event.type = CLIENT_EVENT_TYPE_STDIN_INPUT;

            char *line = client_event.input.text;
            size_t bytes = sizeof(client_event.input.text);
            npos_chk(bytes = getline(&line, &bytes, stdin));
            client_event.input.read = bytes;

            non0_chk(event_handler(client, &client_event));
            continue;
        }

        err_chk(event->data.u32 == EPOLL_DATA_SERVER);

        if ((event->events & EPOLLIN) == EPOLLIN) {
            event->events &= ~EPOLLIN;

            non0_chk(client_handle_message(client, event_handler));
        } 
    } 

    uint64_t now_ms = get_time_ms();
    uint64_t last = now_ms - client->last_message;
    if (last > CLIENT_HEARTRATE_MS) {
        struct message message;
        message.type = MESSAGE_TYPE_HEARTBEAT;
        non0_chk(client_send(client, &message));
    }

    return 0;
}

int
client_destroy(struct client *client)
{
    struct sockaddr_un addr;
    if (client->type == CLIENT_TYPE_UNIX) {
        socklen_t socklen = sizeof(addr);
        non0_chk(getsockname(client->server, (struct sockaddr *) &addr, &socklen));
    }

    shutdown(client->server, SHUT_RDWR);
    close(client->server);

    if (client->type == CLIENT_TYPE_UNIX) {
        non0_chk(unlink(addr.sun_path));
    }

    return 0;
}
