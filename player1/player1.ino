#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t recvPin = 15;
IRrecv irrecv(recvPin);
decode_results results;

const int row[8] = {
  0, 21, 13, 18, 32, 12, 33, 27
};

const int col[8] = {
  19, 25, 26, 4, 14, 5, 22, 23
};

int pixels[8][8];

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();

  for (int pin = 0; pin < 8; pin++) {
    Serial.printf("%d: %dx%d\n", pin, col[pin], row[pin]);
    pinMode(col[pin], OUTPUT);
    pinMode(row[pin], OUTPUT);
    digitalWrite(row[pin], LOW);
    digitalWrite(col[pin], HIGH);
  }

  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      pixels[x][y] = LOW;
    }
  }
}

void loop() {
  if (irrecv.decode(&results)) {
    if (((results.value & 0xFF0000) >> 16) == 0xFF && (results.value & 0xFFFF) != 0xFFFF) {
      Serial.println("Valid!");
    } else {
      Serial.println("Invalid.");
    }
    Serial.println(results.value, HEX);
    Serial.println();
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

void refreshScreen()
{ 
  for (byte i = 0; i < 8; i++)
    {
      digitalWrite(row[i], HIGH);
      for (byte a = 0; a < 8; a++) 
      {
        digitalWrite(col[a], !pixels[i][a]);
        delayMicroseconds(100);
        digitalWrite(col[a], 1);
      }
      digitalWrite(row[i], LOW);
  }
}
