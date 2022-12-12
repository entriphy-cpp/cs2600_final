#include <MQTTClient.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"
#include "mqtt_config.h"
#include "opcodes.h"

int messageReceived(void* context, char* topic, int length, MQTTClient_message* message) {
    if (strcmp(topic, P2_TOPIC) == 0 && message->payloadlen > 0) {
        enum Opcodes op = *(char *)message->payload;
        switch (op) {
            case PLAYER_1_TIMEOUT:
                exit(1);
                break;
        }
    }
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
        // User answered no
        payload_buffer[0] = PLAYER_2_DENY;
        MQTTClient_publish(client, P1_TOPIC, 1, payload_buffer, QOS, 0, &dt);
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        return 0;
    } else {
        payload_buffer[0] = PLAYER_2_CONFIRM;
        MQTTClient_publish(client, P1_TOPIC, 1, payload_buffer, QOS, 0, &dt);
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}