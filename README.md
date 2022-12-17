# CS 2600: Tic-Tac-Toe (Final)
Play a game of tic-tac-toe between an ESP32 and a computer over MQTT.

## Requirements
* [paho.mqtt.c](https://github.com/eclipse/paho.mqtt.c) (for Player 2 code)
* [Mosquitto](https://mosquitto.org/) (for Bash auto script)
* Arduino libraries (for Player 1)
    * *LiquidCrystal I2C* by Frank de Brabander
    * *PubSubClient* by Nick O'Leary
    * *IRremoteESP8266* by Sebastien Warin (IC pins for numpad were bent :()

## P2 Output
```
Would you like to play tic-tac-toe? (y/n): y
Waiting for response from Player 1...
=======
-------
| | | |
-------
| | | |
-------
| | | |
-------
PLAYER 1 MOVE (X)
Waiting for Player 1's move...
=======
-------
|X| | |
-------
| | | |
-------
| | | |
-------
PLAYER 2 MOVE (O)
Enter row: 3
Enter column: 1
=======
-------
|X| | |
-------
| | | |
-------
|O| | |
-------
PLAYER 1 MOVE (X)
Waiting for Player 1's move...

[truncated]

-------
|X| |O|
-------
|X|X|X|
-------
|O| |O|
-------
WINNER: PLAYER 1
```

## Board
![board1](https://cdn.discordapp.com/attachments/1015362506295877767/1052371288469020672/20221213_154638.jpg)
![board2](https://cdn.discordapp.com/attachments/260693013192376324/1055629752439930950/20221222_153416.jpg)
![board3](https://cdn.discordapp.com/attachments/260693013192376324/1055629752720953364/20221222_153501.jpg)

