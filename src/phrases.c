#include "phrases.h"
#include <stdio.h>
#include <string.h>

/* Used until the preferences file supplies a set of its own. */
static const char* default_phrases[] = {"Hello! ", "Thank you. ", "One moment, please. ", "Yes", "No"};
#define DEFAULT_PHRASE_COUNT ((int)(sizeof(default_phrases) / sizeof(default_phrases[0])))

int phrase_count(const AppState* s) {
    return s->phrases.count > 0 ? s->phrases.count : DEFAULT_PHRASE_COUNT;
}

const char* phrase_text(const AppState* s, int index) {
    if (index < 0 || index >= phrase_count(s)) return "";
    return s->phrases.count > 0 ? s->phrases.items[index] : default_phrases[index];
}

/* Cut back to the last complete UTF-8 code point. */
static void trim_partial_utf8(char* text) {
    size_t len = strlen(text), lead = len;
    while (lead > 0 && ((unsigned char)text[lead - 1] & 0xc0) == 0x80) --lead;
    if (lead == 0) { text[0] = '\0'; return; }
    unsigned char first = (unsigned char)text[lead - 1];
    if (first < 0x80) return;
    size_t needed = first < 0xe0 ? 2 : first < 0xf0 ? 3 : 4;
    if (len - (lead - 1) < needed) text[lead - 1] = '\0';
}

bool phrase_append(AppState* s, const char* text) {
    if (!text || !*text || s->phrases.count >= MAX_PHRASES) return false;
    char* slot = s->phrases.items[s->phrases.count];
    snprintf(slot, MAX_PHRASE_LEN, "%s", text);
    trim_partial_utf8(slot);
    if (!*slot) return false;
    ++s->phrases.count;
    return true;
}
