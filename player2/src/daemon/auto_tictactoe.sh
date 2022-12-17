#!/bin/bash

# Opcodes (defined in opcodes.h)
PRINT=00
PLAYER_1_ASK=01
PLAYER_2_CONFIRM=02
PLAYER_2_DENY=03
PLAYER_1_TIMEOUT=04
PLAYER_1_INIT=05
PLAYER_1_MOVE=06
PLAYER_2_MOVE=07
PLAYER_1_QUIT=08
PLAYER_2_QUIT=09
PLAYER_1_WIN=0A
PLAYER_2_WIN=0B

# MQTT settings
P1_TOPIC="cs2600-esp32-player1"
P2_TOPIC="cs2600-esp32-player2"
BROKER="broker.hivemq.com"

# Returns the opcode from the given payload
function payload_opcode() {
    payload_bytes="$1"
    opcode=${payload_bytes::1} # Get first byte
    echo -n "${opcode}" | xxd -p # Return opcode as decimal
}

# Sends a payload with the given opcode
function send_payload() {
    opcode="$1"
    msg=$(echo -n -e "\x${opcode}")
    mosquitto_pub -h "${BROKER}" -t "${P1_TOPIC}" -m "${msg}"
}

board=() # Holds what marks were made on the board

function bytes_to_hex() {
    echo -n "$1" | xxd -p
}

function make_move() {
    # Delay
    sleep 2

    # Select row/column
    row="$(($RANDOM % 3 + 1))"
    column="$(($RANDOM % 3 + 1))"
    coord="0${row}0${column}"
    marked=$(is_marked ${coord})

    # Check if coordinate is already marked and find a new coordinate if it is
    while is_marked $coord
    do
        row="$(($RANDOM % 3 + 1))"
        column="$(($RANDOM % 3 + 1))"
        coord="0${row}0${column}"
        marked=$(is_marked ${coord})
    done
    echo "${coord}"
    board+=("${coord}")
    
    # Send to player 1
    msg=$(echo -n -e "\x${PLAYER_2_MOVE}\x${row}\x${column}")
    mosquitto_pub -h "${BROKER}" -t "${P1_TOPIC}" -m "${msg}"
}

function is_marked() {
    if [ "${#distro[@]}" -eq "9" ]; then
        return 1 # Board is all filled
    fi
    for i in "${board[@]}"
    do
        if [ "$i" -eq "$1" ]; then
            return 0
        fi
    done
    return 1
}

while true
do 
    mosquitto_sub -h "${BROKER}" -t "${P2_TOPIC}" | while read -r payload
    do
        case $(payload_opcode "${payload}") in
            "${PLAYER_1_ASK}")
                sleep 3 # Delay
                send_payload "${PLAYER_2_CONFIRM}" # Send confirmation to play
                ;;
            "${PLAYER_1_INIT}")
                board=() # Clear array
                first_player=$(bytes_to_hex "${payload:1:1}") # Get which player goes first from op
                if [ "${first_player}" == "02" ]; then
                    make_move
                fi
                ;;
            "${PLAYER_1_MOVE}")
                # Add move to board and make next move
                row=$(bytes_to_hex "${payload:1:1}")
                column=$(bytes_to_hex "${payload:2:1}")
                coord="${row}${column}"
                board+=("${coord}")
                make_move # Win condition is checked on player 1's side and this will be ignored if game has already ended
                ;;
        esac
    done
    sleep 10
done