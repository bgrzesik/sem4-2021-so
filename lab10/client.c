#include "transport.h"
#include <stdio.h>
#include <string.h>

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
    printf("\t\t0 1 2\n");
    for (size_t r = 0; r < 3 ; r++) {
        const char *letter[] = {"A", "B", "C"};
        printf("\t%s\t", letter[r]);

        for (size_t c = 0; c < 3 ; c++) {
            enum tick_tac tick_tac = client_game.fields[r * 3 + c];
            if (tick_tac == TICK_TAC_EMPTY) {
                printf("_ ");
            } else if (tick_tac == TICK_TAC_X) {
                printf("X ");
            } else if (tick_tac == TICK_TAC_O) {
                printf("O ");
            }
        }

        printf("\n");
    }
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
        if (*input == 'A' || *input == 'B' || *input =='C') {
            break;
        }
        input++;
    }

    if (*input == '\0' || (input[1] != '0' && input[1] != '1' && input[1] != '2')) {
        print_game_field();  
        print_instructions();
        return 0;
    }

    int field = (*input - 'A') * 3 + (input[1] - '0');

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

    printf("event.type = %d\n", event->type);
    switch (event->type) {

        case CLIENT_EVENT_TYPE_SERVER_MESSAGE:
            printf("MESSAGE\n");


            switch (event->message.type) {
                case MESSAGE_TYPE_HANDSHAKE:
                    printf("\tHANDSHAKE\n");
                    client->client_id = event->message.handshake.client_id;
                    break;

                case MESSAGE_TYPE_GAME_START:
                    printf("\tGAME_START\n");
                    non0_chk(handle_game_start(client, event));
                    break;

                case MESSAGE_TYPE_PLAYER_MOVE:
                    printf("\tPLAYER_MOVE\n");
                    non0_chk(handle_player_move(client, event));
                    break;

                case MESSAGE_TYPE_DROP:
                    printf("\tDROP\n");
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
            printf("DROP\n");
            client->alive = 0;

            break;

        case CLIENT_EVENT_TYPE_STDIN_INPUT:
            printf("STDIN\n");

            if (client_game.state != CLIENT_GAME_STATE_PLAYER_TURN) {
                printf("Please wait for your turn.\n");
            } else {
                non0_chk(handle_player_input(client, event));
            }

            break;
    }

    return 0;
}

int
main(int argc, const char *argv[])
{
    /* TODO remove */
    sleep(1);

    struct client client;

    client_game.state = CLIENT_GAME_STATE_WAITING;

    // non0_chk(client_connect_inet(&client, "127.0.0.1", TICK_TACK_TOE_PORT));
    non0_chk(client_connect_unix(&client, "./server"));

    char *name = client_game.player_name;
    size_t bytes = sizeof(client_game.player_name);

    printf("Enter player name: ");
    npos_chk(bytes = getline(&name, &bytes, stdin));
    name[bytes - 1] = '\0';

    struct message message;
    message.type = MESSAGE_TYPE_HANDSHAKE;

    strncpy(message.handshake.name, 
            client_game.player_name,
            MESSAGE_PLAYER_NAME_LEN);

    non0_chk(client_send(&client, &message));

    while (client.alive) {
        non0_chk(client_handle_events(&client, client_handler));
    }

    non0_chk(client_destroy(&client));


    return 0;
}
