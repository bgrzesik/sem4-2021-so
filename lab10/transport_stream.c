#include "transport.h"


#if !defined(TRANSPORT_STREAM) || defined(TRANSPORT_DGRAM)
#error "Invalid macro defined"
#endif

#define PACKET_MAGIC0  0x01fedcba
#define PACKET_MAGIC1  0x98761234

typedef uint32_t packet_magic_t;

struct packet {
    packet_magic_t magic0; /* = PACKET_MAGIC0 */
    struct message message;
    packet_magic_t magic1; /* = PACKET_MAGIC1 */
};

#define EPOLL_DATA_INET (-1)
#define EPOLL_DATA_UNIX (-2)

#define EPOLL_DATA_STDIN 0
#define EPOLL_DATA_SERVER 1

#define EPOLL_MAX_EVENTS 32

static int
packet_read_magic(int socket, int expected)
{
    packet_magic_t  magic = 0;
    size_t read = 0;
    size_t s;

    do {
        npos_chk(s = recv(socket, &magic + read, sizeof(magic) - read, 0));
        read += s;
    } while (s > 0 && read < sizeof(magic));

    return magic == expected ? 0 : -1;
}

static int
message_read(int socket, struct message *message)
{
    non0_chk(packet_read_magic(socket, PACKET_MAGIC0));

    size_t read = 0;
    size_t s;

    do {
        npos_chk(s = recv(socket, message + read, sizeof(struct message) - read, 0));
        read += s;
    } while (s > 0 && read < sizeof(*message));

    non0_chk(packet_read_magic(socket, PACKET_MAGIC1));

    return 0;
}

static int
message_send(int socket, struct message *message)
{
    struct packet packet;
    packet.magic0 = PACKET_MAGIC0;
    packet.magic1 = PACKET_MAGIC1;
    memcpy(&packet.message, message, sizeof(struct message));

    size_t sent = 0;
    size_t s;

    do {
        npos_chk(s = send(socket, &packet + sent, sizeof(struct packet) - sent, 0));
        sent += s;
    } while (s > 0 && sent < sizeof(struct packet));

    return 0;
}

int
server_init(struct server *server, int inet_port, const char *unix_path)
{
    /* inet */
    npos_chk(server->inet_socket = socket(AF_INET, SOCK_STREAM, 0));

    int reuse_addr = 1;
    non0_chk(setsockopt(server->inet_socket, SOL_SOCKET, SO_REUSEADDR,
                        &reuse_addr, sizeof(reuse_addr)));

    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_in.sin_port = htons(inet_port);
    non0_chk(bind(server->inet_socket, (struct sockaddr *) &addr_in, sizeof(addr_in)));
    non0_chk(listen(server->inet_socket, SERVER_MAX_CLIENTS));

    /* unix */
    npos_chk(server->unix_socket = socket(AF_UNIX, SOCK_STREAM, 0));

    if (access(unix_path, F_OK) == 0) {
        non0_chk(unlink(unix_path));
    }

    struct sockaddr_un addr_un;
    addr_un.sun_family = AF_UNIX;
    strncpy(addr_un.sun_path, unix_path, sizeof(addr_un.sun_path));
    non0_chk(bind(server->unix_socket, (struct sockaddr *) &addr_un, sizeof(addr_un)));
    non0_chk(listen(server->unix_socket, SERVER_MAX_CLIENTS));

    /* epoll */
    npos_chk(server->epoll = epoll_create1(0));

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    event.data.u32 = EPOLL_DATA_INET;
    non0_chk(epoll_ctl(server->epoll, EPOLL_CTL_ADD, server->inet_socket, &event));

    event.data.u32 = EPOLL_DATA_UNIX;
    non0_chk(epoll_ctl(server->epoll, EPOLL_CTL_ADD, server->unix_socket, &event));

    /* game logic */
    for (size_t i = 0; i < SERVER_MAX_CLIENTS; ++i) {
        server->clients[i].id = i;
        server->clients[i].type = CONNECTED_CLIENT_TYPE_NONE;
    }

    return 0;
}

static int
server_handle_new_connection(struct server *server,
                             enum connected_client_type type,
                             server_event_handler_t event_handler)
{

    int server_socket;
    if (type == CONNECTED_CLIENT_TYPE_INET) {
        server_socket = server->inet_socket;
    } else {
        server_socket = server->unix_socket;
    }

    int socket;
    npos_chk(socket = accept(server_socket, NULL, NULL));

    int16_t client_id;
    for (client_id = 0; client_id < SERVER_MAX_CLIENTS; ++client_id) {
        if (server->clients[client_id].type == CONNECTED_CLIENT_TYPE_NONE) {
            break;
        }
    }

    if (client_id >= SERVER_MAX_CLIENTS) {
        struct message message;
        message.sender = MESSAGE_IDENTITY_SERVER;
        message.recipient = MESSAGE_IDENTITY_UNKNOWN;
        message.type = MESSAGE_TYPE_DROP;
        message.drop.cause = MESSAGE_DROP_CAUSE_SERVER_LIMIT;

        non0_chk(message_send(socket, &message));

        close(socket);
        return -1;
    }

    printf("[%hd] new connection (%s)\n",
           client_id, type == CONNECTED_CLIENT_TYPE_INET ? "inet" : "unix");

    struct connected_client *client = &server->clients[client_id];
    client->id = client_id;
    client->type = type;
    client->socket = socket;
    client->alive = 1;

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    event.data.u32 = client_id;

    non0_chk(epoll_ctl(server->epoll, EPOLL_CTL_ADD, client->socket, &event));

    struct server_event server_event;
    server_event.type = SERVER_EVENT_TYPE_CLIENT_CONNECTED;
    server_event.client = client;

    non0_chk(event_handler(server, &server_event));

    return 0;
}

static int
server_handle_incoming_message(struct server *server,
                               int16_t client_id,
                               server_event_handler_t event_handler)
{
    struct connected_client *client = &server->clients[client_id];

    printf("[%hd] incoming message\n", client_id);

    struct server_event server_event;
    server_event.type = SERVER_EVENT_TYPE_CLIENT_MESSAGE;
    server_event.client = client;

    non0_chk(message_read(client->socket, &server_event.message));
    non0_chk(event_handler(server, &server_event));

    return 0;
}

static int
server_handle_disconnect(struct server *server,
                         int16_t client_id,
                         server_event_handler_t event_handler)
{
    struct connected_client *client = &server->clients[client_id];

    err_chk(client->type != CONNECTED_CLIENT_TYPE_NONE);
    client->alive = 0;

    struct server_event server_event;
    server_event.type = SERVER_EVENT_TYPE_CLIENT_DROP;
    server_event.client = client;

    non0_chk(event_handler(server, &server_event));

    non0_chk(connected_client_drop(server, client_id, MESSAGE_DROP_CAUSE_UNKNOWN));

    return 0;
}


int
server_poll_events(struct server *server,
                   server_event_handler_t event_handler)
{
    int ret;
    struct epoll_event epoll_events[EPOLL_MAX_EVENTS];

    /* TODO timeout clients */
    npos_chk(ret = epoll_wait(server->epoll, epoll_events, EPOLL_MAX_EVENTS, -1));
    if (ret > EPOLL_MAX_EVENTS) {
        fprintf(stderr, "error: invalid number of events returned from epoll_wait\n");
        return -1;
    }

    for (int i = 0; i < ret; ++i) {
        struct epoll_event *event = &epoll_events[i];
        if ((event->events & EPOLLIN) == EPOLLIN && event->data.u32 == EPOLL_DATA_INET) {
            non0_chk(server_handle_new_connection(server,
                                                  CONNECTED_CLIENT_TYPE_INET,
                                                  event_handler));

        } else if ((event->events & EPOLLIN) == EPOLLIN && event->data.u32 == EPOLL_DATA_UNIX) {
            non0_chk(server_handle_new_connection(server,
                                                  CONNECTED_CLIENT_TYPE_UNIX,
                                                  event_handler));

        } else {
            int can_hup = 0;

            if ((event->events & EPOLLIN) == EPOLLIN) {
                printf("event EPOLLIN\n");
                event->events &= ~EPOLLIN;


                if (server_handle_incoming_message(server,
                                                   (int16_t) event->data.u32,
                                                   event_handler) != 0) {
                    fprintf(stderr, "warn: error while reading message\n");
                    can_hup = 1;
                }

            } else if ((event->events & EPOLLOUT) == EPOLLOUT) {
                printf("event EPOLLOUT\n");
                event->events &= ~EPOLLOUT;

            } 
            
            if (can_hup && ((event->events & EPOLLHUP) == EPOLLHUP || (event->events & EPOLLRDHUP) == EPOLLRDHUP)) {
                printf("event EPOLLHUP | EPOLLRDHUP\n");
                event->events &= ~(EPOLLHUP | EPOLLRDHUP);

                non0_chk(server_handle_disconnect(server,
                                                  (int16_t) event->data.u32,
                                                  event_handler));
            }
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
    err_chk(client->type != CONNECTED_CLIENT_TYPE_NONE);

    message->recipient = (int16_t) client_id;
    message->sender = MESSAGE_IDENTITY_SERVER;
    non0_chk(message_send(client->socket, message));

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

    non0_chk(epoll_ctl(server->epoll, EPOLL_CTL_DEL, client->socket, NULL));

    shutdown(client->socket, SHUT_RDWR);
    close(client->socket);

    client->type = CONNECTED_CLIENT_TYPE_NONE;

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

    client->alive = 1;

    return 0;
}

int
client_connect_inet(struct client *client, const char *hostname, int port)
{
    client->alive = 0;
    client->client_id = MESSAGE_IDENTITY_UNKNOWN;
    npos_chk(client->server = socket(AF_INET, SOCK_STREAM, 0));

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
client_connect_unix(struct client *client, const char *path)
{
    client->alive = 0;
    client->client_id = MESSAGE_IDENTITY_UNKNOWN;
    npos_chk(client->server = socket(AF_UNIX, SOCK_STREAM, 0));

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
client_handle_events(struct client *client,
                     client_event_handler_t event_handler)
{
    struct epoll_event epoll_events[2];
    int ret;

    /* TODO implement heartbeat */
    npos_chk(ret = epoll_wait(client->epoll, epoll_events, 2, -1));

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
            printf("event EPOLLIN\n");
            event->events &= ~EPOLLIN;

            struct client_event client_event;
            client_event.type = CLIENT_EVENT_TYPE_SERVER_MESSAGE;

            non0_chk(message_read(client->server, &client_event.message));


            non0_chk(event_handler(client, &client_event));

        } else if ((event->events & EPOLLOUT) == EPOLLOUT) {
            printf("event EPOLLOUT\n");
            event->events &= ~EPOLLOUT;

        } else if ((event->events & EPOLLHUP) == EPOLLHUP || (event->events & EPOLLRDHUP) == EPOLLRDHUP) {
            event->events &= ~(EPOLLHUP | EPOLLRDHUP);
            printf("event EPOLLHUP | EPOLLRDHUP\n");

            struct client_event client_event;
            client_event.type = CLIENT_EVENT_TYPE_SERVER_DROP;

            non0_chk(event_handler(client, &client_event));
        }
    }

    return 0;
}

int
client_send(struct client *client, struct message *message)
{
    message->sender = client->client_id;
    message->recipient = MESSAGE_IDENTITY_SERVER;
    non0_chk(message_send(client->server, message));

    return 0;
}


int
client_destroy(struct client *client)
{
    shutdown(client->server, SHUT_RDWR);
    close(client->server);
    return 0;
}
