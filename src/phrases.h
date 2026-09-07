#ifndef PHRASES_H
#define PHRASES_H
#include "app_state.h"

/* Quick-phrase contents. Kept apart from the shell so preferences can read and
 * write phrases without depending on rendering or audio. */
int phrase_count(const AppState* s);
const char* phrase_text(const AppState* s, int index);

/* Adds one configured phrase, replacing the built-in set. Text longer than a
 * phrase slot is truncated without leaving a partial UTF-8 sequence behind,
 * which would otherwise render as nothing at all. Returns false when the list
 * is full or the text is empty. */
bool phrase_append(AppState* s, const char* text);

#endif /* PHRASES_H */
