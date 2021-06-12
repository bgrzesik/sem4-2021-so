#include "transport.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>

enum client_game_state {
    CLIENT_GAME_STATE_WAITING,
    CLIENT_GAME_STATE_PLAYER_TURN,
    CLIENT_GAME_STATE_OPPONENT_TURN,
};

struct client_game {
    enum client_game_state state;
    enum tick_tac tick_tac;

    char player_name[MESSAGE_PLAYER_NAME_LEN];

    enum tick_tac fields[9];
} client_game;

static void
print_game_field(void)
{
    printf("\n");
    printf("\t\t+0 +1 +2\n");
    for (size_t r = 0; r < 3 ; r++) {
        const char *letter[] = {"1", "4", "7"};
        printf("\t%s\t", letter[r]);

        for (size_t c = 0; c < 3 ; c++) {
            enum tick_tac tick_tac = client_game.fields[r * 3 + c];
            if (tick_tac == TICK_TAC_EMPTY) {
                printf(" _ ");
            } else if (tick_tac == TICK_TAC_X) {
                printf(" X ");
            } else if (tick_tac == TICK_TAC_O) {
                printf(" O ");
            }
        }

        printf("\n");
    }
    printf("\n");
}

static void 
print_player_turn(void)
{
    printf("Your turn: \n");
}

static void
print_instructions(void)
{
    printf("Please enter correct field coordinates\n");
}

static void
print_game_ended(void)
{
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

    for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        int f0 = patterns[i][0], f1 = patterns[i][1], f2 = patterns[i][2];

        if (client_game.fields[f0] == client_game.fields[f1] && client_game.fields[f1] == client_game.fields[f2]) {
            winner = client_game.fields[f0];
            if (winner != TICK_TAC_EMPTY) {
                break;
            }
        }
    }

    printf("Game ended\n");

    if (winner == TICK_TAC_EMPTY) {
        printf("Tie.\n");
    } else if (winner == client_game.tick_tac) {
        printf("You won!\n");
    } else {
        printf("Opponent won :C\n");
    }
}

static int
handle_game_start(struct client *client, struct client_event *event)
{
    printf("Game starting, you're playing against '%s'\n", 
           event->message.game_start.opponent);

    client_game.tick_tac = event->message.game_start.tick_tac;

    if (client_game.tick_tac == TICK_TAC_X) {
        print_game_field();  
        print_player_turn();
        client_game.state = CLIENT_GAME_STATE_PLAYER_TURN;
    } else {
        print_game_field();  
        printf("You are O\n");
        client_game.state = CLIENT_GAME_STATE_OPPONENT_TURN;
    }

    return 0;   
}

static int
handle_player_input(struct client *client, struct client_event *event)
{
    char *input = event->input.text;

    while (*input != '\0' && input < event->input.text + MESSAGE_PLAYER_NAME_LEN) {
        if ('1' <= *input && *input <= '9') {
            break;
        }
        input++;
    }

    if (*input == '\0' || !('1' <= *input && *input <= '9')) {
        print_game_field();  
        print_instructions();
        return 0;
    }

    int field = *input - '1';

    if (client_game.fields[field] != TICK_TAC_EMPTY) {
        print_game_field();  
        print_instructions();
        return 0;
    }

    err_chk(0 <= field && field < 9);

    struct message message;
    message.type = MESSAGE_TYPE_PLAYER_MOVE;
    message.player_move.player = client_game.tick_tac;
    message.player_move.field = field;

    non0_chk(client_send(client, &message));

    client_game.state = CLIENT_GAME_STATE_OPPONENT_TURN;

    return 0;
}

static int
handle_player_move(struct client *client, struct client_event *event)
{
    int field = event->message.player_move.field;
    enum tick_tac tick_tac = event->message.player_move.player;

    err_chk(0 <= field && field < 9);

    client_game.fields[field] = tick_tac;

    if (tick_tac  == client_game.tick_tac) {
        client_game.state = CLIENT_GAME_STATE_OPPONENT_TURN;
        print_game_field();  
    } else {
        client_game.state = CLIENT_GAME_STATE_PLAYER_TURN;
        print_game_field();  
        print_player_turn();
    }

    return 0;
}

int
client_handler(struct client *client, struct client_event *event)
{
    switch (event->type) {

        case CLIENT_EVENT_TYPE_SERVER_MESSAGE:
            switch (event->message.type) {
                case MESSAGE_TYPE_HANDSHAKE:
                    client->client_id = event->message.handshake.client_id;
                    break;

                case MESSAGE_TYPE_GAME_START:
                    non0_chk(handle_game_start(client, event));
                    break;

                case MESSAGE_TYPE_PLAYER_MOVE:
                    non0_chk(handle_player_move(client, event));
                    break;

                case MESSAGE_TYPE_DROP:
                    client->alive = 0;

                    switch (event->message.drop.cause) {
                    
                    case MESSAGE_DROP_CAUSE_SERVER_LIMIT:
                        printf("Exceeded player limit\n");
                        break;

                    case MESSAGE_DROP_CAUSE_TIMEOUT:
                        printf("Timeout\n");
                        break;

                    case MESSAGE_DROP_CAUSE_NAME_CONFLICT:
                        printf("Name conflict\n");
                        break;


                    case MESSAGE_DROP_CAUSE_SECOND_PLAYER:
                        printf("Second player disconnected\n");
                        break;

                    case MESSAGE_DROP_CAUSE_GAME_END:
                        print_game_field();
                        print_game_ended();
                        break;

                    default:
                        printf("Unknown drop cause\n");
                        break;

                    }

                    break;
            }
            break;

        case CLIENT_EVENT_TYPE_SERVER_DROP:
            client->alive = 0;

            break;

        case CLIENT_EVENT_TYPE_STDIN_INPUT:
            if (client_game.state == CLIENT_GAME_STATE_PLAYER_TURN) {
                non0_chk(handle_player_input(client, event));
            } else if (client_game.state == CLIENT_GAME_STATE_WAITING) {
                printf("Please wait for your turn.\n");
            } else {
                printf("Please wait fot the second player to tune in.\n");
            }

            break;
    }

    return 0;
}

struct client client;

static void
sigint_handler(int signal)
{
    client_destroy(&client);
    exit(0);
}

int
main(int argc, const char *argv[])
{
    /* TODO remove */
    sleep(1);


    client_game.state = CLIENT_GAME_STATE_WAITING;

    if (argc != 4) {
        fprintf(stderr, "usage: ./a.out <name> unix|inet <ip>:<port>|<path>\n");
        return -1;
    }

    const char *name = argv[1];
    const char *method = argv[2];
    const char *addr = argv[3];

    if (strcmp(method, "inet") == 0) {
        char *double_dot = strchr(addr, ':');
        if (double_dot == NULL) {
            fprintf(stderr, "usage: ./a.out <name> unix|inet <ip>:<port>|<path>\n");
            return -1;
        }

        *double_dot = '\0';
        int port = atoi(double_dot + 1);
        if (port == 0) {
            fprintf(stderr, "usage: ./a.out <name> unix|inet <ip>:<port>|<path>\n");
            return -1;
        }

        non0_chk(client_connect_inet(&client, addr, port));
    } else if (strcmp(method, "unix") == 0) {
        non0_chk(client_connect_unix(&client, addr, name));
    } else {
        fprintf(stderr, "usage: ./a.out <name> unix|inet <ip>:<port>|<path>\n");
        return -1;
    }

    non0_chk(signal(SIGINT, &sigint_handler));

    strncpy(client_game.player_name, name, MESSAGE_PLAYER_NAME_LEN);

    struct message message;
    message.type = MESSAGE_TYPE_HANDSHAKE;

    strncpy(message.handshake.name, client_game.player_name, MESSAGE_PLAYER_NAME_LEN);

    non0_chk(client_send(&client, &message));

    while (client.alive) {
        non0_chk(client_handle_events(&client, client_handler));
    }

    non0_chk(client_destroy(&client));


    return 0;
}
