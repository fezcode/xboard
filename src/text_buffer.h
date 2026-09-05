#ifndef TEXT_BUFFER_H
#define TEXT_BUFFER_H
#include <stdbool.h>
#define MAX_TEXT_LEN 4096
#define HISTORY_DEPTH 32
typedef struct { char text[MAX_TEXT_LEN]; int len, cursor; bool selected; } TextSnapshot;
typedef struct {
    TextSnapshot current;
    TextSnapshot undo[HISTORY_DEPTH], redo[HISTORY_DEPTH];
    int undo_count, redo_count;
} TextBuffer;
bool text_insert(TextBuffer* b, const char* text);
void text_backspace(TextBuffer* b, bool word);
void text_clear(TextBuffer* b);
bool text_undo(TextBuffer* b);
bool text_redo(TextBuffer* b);
void text_move(TextBuffer* b, int direction);
#endif
