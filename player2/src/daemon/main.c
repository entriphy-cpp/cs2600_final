#include <MQTTClient.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt_config.h"
#include "opcodes.h"

int messageReceived(void* context, char* topic, int length, MQTTClient_message* message) {
    if (strcmp(topic, TOPIC) == 0 && message->payloadlen == 1 && *((char*)message->payload) == PLAYER_1_ASK) {
        printf("Received PLAYER_1_ASK!\n", topic, length, message->payload);
    }
}

int main(int argc, char* argv[]) {
    // Initialize MQTT
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_create(&client, ADDRESS, DAEMON_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(client, NULL, NULL, messageReceived, NULL); // TODO: Handle failed/lost connection
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    // Connect to broker
    int rc;
    printf("Connecting to %s as %s...\n", ADDRESS, DAEMON_ID);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Connection successful!\n", ADDRESS, DAEMON_ID);

    // Subscribe to topic and infinitely loop
    printf("Subscribing to %s; waiting for PLAYER_1_ASK...\n", TOPIC, DAEMON_ID);
    MQTTClient_subscribe(client, TOPIC, QOS);

    int ch;
    do {
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');
    
    // Disconnect from broker
    printf("Disconnecting...\n");
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}