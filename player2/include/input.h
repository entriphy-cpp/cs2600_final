// Prompt the user for an integer
int integerInput(char *text);

// Prompt the user for a positive integer
int integerInputPositive(char *text, int allowZero);

// Prompt the user for an integer between min and max
int integerInputRange(char *text, int min, int max);

// Prompt the user for a decimal number
double doubleInput(char *text);

// Prompt the user for a positive decimal number
double doubleInputPositive(char *text, int allowZero);

// Prompt the user for a decimal number between min and max
double doubleInputRange(char *text, double min, double max);

// Prompt the user for a Y/N response
int booleanInput(char *text);