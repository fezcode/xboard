#ifndef MORSE_H
#define MORSE_H

#include <stdbool.h>

typedef struct {
    char ch;
    const char* code;
} MorseEntry;

char morse_decode(const char* code);
const char* morse_encode(char ch);
const MorseEntry* morse_get_table(int* out_count);

#endif /* MORSE_H */
