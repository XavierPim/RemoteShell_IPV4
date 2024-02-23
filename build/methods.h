#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#pragma once


extern const char *ones[];
extern const char *tens[];
extern const char *upperLimits[];


//Data Structures
typedef struct {
  int* digits;
  int ones;
  int tens;
  int hundreds;
  int ones_thousands;
  int tens_thousands;
  int hundreds_thousands;
  int ones_millions;
  int tens_millions;
  int hundreds_millions;
  int32_t ones_billions;
  int32_t numDigits;
  int32_t total;
  int lastToken;
  int lastLastToken;
} numberStruct;

typedef struct {
    char *str;
    int numWords;
} wordStruct;


//Input Methods
numberStruct PlaceValueExtractor(int32_t inputInt);//encode
char *concatenateArgs(int argc, char *argv[]);

//Output Methods
wordStruct num2Words(numberStruct parsedStruct, int isNegative);//encode
void word2Num(char *token, numberStruct *ptr);//decode


//Processors
void freeWordStruct(wordStruct* words);
void removeExtraSpaces(char *str);
void append(wordStruct *stored, const char *text);
numberStruct initializer(numberStruct new);
