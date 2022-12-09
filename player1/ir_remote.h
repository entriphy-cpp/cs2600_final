// From Chapter 23
enum IRKeys {
  IR_POWER = 0xFFA25D,
  IR_MENU  = 0xFFE21D,
  IR_TEST  = 0xFF22DD,
  IR_PLUS  = 0xFF02FD,
  IR_BACK  = 0xFFC23D,
  IR_PREV  = 0xFFE01F,
  IR_PLAY  = 0xFFA857,
  IR_NEXT  = 0xFF906F,
  IR_0     = 0xFF6897,
  IR_MINUS = 0xFF9867,
  IR_C     = 0xFFB04F,
  IR_1     = 0xFF30CF,
  IR_2     = 0xFF18E7,
  IR_3     = 0xFF7A85,
  IR_4     = 0xFF10EF,
  IR_5     = 0xFF38C7,
  IR_6     = 0xFF5AA5,
  IR_7     = 0xFF42BD,
  IR_8     = 0xFF4AB5,
  IR_9     = 0xFF52AD
};

int keyToInt(int key) {
  switch (key) {
    case IR_0:
      return 0;
    case IR_1:
      return 1;
    case IR_2:
      return 2;
    case IR_3:
      return 3;
    case IR_4:
      return 4;
    case IR_5:
      return 5;
    case IR_6:
      return 6;
    case IR_7:
      return 7;
    case IR_8:
      return 8;
    case IR_9:
      return 9;
    default:
      return -1;
  }
}