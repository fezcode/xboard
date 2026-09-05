#include "morse.h"
#include <string.h>
#include <ctype.h>

static const MorseEntry MORSE_TABLE[] = {
    { 'A', ".-" },
    { 'B', "-..." },
    { 'C', "-.-." },
    { 'D', "-.." },
    { 'E', "." },
    { 'F', "..-." },
    { 'G', "--." },
    { 'H', "...." },
    { 'I', ".." },
    { 'J', ".---" },
    { 'K', "-.-" },
    { 'L', ".-.." },
    { 'M', "--" },
    { 'N', "-." },
    { 'O', "---" },
    { 'P', ".--." },
    { 'Q', "--.-" },
    { 'R', ".-." },
    { 'S', "..." },
    { 'T', "-" },
    { 'U', "..-" },
    { 'V', "...-" },
    { 'W', ".--" },
    { 'X', "-..-" },
    { 'Y', "-.--" },
    { 'Z', "--.." },
    { '1', ".----" },
    { '2', "..---" },
    { '3', "...--" },
    { '4', "....-" },
    { '5', "....." },
    { '6', "-...." },
    { '7', "--..." },
    { '8', "---.." },
    { '9', "----." },
    { '0', "-----" },
    { '.', ".-.-.-" },
    { ',', "--..--" },
    { '?', "..--.." },
    { '/', "-..-." },
    { '!', "-.-.--" },
    { '-', "-....-" },
    { '@', ".--.-." },
};

static const int TABLE_COUNT = sizeof(MORSE_TABLE) / sizeof(MORSE_TABLE[0]);

char morse_decode(const char* code) {
    if (!code || code[0] == '\0') return '\0';
    for (int i = 0; i < TABLE_COUNT; ++i) {
        if (strcmp(MORSE_TABLE[i].code, code) == 0) {
            return MORSE_TABLE[i].ch;
        }
    }
    return '\0';
}

const char* morse_encode(char ch) {
    ch = (char)toupper((unsigned char)ch);
    for (int i = 0; i < TABLE_COUNT; ++i) {
        if (MORSE_TABLE[i].ch == ch) {
            return MORSE_TABLE[i].code;
        }
    }
    return "";
}

const MorseEntry* morse_get_table(int* out_count) {
    if (out_count) *out_count = TABLE_COUNT;
    return MORSE_TABLE;
}
