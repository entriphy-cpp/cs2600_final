enum Opcodes {
    PRINT,            // Debugging purposes
    PLAYER_1_ASK,     // p1 -> p2: Ask Player 2 if they want to play
    PLAYER_2_CONFIRM, // p2 -> p1: Player 2 confirms to Player 1 that they want to play
    PLAYER_2_DENY,    // p2 -> p1: Player 2 does not want to play
    PLAYER_1_TIMEOUT, // p1 -> p2: Play request timeout
    PLAYER_1_INIT     // p1 -> p2: Send initial game data to Player 2
};

typedef struct __attribute__((__packed__)) {
    char op;
    char first_player;
} InitGame;

typedef struct __attribute__((__packed__)) {
    char op;
    char x;
    char y;
} PlayerMoveOp;

