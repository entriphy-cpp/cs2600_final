#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <string.h>
#include <WiFi.h>
#include <Wire.h>
#include "game_state.h"
#include "ir_remote.h"
#include "mqtt_config.h"
#include "opcodes.h"
#include "wifi_secrets.h"

extern int keyToInt(int key);

// IR setup
const uint16_t recvPin = 35;
IRrecv irrecv(recvPin);
decode_results results;

// 8x8 LED matrix setup
const int row[8] = {  0, 2, 13, 18, 32, 12, 33, 27 };
const int col[8] = { 19, 25, 26,  4, 14,  5, 15, 23 };
int pixels[8][8];

// LCD pins
#define SDA 21
#define SCL 22
LiquidCrystal_I2C lcd(0x27, 16, 2);

// WiFi/MQTT connection
WiFiClient wifi_client;
PubSubClient client(wifi_client);

// Game state
enum GameState state;
long timeout_time;
byte *current_payload;
bool isCpu = false;

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();

  // Setup pins
  for (int pin = 0; pin < 8; pin++) {
    Serial.printf("%d: %dx%d\n", pin, col[pin], row[pin]);
    pinMode(col[pin], OUTPUT);
    pinMode(row[pin], OUTPUT);
    digitalWrite(row[pin], LOW);
    digitalWrite(col[pin], HIGH);
  }

  // Reset LED matrix
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      pixels[x][y] = LOW;
    }
  }

  // Setup LCD
  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.printf("Connecting to WiFi @ %s...", WIFI_SSID);
  lcdPrint("WiFi: Connecting", WIFI_SSID, true);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi!");

  // Connect to MQTT broker
  client.setServer(ADDRESS, 1883);
  client.setCallback(callback);
  while (!client.connected()) { // Loop until succesfully connected to broker
    Serial.printf("Connecting to %s as %s...\n", ADDRESS, CLIENT_ID);
    lcdPrint("MQTT: Connecting", P1_TOPIC, true);
    if (client.connect(CLIENT_ID)) { // Attempt to connect to broker
        Serial.println("Connected to MQTT broker!");
        client.subscribe(P1_TOPIC); // Subscribe to topic
    } else {
        Serial.printf("Failed to connect (%d); retrying...\n", client.state());
        lcdPrint("", "MQTT: Failed", false);
        delay(2000);
    }
  }
  
  lcdPrint("Press 0 to play!", "", true);
}

void loop() {
  client.loop();
  if (irrecv.decode(&results)) {
    int key = keyToInt(results.value);
    if (key != -1) {
      switch (state) {
        case PLAYER_1_IDLE:
          if (key == 0) {
            char payload[] = { PLAYER_1_ASK };
            if (client.publish(P2_TOPIC, payload)) {
              Serial.println("Sent PLAYER_1_ASK!");
              lcdPrint("Waiting for P2", "", true);
              updateState(PLAYER_1_WAITING);
            } else {
              Serial.println("Failed to send PLAYER_1_ASK.");
            }
          }
          break;
      }
    }

    irrecv.resume();
  }

  if (Serial.available() > 0) {
    String s = Serial.readString();
    s.trim();
    int x = s.charAt(0) - 0x30;
    int y = s.charAt(2) - 0x30;
    pixels[x][y] = s.endsWith("!") ? LOW : HIGH;
  }

  switch (state) {
    case PLAYER_1_IDLE:
      // Animation
      break;
    case PLAYER_1_WAITING:
      if (asyncDelay(10)) {
        char payload[] = { PLAYER_1_TIMEOUT };
        client.publish(P2_TOPIC, payload);
        lcdPrint("P2: No response", "Playing vs. CPU!", true);
        updateState(PLAYER_1_DISPLAY_PLAYER);
        isCpu = true;
      } else {
        if (current_payload != NULL && (current_payload[0] == PLAYER_2_CONFIRM || current_payload[0] == PLAYER_2_DENY)) {
          lcdPrint(current_payload[0] == PLAYER_2_CONFIRM ? "P2: Confirm" : "P2: Deny", current_payload[0] == PLAYER_2_CONFIRM ? "Playing vs. P2!" : "Playing vs. CPU!", true);
          updateState(PLAYER_1_DISPLAY_PLAYER);
          isCpu = current_payload[0] == PLAYER_2_DENY;
        } else {
          char buf[2];
          lcdPrint("", itoa((millis() - timeout_time) / 1000, buf, 10), false);
        }
        
      }
      break;
    case PLAYER_1_DISPLAY_PLAYER:
      if (asyncDelay(3000)) {
        state = PLAYER_1_MOVING;
      }
      break;
  }

  refreshScreen();
  delay(5);
}


void refreshScreen() { 
  for (byte x = 0; x < 8; x++) {
    digitalWrite(row[x], HIGH);
    for (byte y = 0; y < 8; y++) {
      digitalWrite(col[y], !pixels[x][y]);
      delayMicroseconds(100);
      digitalWrite(col[y], 1);
    }
    digitalWrite(row[x], LOW);
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  if (strcmp(topic, P1_TOPIC) == 0 && length > 0) {
    current_payload = payload;
  }
}

void lcdPrint(const char *row1, const char *row2, int clear) {
  if (clear)
    lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(row1);
  lcd.setCursor(0, 1);
  lcd.print(row2);
}

int asyncDelay(int timeout) {
  return millis() > (timeout_time + timeout * 1000);
}

void updateState(enum GameState game_state) {
  state = game_state;
  timeout_time = millis();
  current_payload = NULL;
}

void resetGame() {
  lcdPrint("Press 0 to play!", "", true);
  state = PLAYER_1_IDLE;
  timeout_time = millis();
  current_payload = NULL;
  isCpu = false;
}