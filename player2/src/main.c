#include <MQTTClient.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "game_state.h"
#include "input.h"
#include "mqtt_config.h"
#include "opcodes.h"

MQTTClient client;
MQTTClient_deliveryToken dt;

// Game state
enum GameState game_state;
char *current_payload = NULL;
enum Mark { empty = ' ', p1 = 'X', p2 = 'O'};
enum Mark board[3][3] = {
    empty, empty, empty,
    empty, empty, empty,
    empty, empty, empty
};
int totalMoves = 0;

int messageReceived(void* context, char* topic, int length, MQTTClient_message* message) {
    if (strcmp(topic, P2_TOPIC) == 0 && message->payloadlen > 0) {
        enum Opcodes op = *(char *)message->payload;
        current_payload = message->payload;
        switch (op) {
            case PLAYER_1_TIMEOUT:
                exit(1);
                break;
            case PLAYER_1_QUIT:
                printf("PLAYER 2 WINS\n");
                printf("Player 1 quit.\n");
                sleep(5);
                exit(0);
                break;
        }
    }
}

// Blocks until a message from Player 1 is received
void waitForMessage() {
    while (current_payload == NULL);
}

void updateState(enum GameState state) {
    game_state = state;
    current_payload = NULL;
}

// Prints the board
void printBoard() {
    printf("-------\n");
    for (int i = 0; i < 3; i++) {
        printf("|%c|%c|%c|\n", board[i][0], board[i][1], board[i][2]);
        printf("-------\n");
    }
}

// Gets the mark character for the current player (defined in Mark enum)
char getPlayerMark() {
    return game_state == PLAYER_1_MOVING ? p1 : p2;
}

// Gets move from player
void playerMove(pPlayerMove move) {
    int row = integerInputRange("Enter row: ", 1, 3);
    int column = integerInputRange("Enter column: ", 1, 3);

    // Decrement values to make it a valid index
    row--;
    column--;

    if (board[row][column] == empty) {
        board[row][column] = getPlayerMark();
    } else {
        printf("There is already a mark at this position.\n");
        playerMove(move);
    }

    board[row][column] = p2;
    move->op = PLAYER_2_MOVE;
    move->row = row;
    move->column = column;
}

// Checks if the specified player has won by having all marks in a specific row
int checkRow(int row) {
    char mark = getPlayerMark();
    return board[row][0] == mark && board[row][1] == mark && board[row][2] == mark;
}

// Checks if the specified player has won by having all marks in a specific column
int checkColumn(int column) {
    char mark = getPlayerMark();
    return board[0][column] == mark && board[1][column] == mark && board[2][column] == mark;
}

// Checks if the player has won via diagonal
int checkDiagonals() {
    char mark = getPlayerMark();
    return (board[0][0] == mark && board[1][1] == mark && board[2][2] == mark) ||
        (board[0][2] == mark && board[1][1] == mark && board[2][0] == mark);
}

// Updates game_state if there is a winner
void checkWinner() {
    int rowWin = checkRow(0) || checkRow(1) || checkRow(2);
    int columnWin = checkColumn(0) || checkColumn(1) || checkColumn(2);
    int diagonalWin = checkDiagonals();
    if (rowWin || columnWin || diagonalWin) {
        updateState(game_state == PLAYER_1_MOVING ? PLAYER_1_WIN : PLAYER_2_WIN);
    } else if (totalMoves == 9) {
        updateState(PLAYER_TIE);
    }
}

void signalHandler(int dummy) {
    if (game_state == PLAYER_1_MOVING || game_state == PLAYER_2_MOVING) {
        char payload = PLAYER_2_QUIT;
        MQTTClient_publish(client, P1_TOPIC, 3, &payload, QOS, 0, &dt);
        MQTTClient_waitForCompletion(client, dt, 3000);
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    exit(0);
}

int main(int argc, char* argv[]) {
    // Initialize MQTT
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_create(&client, ADDRESS, CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(client, NULL, NULL, messageReceived, NULL); // TODO: Handle failed/lost connection
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    // Connect to broker
    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    MQTTClient_subscribe(client, P2_TOPIC, QOS);
    signal(SIGINT, signalHandler); // Player presses Ctrl + C
    signal(SIGHUP, signalHandler); // Player closes window

    // Ask user if they want to play tic-tac-toe
    int response = booleanInput("Would you like to play tic-tac-toe? (y/n): ");
    if (!response) {
        // User answered no; publish response and quit
        char payload = PLAYER_2_DENY;
        MQTTClient_publish(client, P1_TOPIC, 1, &payload, QOS, 0, &dt);
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        return 0;
    } else {
        char payload = PLAYER_2_CONFIRM;
        MQTTClient_publish(client, P1_TOPIC, 1, &payload, QOS, 0, &dt);
    }

    // Wait for Player 1 to decide who goes first
    printf("Waiting for response from Player 1...\n");
    waitForMessage();
    pInitGame init = (pInitGame)current_payload;
    updateState(init->first_player == 1 ? PLAYER_1_MOVING : PLAYER_2_MOVING);

    // Game loop
    while (game_state != PLAYER_1_WIN && game_state != PLAYER_2_WIN && game_state != PLAYER_TIE) {
        printBoard();
        printf("=======\n");
        printf("PLAYER %d MOVE (%c)\n", game_state - 2, getPlayerMark());
        if (game_state == PLAYER_1_MOVING) {
            printf("Waiting for Player 1's move...\n");
            waitForMessage();
            pPlayerMove move = (pPlayerMove)current_payload;
            board[move->row][move->column] = p1;
        } else {
            PlayerMove move;
            playerMove(&move);
            MQTTClient_publish(client, P1_TOPIC, 3, &move, QOS, 0, &dt);
        }
        totalMoves++;
        checkWinner();
        if (game_state != PLAYER_1_WIN && game_state != PLAYER_2_WIN && game_state != PLAYER_TIE)
            updateState(game_state == PLAYER_1_MOVING ? PLAYER_2_MOVING : PLAYER_1_MOVING);
    }

    printBoard();

    if (game_state == PLAYER_TIE) {
        printf("TIE GAME\n");
    } else {
        printf("WINNER: PLAYER %d\n", game_state == PLAYER_1_WIN ? 1 : 2);
    }
    sleep(5);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}