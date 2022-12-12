#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "input.h"

void clearBuffer() {
    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n'); // Clears buffer to prevent printf output from looping into scanf
}

int integerInput(char *text) {
    int input = 0;
    int valid = 0;

    do { // Loop until a valid input is entered
        printf(text);
        valid = scanf("%d", &input); // Read option from user
        if (!valid) { 
            printf("Invalid number.\n"); // User entered an invalid number
        }
        clearBuffer();
    } while (!valid);

    return input;
}

int integerInputPositive(char *text, int allowZero) {
    int input = 0;

    do { // Loop until a positive number is entered
        input = integerInput(text); // Read option from user
        if (input < (allowZero ? 0 : 1)) {
            // User's input was not positive
            printf(allowZero ? "Invalid number (must be positive).\n" : "Invalid number (must be positive, nonzero).\n");
        }
    } while (input < (allowZero ? 0 : 1));

    return input;
}

int integerInputRange(char *text, int min, int max) {
    int input = 0;

    do { // Loop until a number between min and max is entered
        input = integerInput(text); // Read option from user
        if (input < min || input > max) {
            // User's input was not in range
            printf("Invalid number (must be between %d and %d).\n", min, max);
        }
    } while (input < min || input > max);

    return input;
}

double doubleInput(char *text) {
    double input = 0;
    int valid = 0;

    do { // Loop until a valid input is entered
        printf(text);
        valid = scanf("%lf", &input); // Read option from user
        if (!valid) { 
            printf("Invalid number.\n"); // User entered an invalid double
        }
        clearBuffer();
    } while (!valid);

    return input;
}

double doubleInputPositive(char *text, int allowZero) {
    double input = 0;

    do { // Loop until a positive number is entered
        input = doubleInput(text); // Read option from user
        if (input < (allowZero ? 0 : 1)) {
            // User's input was not positive
            printf(allowZero ? "Invalid number (must be positive).\n" : "Invalid number (must be positive, nonzero).\n");
        }
    } while (input < (allowZero ? 0 : 1));

    return input;
}

double doubleInputRange(char *text, double min, double max) {
    double input = 0;

    do { // Loop until a number between min and max is entered
        input = doubleInput(text); // Read option from user
        if (input < min || input > max) {
            // User's input was not in range
            printf("Invalid number (must be between %.2f and %.2f).\n", min, max);
        }
    } while (input < min || input > max);

    return input;
}

int booleanInput(char *text) {
    char input = 0;

    do { // Loop until a valid input is entered
        printf(text);
        int scan = scanf("%c", &input); // Read option from user
        if (!scan || (input != 'y' && input != 'Y' && input != 'n' && input != 'N')) {
            printf("Invalid y/n response.\n");
        }
        clearBuffer();
    } while (input != 'y' && input != 'Y' && input != 'n' && input != 'N');

    return input == 'y' || input == 'Y';
}