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
enum GameState game_state;
long timeout_time;
byte *current_payload;
bool isCpu = false;
bool isQuit = false;
enum Mark { empty = ' ', p1 = 'X', p2 = 'O'};
enum Mark board[3][3] = {
    empty, empty, empty,
    empty, empty, empty,
    empty, empty, empty
};
int totalMoves = 0;

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
  
  resetGame();
}

void loop() {
  // Required to keep MQTT connection alive
  client.loop();

  // Received input from IR remote
  if (irrecv.decode(&results)) {
    int key = keyToInt(results.value);
    if (key != -1) {
      switch (game_state) {
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
        case PLAYER_1_MOVING:
          if (key >= 1 && key <= 9) {
            key--;
            int row = key / 3;
            int column = key % 3;
            if (board[row][column] == empty) {
                board[row][column] = p1;
                updateBoardDisplay();
                if (!isCpu) {
                  PlayerMove move = { PLAYER_1_MOVE, row, column };
                  client.publish(P2_TOPIC, (char *)&move);
                }
                totalMoves++;
                checkWinner();
                if (game_state != PLAYER_1_WIN && game_state != PLAYER_2_WIN && game_state != PLAYER_TIE && game_state != PLAYER_1_QUIT && game_state != PLAYER_2_QUIT)
                  updateState(game_state == PLAYER_1_MOVING ? PLAYER_2_MOVING : PLAYER_1_MOVING);
            } else {
                lcdPrint("Player 1 Move", "Cannot place", true);
            }
          } else if (key == 10) {
            if (!isCpu) {
              char payload = PLAYER_1_QUIT;
              client.publish(P2_TOPIC, &payload);
            }
            isQuit = true;
            updateState(PLAYER_2_WIN);
          }
          break;
        case PLAYER_2_MOVING:
          if (key == 10) {
            if (!isCpu) {
              char payload = PLAYER_1_QUIT;
              client.publish(P2_TOPIC, &payload);
            }
            isQuit = true;
            updateState(PLAYER_2_WIN);
          }
          break;
      }
    }

    irrecv.resume();
  }

  // Serial debug for 8x8 matrix
  if (Serial.available() > 0) {
    String s = Serial.readString();
    s.trim();
    int x = s.charAt(0) - 0x30;
    int y = s.charAt(2) - 0x30;
    pixels[x][y] = s.endsWith("!") ? LOW : HIGH;
  }

  // Game loop
  switch (game_state) {
    case PLAYER_1_IDLE:
      // Waiting for play to press 0 on remote
      break;
    case PLAYER_1_WAITING:
      if (asyncDelay(10)) {
        // Player 2 did not respond to request
        char payload = PLAYER_1_TIMEOUT;
        client.publish(P2_TOPIC, &payload);
        lcdPrint("P2: Timeout", "Playing vs. CPU!", true);
        updateState(PLAYER_1_DISPLAY_PLAYER);
        updateBoardDisplay();
        isCpu = true;
      } else {
        if (current_payload != NULL && (current_payload[0] == PLAYER_2_CONFIRM || current_payload[0] == PLAYER_2_DENY)) {
          lcdPrint(current_payload[0] == PLAYER_2_CONFIRM ? "P2: Confirmed" : "P2: Denied", current_payload[0] == PLAYER_2_CONFIRM ? "Playing vs. P2!" : "Playing vs. CPU!", true);
          isCpu = current_payload[0] == PLAYER_2_DENY;
          updateState(PLAYER_1_DISPLAY_PLAYER);
          updateBoardDisplay();
        } else {
          // Print number of seconds to LCD
          char buf[2];
          lcdPrint("", itoa((millis() - timeout_time) / 1000, buf, 10), false);
        }
      }
      break;
    case PLAYER_1_DISPLAY_PLAYER:
      if (asyncDelay(3)) {
        byte first_player = (esp_random() % 2) + 1;
        char payload[] = { PLAYER_1_INIT, first_player };
        client.publish(P2_TOPIC, payload);
        char buf[1];
        lcdPrint("Player", itoa(first_player, buf, 10), true);
        updateState(first_player == 1 ? PLAYER_1_MOVING : PLAYER_2_MOVING);
      }
      break;
    case PLAYER_1_MOVING:
      // Wait for player to press input on remote
      break;
    case PLAYER_2_MOVING:
      if (!isCpu) {
        if (current_payload != NULL && (current_payload[0] == PLAYER_2_MOVE || current_payload[0] == PLAYER_2_QUIT)) {
          if (current_payload[0] == PLAYER_2_QUIT) {
            updateState(PLAYER_1_WIN);
            isQuit = true;
          } else {
            pPlayerMove move = (pPlayerMove)current_payload;
            board[move->row][move->column] = p2;
            updateBoardDisplay();
            totalMoves++;
            checkWinner();
            if (game_state != PLAYER_1_WIN && game_state != PLAYER_2_WIN && game_state != PLAYER_TIE)
                updateState(PLAYER_1_MOVING);
          }
        }
      } else if (asyncDelay(2)) {
        int row = esp_random() % 3;
        int column = esp_random() % 3;
        while (board[row][column] != empty) {
            row = rand() % 3;
            column = rand() % 3;
        }
        board[row][column] = p2;
        updateBoardDisplay();
        totalMoves++;
        checkWinner();
        if (game_state != PLAYER_1_WIN && game_state != PLAYER_2_WIN && game_state != PLAYER_TIE)
          updateState(PLAYER_1_MOVING);
      }
      
      break;
    case PLAYER_1_WIN:
    case PLAYER_2_WIN:
    case PLAYER_TIE:
      if (asyncDelay(5))
        resetGame();
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

void updateState(enum GameState state) {
  game_state = state;
  timeout_time = millis();
  current_payload = NULL;

  switch (state) {
    case PLAYER_1_MOVING:
      lcdPrint("Player 1 Move", "Press a # (1-9)", true);
      break;
    case PLAYER_2_MOVING:
      lcdPrint("Player 2 Move", "Waiting...", true);
      break;
    case PLAYER_1_WIN:
      lcdPrint("Player 1 wins!", isQuit ? "Player 2 quit :(" : "", true);
      break;
    case PLAYER_2_WIN:
      lcdPrint("Player 2 wins!", isQuit ? "Player 1 quit :(" : "", true);
      break;
    case PLAYER_TIE:
      lcdPrint("Tie game.", "", true);
      break;
  }
}

void resetGame() {
  lcdPrint("Press 0 to play!", "", true);
  game_state = PLAYER_1_IDLE;
  timeout_time = millis();
  current_payload = NULL;
  isCpu = false;
  isQuit = false;
  totalMoves = 0;
  resetBoard();
  idleBoard();
}

void idleBoard() {
  resetDisplay();
  pixels[1][2] = HIGH;
  pixels[1][3] = HIGH;
  pixels[1][4] = HIGH;
  pixels[1][5] = HIGH;
  pixels[1][5] = HIGH;
  pixels[6][2] = HIGH;
  pixels[6][3] = HIGH;
  pixels[6][4] = HIGH;
  pixels[6][5] = HIGH;
  pixels[6][5] = HIGH;

  pixels[2][2] = HIGH;
  pixels[3][2] = HIGH;
  pixels[4][2] = HIGH;
  pixels[5][2] = HIGH;
  pixels[6][2] = HIGH;
  pixels[2][5] = HIGH;
  pixels[3][5] = HIGH;
  pixels[4][5] = HIGH;
  pixels[5][5] = HIGH;
  pixels[6][5] = HIGH;
}

void updateBoardDisplay() {
  resetDisplay();
  for (int x = 0; x < 3; x++) {
    for (int y = 0; y < 3; y++) {
      if (board[x][y] != empty) {
        int row = x * 3;
        int column = y * 3;
        pixels[row + 1][column] = HIGH;
        pixels[row][column + 1] = HIGH;
        if (board[x][y] == p2) {
          pixels[row][column] = HIGH;
          pixels[row + 1][column + 1] = HIGH;
        }
      }
    }
  }
}

void resetDisplay() {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      pixels[x][y] = LOW;
    }
  }
}

void resetBoard() {
  for (int x = 0; x < 3; x++) {
    for (int y = 0; y < 3; y++) {
      board[x][y] = empty;
    }
  }
}

// Gets the mark character for the current player (defined in Mark enum)
char getPlayerMark() {
    return game_state == PLAYER_1_MOVING ? p1 : p2;
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

// Returns 1 or 2 if the specified player won (-1 if tie, 0 if no winner)
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