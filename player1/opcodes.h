enum Opcodes {
    PRINT,            // Debugging purposes
    PLAYER_1_ASK,     // p1 -> p2: Ask Player 2 if they want to play
    PLAYER_2_CONFIRM, // p2 -> p1: Player 2 confirms to Player 1 that they want to play
    PLAYER_2_DENY,    // p2 -> p1: Player 2 does not want to play
    PLAYER_1_TIMEOUT, // p1 -> p2: Play request timeout
    PLAYER_1_INIT,    // p1 -> p2: Send initial game data to Player 2
    PLAYER_1_MOVE,    // p1 -> p2: Send row and column
    PLAYER_2_MOVE,    // p2 -> p2: Send row and column
    PLAYER_1_QUIT,    // p1 -> p2: Player 1 quits (pressed power button)
    PLAYER_2_QUIT     // p2 -> p1: Player 2 quits (pressed Ctrl+C)
};

typedef struct __attribute__((__packed__)) {
    char op;
    char first_player;
} InitGame, *pInitGame;

typedef struct __attribute__((__packed__)) {
    char op;
    char row;
    char column;
} PlayerMove, *pPlayerMove;

