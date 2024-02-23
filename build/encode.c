#include <stdio.h>
#include <stdlib.h>
#include "methods.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#define ZERO "zero"


int main(int argc, char *argv[]) {
    //error detection if empty arguments
    if (argc != 2) {
        printf("FORMAT Enter: %s <number>\n", argv[0]);
        return 1;
    }
    //grabs CLI argument
    char *arg1 = argv[1];
    char *endptr; // Pointer to the character that ends the conversion
    long inputLong = strtol(arg1, &endptr, 10);

    // Check for conversion errors
    if ((errno == ERANGE && (inputLong == LONG_MAX || inputLong == LONG_MIN)) || (errno != 0 && inputLong == 0)) {
        perror("strtol");
        return 1;
    }

    // Check for trailing non-numeric characters
    if (*endptr != '\0') {
        printf("Invalid input: %s\n", arg1);
        return 1;
    }

    // Convert the input string to an integer
    if (inputLong >= INT32_MIN && inputLong <= INT32_MAX) {

        //if conversion is successful
        int32_t inputInt = (int32_t) inputLong;

        //checks if negative
        int isNegativeFlag = 0;
        if (inputInt < 0) {
            inputInt = abs(inputInt);
            isNegativeFlag = 1;
        } else if (inputInt == 0) {
            printf("%s", ZERO);
        }

        // Passes input into place value struct
        numberStruct parsedInput = PlaceValueExtractor(inputInt);

        //replaces stored digits into words and concatenating into one long string
        wordStruct storedWords = num2Words(parsedInput, isNegativeFlag);

        //removes extra spaces from the processing
        removeExtraSpaces(storedWords.str);

        //prints output
        printf("%s\n", storedWords.str);

        //free the allocated memory
        memset(&parsedInput, 0, sizeof(numberStruct));
        freeWordStruct(&storedWords);
    } else {
        printf("%ld is not within the range of int32_t.\n", inputLong);
        return 1;
    }
}
