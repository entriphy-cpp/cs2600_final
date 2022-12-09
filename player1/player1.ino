#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
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
WiFiClient wifiClient;
PubSubClient client(wifiClient);


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
  lcd.setCursor(0, 0);
  lcd.print("WiFi: Connecting");
  lcd.setCursor(0, 1);
  lcd.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi!");
  lcd.clear();

  // Connect to MQTT broker
  client.setServer(ADDRESS, 1883);
  client.setCallback(callback);
  while (!client.connected()) { // Loop until succesfully connected to broker
    Serial.printf("Connecting to %s as %s...\n", ADDRESS, CLIENT_ID);
    lcd.setCursor(0, 0);
    lcd.print("MQTT: Connecting");
    lcd.setCursor(0, 1);
    lcd.print(TOPIC);
    if (client.connect(CLIENT_ID)) { // Attempt to connect to broker
        Serial.println("Connected to MQTT broker!");
        client.subscribe(TOPIC); // Subscribe to topic
    } else {
        Serial.printf("Failed to connect (%d); retrying...\n", client.state());
        lcd.print("MQTT: Failed");
        lcd.setCursor(0, 1);
        delay(2000);
    }
  }
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Press 0 to play!");
}

void loop() {
  client.loop();
  if (irrecv.decode(&results)) {
    int key = keyToInt(results.value);
    if (key == 0) {
      lcd.setCursor(0, 1);
      lcd.print("Waiting...");
      char payload[] = { PLAYER_1_ASK };
      if (client.publish(TOPIC, payload)) {
        Serial.println("Sent PLAYER_1_ASK!");
        lcd.setCursor(0, 0);
      } else {
        Serial.println("Failed to send PLAYER_1_ASK.");
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
  // Do nothing for now
}
