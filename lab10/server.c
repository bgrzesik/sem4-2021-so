#include "transport.h"
#include <signal.h>
#include <string.h>

enum player_state {
    PLAYER_STATE_NONE,
    PLAYER_STATE_NOTPLAYING,
    PLAYER_STATE_PLAYING,
};

#define GAME_PLANE_COUNT (SERVER_MAX_CLIENTS / 2 + 1)

struct game_plane {
    int taken;

    int16_t player_a; /* X */
    int16_t player_b; /* O */
    int16_t player_turn;

    enum tick_tac fields[9];
};

struct player {
    char name[MESSAGE_PLAYER_NAME_LEN];
    enum player_state state;
    struct game_plane *plane;
};

struct games {
    struct player players[SERVER_MAX_CLIENTS];
    struct game_plane planes[GAME_PLANE_COUNT];
} games;


static int
players_match(struct server *server, int16_t player_a, int16_t player_b)
{
    printf("matching %hd %hd\n", player_a, player_b);

    err_chk(games.players[player_a].state == PLAYER_STATE_NOTPLAYING);
    err_chk(games.players[player_b].state == PLAYER_STATE_NOTPLAYING);

    size_t plane_id;
    for (plane_id = 0; plane_id < GAME_PLANE_COUNT; plane_id++) {
        if (!games.planes[plane_id].taken) {
            break;
        }
    }

    struct game_plane *plane = &games.planes[plane_id];
    plane->taken = 1;
    plane->player_a = player_a;
    plane->player_b = player_b;
    plane->player_turn = player_a;

    for (size_t i = 0; i < 9 ; i++) {
        plane->fields[i] = TICK_TAC_EMPTY;
    }

    games.players[player_a].plane = plane;
    games.players[player_b].plane = plane;

    struct message message;
    message.type = MESSAGE_TYPE_GAME_START;
    message.game_start.tick_tac = TICK_TAC_X;
    strncpy(message.game_start.opponent, 
            games.players[player_b].name,
            MESSAGE_PLAYER_NAME_LEN);

    non0_chk(connected_client_send(server, player_a, &message));

    message.type = MESSAGE_TYPE_GAME_START;
    message.game_start.tick_tac = TICK_TAC_O;
    strncpy(message.game_start.opponent, 
            games.players[player_a].name,
            MESSAGE_PLAYER_NAME_LEN);

    non0_chk(connected_client_send(server, player_b, &message));

    games.players[player_a].state = PLAYER_STATE_PLAYING;
    games.players[player_b].state = PLAYER_STATE_PLAYING;

    return 0;
}

static int
handle_handshake(struct server *server, struct server_event *event)
{
    printf("\tHANDSHAKE %s\n", event->message.handshake.name);

    int16_t client_id = event->client->id;
    int16_t opponent_id = -1;

    for (size_t i = 0; i < SERVER_MAX_CLIENTS; i++) {
        if (games.players[i].state == PLAYER_STATE_NONE) {
            continue;
        } else if (opponent_id == -1 && games.players[i].state == PLAYER_STATE_NOTPLAYING) {
            opponent_id = i;
        }

        if (strncmp(event->message.handshake.name,
                    games.players[i].name, MESSAGE_PLAYER_NAME_LEN) == 0) {

            printf("name collision droping client\n");
            non0_chk(connected_client_drop(server, client_id, 
                                           MESSAGE_DROP_CAUSE_NAME_CONFLICT));

            return 0;
        }
    }

    struct player *player = &games.players[client_id];
    strncpy(player->name, event->message.handshake.name, MESSAGE_PLAYER_NAME_LEN);
    player->state = PLAYER_STATE_NOTPLAYING;

    struct message res;
    res.type = MESSAGE_TYPE_HANDSHAKE;
    res.handshake.client_id = client_id;

    non0_chk(connected_client_send(server, client_id, &res));

    if (opponent_id != -1) {
        non0_chk(players_match(server, opponent_id, client_id));
    }

    return 0;
}

int
handle_player_move(struct server *server, struct server_event *event)
{
    uint16_t client_id = event->client->id;
    struct player *sender = &games.players[client_id];
    struct game_plane *plane = sender->plane;

    err_chk(plane->player_turn == client_id);
    
    size_t field = event->message.player_move.field;

    err_chk(0 <= field && field < 9);

    enum tick_tac tick_tac = plane->player_a == client_id ? TICK_TAC_X : TICK_TAC_O;
    plane->fields[field] = tick_tac;

    struct message message;
    message.type = MESSAGE_TYPE_PLAYER_MOVE;
    message.player_move.field = field;
    message.player_move.player = tick_tac;

    if (client_id == plane->player_a) {
        plane->player_turn = plane->player_b;
    } else {
        plane->player_turn = plane->player_a;
    }

    non0_chk(connected_client_send(server, plane->player_a, &message));
    non0_chk(connected_client_send(server, plane->player_b, &message));

    /*
        0 1 2
        3 4 5
        6 7 8
    */
    int patterns[][3] ={
        {0, 1, 2},
        {3, 4, 5},
        {6, 7, 8},


        {0, 3, 6},
        {1, 4, 7},
        {2, 5, 8},

        {0, 4, 8},
        {2, 4, 6},
    };

    enum tick_tac winner = TICK_TAC_EMPTY;
    int any_empty = 0;

    for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        int f0 = patterns[i][0], f1 = patterns[i][1], f2 = patterns[i][2];

        any_empty = any_empty || plane->fields[f0] == TICK_TAC_EMPTY;
        any_empty = any_empty || plane->fields[f1] == TICK_TAC_EMPTY;
        any_empty = any_empty || plane->fields[f2] == TICK_TAC_EMPTY;

        if (plane->fields[f0] == plane->fields[f1] && plane->fields[f1] == plane->fields[f2]) {
            winner = plane->fields[f0];
            if (winner != TICK_TAC_EMPTY) {
                break;
            }
        }
    }
    
    if (!any_empty || winner != TICK_TAC_EMPTY) {
        non0_chk(connected_client_drop(server, plane->player_a, 
                                       MESSAGE_DROP_CAUSE_GAME_END));

        non0_chk(connected_client_drop(server, plane->player_b, 
                                       MESSAGE_DROP_CAUSE_GAME_END));

        games.players[plane->player_a].state = PLAYER_STATE_NONE;
        games.players[plane->player_b].state = PLAYER_STATE_NONE;
    }

    return 0;
}

static int
handle_client_drop(struct server *server, struct server_event *event)
{
    int16_t client_id = event->client->id;
    struct player *sender = &games.players[client_id];

    if (sender->state != PLAYER_STATE_PLAYING) {
        sender->state = PLAYER_STATE_NONE;
        return 0;
    }

    struct game_plane *plane = sender->plane;

    if (plane->player_a == client_id) {
        non0_chk(connected_client_drop(server, plane->player_b, 
                                       MESSAGE_DROP_CAUSE_SECOND_PLAYER));
    } else {
        non0_chk(connected_client_drop(server, plane->player_a, 
                                       MESSAGE_DROP_CAUSE_SECOND_PLAYER));
    }

    games.players[plane->player_a].state = PLAYER_STATE_NONE;
    games.players[plane->player_b].state = PLAYER_STATE_NONE;

    return 0;
}

int
server_handler(struct server *server, struct server_event *event)
{
    printf("server event: ");
    switch (event->type) {
        case SERVER_EVENT_TYPE_CLIENT_CONNECTED:
            printf("CONNECTED\n");
            break;

        case SERVER_EVENT_TYPE_CLIENT_MESSAGE:
            printf("MESSAGE\n");

            switch (event->message.type) {
                case MESSAGE_TYPE_HANDSHAKE:
                    non0_chk(handle_handshake(server, event));
                    break;

                case MESSAGE_TYPE_PLAYER_MOVE:
                    non0_chk(handle_player_move(server, event));
                    break;

                case MESSAGE_TYPE_DROP:
                    printf("\tDROP\n");
                    break;

                default:
                    fprintf(stderr, "error: invalid message received\n");
                    return -1;
            }

            break;

        case SERVER_EVENT_TYPE_CLIENT_DROP:
            printf("DROP\n");
            handle_client_drop(server, event);
            break;
    }

    return 0;
}

struct server server;

static void
sigint_handler(int signal)
{
    server_destroy(&server);
    exit(0);
}

int
main(int argc, const char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: ./a.out <port> <unix>\n");
        return -1;
    }

    int port = atoi(argv[1]);
    const char *path = argv[2];
    if (port == 0 || argv[2][0] == '\0') {
        fprintf(stderr, "usage: ./a.out <port> <unix>\n");
        return -1;
    }

    for (size_t i = 0; i < SERVER_MAX_CLIENTS; i++) {
        games.players[i].state = PLAYER_STATE_NONE;
        games.players[i].name[0] = '\0';
    }

    for (size_t i = 0; i < GAME_PLANE_COUNT; i++) {
        games.planes[i].taken = 0;
    }

    non0_chk(server_init(&server, port, path));

    non0_chk(signal(SIGINT, &sigint_handler));

    while (1) {
        non0_chk(server_poll_events(&server, &server_handler));
    }
    non0_chk(server_destroy(&server));

    return 0;
}