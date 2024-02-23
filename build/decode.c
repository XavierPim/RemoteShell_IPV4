#include "methods.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {

    numberStruct* ptr =  malloc(sizeof(numberStruct));
    initializer(*ptr);
    int isNegative = 0;


// Check if there are enough arguments
    if (argc < 2) {
        printf("FORMAT with words: %s <string1> <string2> ...\n", argv[0]);
        return 1;
    }
    // Loop through the remaining command-line arguments
    for (int i = 1; i < argc; i++) {
        char *token = strtok(argv[i], " ");
        if (strcmp(token, "negative") == 0) {
            isNegative = 1;
        } else if((strcmp(token, "zero") == 0)){
            printf("0\n");
            exit(1);
        }
        word2Num(token, ptr);
        // Keep calling strtok until it returns NULL, which means no more tokens
        while (token != NULL) {
            token = strtok(NULL, " "); // Pass NULL to continue tokenization
        }
    }
    if(isNegative==1){
        printf("-%d \n", ptr->total);
    }else {
        printf("%d \n", ptr->total);
    }
    if (ptr->total<= INT32_MIN || ptr->total >= INT32_MAX){
        printf("is not within the range of int32_t.\n");
    }

    memset(ptr, 0, sizeof(numberStruct));
}
