#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "methods.h"

#define NEGAWUT "negative"

//Word Dictionary
const char *ones[] = {"", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
                      "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen",
                      "nineteen"};
const int onesSize = sizeof(ones) / sizeof(ones[0]);
const char *tens[] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
const int tenSize = sizeof(tens) / sizeof(tens[0]);
const char *upperLimits[] = {"hundred", "thousand", "million", "billion"};
const int upperSize = sizeof(upperLimits) / sizeof(upperLimits[0]);


//Local Methods
void wordAppender(numberStruct parsedStruct, wordStruct stored);

/**
 * Definition of PlaceValueExtractor for initializing the numberStruct
 * PRE:int32_inputInt input ensure valid int form +/- 2^32 integers
 * POST: fully initialized struct variables containing place values
 */

numberStruct PlaceValueExtractor(int32_t inputInt) {
    numberStruct result;
    initializer(result);

    // Determine the number of digits in the integer
    int temp = inputInt;
    int count = 0;

    // Calculate the number of digits - as suggested
    while (temp != 0) {
        temp /= 10;
        count++;
    }

    // Allocate memory for the digits array
    result.digits = (int *) malloc((count + 1) * sizeof(int));

    // Parse the digits into the array
//    printf("\n");//DO NOT DELETE NO IDEA WHY THIS FIXES IT
    temp = inputInt;
    for (int i = 1; i <= count; i++) {
        result.digits[i] = temp % 10;
//        printf("struct: %d temp: %d\n",result.digits[i],temp % 10);
        temp /= 10;
        switch (i) {
            case 1:
//                printf("index 1 ones: %d \n", result.digits[i]);
                result.ones = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 2:
//                printf("index 2 tens: %d \n", result.digits[i]);
                result.tens = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 3:
//                printf("index 3 hundreds: %d \n", result.digits[i]);
                result.hundreds = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 4:
//                printf("index 4 ones_thousands: %d \n", result.digits[i]);
                result.ones_thousands = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 5:
//                printf("index 5 tens_thousands: %d \n", result.digits[i]);
                result.tens_thousands = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 6:
//                printf("index 6 hundreds_thousands: %d \n", result.digits[i]);
                result.hundreds_thousands = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 7:
//                printf("index 7 ones_millions: %d \n", result.digits[i]);
                result.ones_millions = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 8:
//                printf("index 8 tens_millions: %d \n", result.digits[i]);
                result.tens_millions = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 9:
//                printf("index 9 hundreds_millions: %d \n", result.digits[i]);
                result.hundreds_millions = result.digits[i];
                result.numDigits = count + 1;
                break;
            case 10:
//                printf("index 10 ones_billions: %d \n", result.digits[i]);
                result.ones_billions = result.digits[i];
                result.numDigits = count + 1;
                break;
            default:
                result.numDigits = count + 1;
                break;
        }
    }
    result.numDigits = count;

//         TEST place value test
//    printf("ones : %s\n",  ones[result.digits[1]]);
//    printf("tens : %s\n",  tens[result.digits[2]]);
//    printf("hundreds : %s %s\n", ones[result.digits[3]], upperLimits[0]);
//    printf("ones_thousands : %s %s\n", ones[result.digits[4]], upperLimits[1]);
//    printf("tens_thousand : %s\n",  ones[result.digits[5]]);
//    printf("hundreds_thousands : %s\n",  tens[result.digits[6]]);
//    printf("ones_millions  : %s %s\n", ones[result.digits[7]], upperLimits[2]);
//    printf("tens_millions : %s\n",  ones[result.digits[8]]);
//    printf("hundreds_millions: %s\n",  tens[result.digits[9]]);
//    printf("ones_billions: %s %s\n", ones[result.digits[10]], upperLimits[3]);
//    printf("There are: %d digits\n", result.numDigits);

    return result;
}



wordStruct num2Words(numberStruct parsedStruct, int isNegative) {
    wordStruct stored;
    stored.str = strdup(""); // Initialize with an empty string
    stored.numWords = 0;

    if (isNegative == 1) {
        append(&stored, NEGAWUT);
    }
//    printf("number of digits: %d",parsedStruct.numDigits);
    wordAppender(parsedStruct, stored);
    return stored;
}

//cleans memory
void freeWordStruct(wordStruct *words) {
    if (words->str != NULL) {
        free(words->str);
        words->str = NULL;
    }
    words->numWords = 0;
}


//Appends the words according to the digits from the stored struct
void wordAppender(numberStruct parsedStruct, wordStruct stored) {
    int skipNextIteration = 0; // Initialize the flag

    for (int i = parsedStruct.numDigits; 0 < i; i--) {
        if (skipNextIteration) {
            skipNextIteration = 0; // Reset the flag
            continue; // Skip the current iteration
        }
        switch (i) {
            case 1://ones
                append(&stored, ones[parsedStruct.digits[1]]);
                break;
            case 2://teens to tens
                if (parsedStruct.digits[2] == 1) {
                    append(&stored, ones[parsedStruct.digits[1] + 10]);
                    skipNextIteration = 1;
                }
                if (parsedStruct.digits[2] != 0) {
                    append(&stored, tens[parsedStruct.digits[2]]);
                }
                break;
            case 3://hundreds
                append(&stored, ones[parsedStruct.digits[3]]);
                if (parsedStruct.digits[3] != 0) {
                    append(&stored, upperLimits[0]);
                }
                break;
            case 4://thousands
                append(&stored, ones[parsedStruct.digits[4]]);
                if (parsedStruct.digits[4] != 0) {
                    append(&stored, upperLimits[1]);
                }
                break;
            case 5://tens_thousands
                if (parsedStruct.digits[5] != 0) {
                    append(&stored, tens[parsedStruct.digits[5]]);

                    if (parsedStruct.digits[5] == 1) {
                        append(&stored, ones[parsedStruct.digits[4] + 10]);
                        append(&stored, upperLimits[1]);
                        skipNextIteration = 1;
                    }
                }
                break;
            case 6://hundred_thousands
                append(&stored, ones[parsedStruct.digits[6]]);
                if (parsedStruct.digits[6] != 0) {
                    append(&stored, upperLimits[0]);
                }
                if (parsedStruct.digits[5] == 0 && parsedStruct.digits[4] == 0) {
                    append(&stored, upperLimits[1]);
                }
                break;
            case 7://millions
                append(&stored, ones[parsedStruct.digits[7]]);
                if (parsedStruct.digits[7] != 0) {
                    append(&stored, upperLimits[2]);
                }
                break;
            case 8://tens_millions
                if (parsedStruct.digits[8] == 1) {
                    append(&stored, ones[parsedStruct.digits[7] + 10]);
                    append(&stored, upperLimits[2]);
                    skipNextIteration = 1;
                }
                if (parsedStruct.digits[8] != 0) {
                    append(&stored, tens[parsedStruct.digits[8]]);
                }
                break;
            case 9://hundred_millions
                if (parsedStruct.digits[9] != 0) {
                    append(&stored, ones[parsedStruct.digits[9]]);
                    append(&stored, upperLimits[0]);

                    if (parsedStruct.digits[8] == 0 && parsedStruct.digits[7] == 0) {
                        append(&stored, upperLimits[2]);
                    }
                }
                break;
            case 10://billions
                append(&stored, ones[parsedStruct.digits[10]]);
                if (parsedStruct.digits[10] != 0) {
                    append(&stored, upperLimits[3]);
                }
                break;
            default:
                break;
        }
    }
}

//custom append tool to allocate, connect, and concat the strings together
void append(wordStruct *stored, const char *text) {
    // Calculate the new length of the string after appending
    size_t newLength = strlen(stored->str) + strlen(text) + 2; // +2 for the space and null terminator

    // Allocate or reallocate memory for the string
    stored->str = (char *) realloc(stored->str, newLength);

    // Append a space if the existing string is not empty
    if (stored->numWords > 0) {
        strcat(stored->str, " ");
    }

    // Concatenate the new text to the existing string
    strcat(stored->str, text);

    // Update the word count
    stored->numWords++;
}


//initializes the numberStruct
numberStruct initializer(numberStruct new) {
    new.ones = 0;
    new.tens = 0;
    new.hundreds = 0;
    new.ones_thousands = 0;
    new.tens_thousands = 0;
    new.hundreds_thousands = 0;
    new.ones_millions = 0;
    new.tens_millions = 0;
    new.hundreds_millions = 0;
    new.ones_billions = 0;
    return new;
}

//removes extra spaces from the concatenated string
void removeExtraSpaces(char *str) {
    int i, j;
    int len = strlen(str);
    int isSpace = 0; // Flag to track consecutive spaces

    for (i = 0, j = 0; i < len; i++) {
        if (str[i] != ' ') {
            // Copy non-space characters to the new position
            str[j++] = str[i];
            isSpace = 0; // Reset the consecutive spaces flag
        } else {
            // Skip consecutive spaces
            if (!isSpace) {
                str[j++] = ' '; // Copy a single space
                isSpace = 1; // Set the consecutive spaces flag
            }
        }
    }
    // Null-terminate the string
    str[j] = '\0';
}


/**
 * Definition of word2Num
 * PRE:Take in string token
 * POST: sum of all the numbers
 */
void word2Num(char *token, numberStruct *ptr) {
    for (int i = 0; i < onesSize; ++i) {
        if (strcmp(token, ones[i]) == 0) {
            ptr->total += i;
            ptr->lastToken = i;
//            printf("six last: %d lastLast: %d i: %d total: %d \n", ptr->lastToken, ptr->lastLastToken, i,ptr->total);
        }
    }
    for (int i = 0; i < tenSize; ++i) {
        if (strcmp(token, tens[i]) == 0) {
            ptr->total += i * 10;
            ptr->lastToken = i * 10;
            ptr->lastLastToken = i * 10;
//            printf("last: %d lastLast: %d i: %d total: %d \n", ptr->lastToken, ptr->lastLastToken, i, ptr->total);
        }
    }

    for (int i = 0; i < upperSize; ++i) {
        if (strcmp(token, upperLimits[i]) == 0) {

            switch (i) {
                case 0:
                    ptr->total += (ptr->lastToken * 100) - ptr->lastToken;
                    ptr->lastLastToken = ptr->lastToken;
                    ptr->lastLastToken = 0;
                    break;

                case 1:
                    if (ptr->lastLastToken >= 20 && ptr->lastLastToken <= 90) {
                        ptr->total +=
                                ((ptr->lastToken + ptr->lastLastToken) * 1000) - (ptr->lastToken + ptr->lastLastToken);
                    } else {
                        ptr->total += (ptr->lastToken * 1000) - ptr->lastToken;
                    }
                    ptr->lastLastToken = ptr->lastToken;
                    ptr->lastToken = 0;
//                        }
                    break;

                case 2:
//                    printf("before last: %d lastLast: %d i: %d total: %d \n", ptr->lastToken, ptr->lastLastToken,i,ptr->total);
                    if (ptr->lastLastToken >= 20 && ptr->lastLastToken <= 90) {
                        ptr->total += ((ptr->lastToken + ptr->lastLastToken) * 1000000) -
                                      (ptr->lastToken + ptr->lastLastToken);
//                        printf("after last: %d lastLast: %d i: %d total: %d \n", ptr->lastToken, ptr->lastLastToken, i, ptr->total);
                    } else {
                        ptr->total += (ptr->lastToken * 1000000) - ptr->lastToken;
                    }
                    ptr->lastLastToken = ptr->lastToken;
                    ptr->lastToken = 0;
                    break;

                case 3:
                    if (ptr->lastLastToken >= 20 && ptr->lastLastToken <= 90) {
                        ptr->total += ((ptr->lastToken + ptr->lastLastToken) * 1000000000) -
                                      (ptr->lastToken + ptr->lastLastToken);
                    } else {
                        ptr->total += (ptr->lastToken * 1000000000) - ptr->lastToken;
                    }
                    ptr->lastLastToken = ptr->lastToken;
                    ptr->lastToken = 0;
                    break;

                default:
                    break;
            }
        }
    }

}
