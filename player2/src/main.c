#include <MQTTClient.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "game_state.h"
#include "input.h"
#include "mqtt_config.h"
#include "opcodes.h"

char *current_payload = NULL;
enum Mark { empty = ' ', p1 = 'X', p2 = 'O'};
enum Mark board[3][3] = {
    empty, empty, empty,
    empty, empty, empty,
    empty, empty, empty
};


int messageReceived(void* context, char* topic, int length, MQTTClient_message* message) {
    if (strcmp(topic, P2_TOPIC) == 0 && message->payloadlen > 0) {
        enum Opcodes op = *(char *)message->payload;
        current_payload = message->payload;
        switch (op) {
            case PLAYER_1_TIMEOUT:
                exit(1);
                break;
        }
    }
}

void waitForMessage() {
    while (current_payload == NULL);
}

int main(int argc, char* argv[]) {
    // Initialize MQTT
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_deliveryToken dt;
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
    char payload_buffer[20];

    // Ask user if they want to play tic-tac-toe
    int response = booleanInput("Would you like to play tic-tac-toe? (y/n): ");
    if (!response) {
        // User answered no; publish response and quit
        payload_buffer[0] = PLAYER_2_DENY;
        MQTTClient_publish(client, P1_TOPIC, 1, payload_buffer, QOS, 0, &dt);
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        return 0;
    } else {
        payload_buffer[0] = PLAYER_2_CONFIRM;
        MQTTClient_publish(client, P1_TOPIC, 1, payload_buffer, QOS, 0, &dt);
    }

    // Game state
    enum GameState game_state = PLAYER_1_DISPLAY_PLAYER;

    // Wait for Player 1 to decide who goes first
    printf("Waiting for response from Player 1...\n");
    waitForMessage();
    pInitGame init = (pInitGame)current_payload;
    game_state = init->first_player == 1 ? PLAYER_1_MOVING : PLAYER_2_MOVING;
    printf("%d\n", init->first_player);
    sleep(3);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}