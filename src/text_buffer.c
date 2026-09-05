#include "text_buffer.h"
#include <string.h>
#include <ctype.h>

static int previous(const char* s, int p) {
    if (p > 0) --p;
    while (p > 0 && ((unsigned char)s[p] & 0xc0) == 0x80) --p;
    return p;
}
static int next(const char* s, int p, int len) {
    if (p < len) ++p;
    while (p < len && ((unsigned char)s[p] & 0xc0) == 0x80) ++p;
    return p;
}
static void push(TextSnapshot* stack, int* count, TextSnapshot snapshot) {
    if (*count == HISTORY_DEPTH) {
        memmove(stack, stack + 1, sizeof(*stack) * (HISTORY_DEPTH - 1));
        --*count;
    }
    stack[(*count)++] = snapshot;
}
static void checkpoint(TextBuffer* b) {
    push(b->undo, &b->undo_count, b->current);
    b->redo_count = 0;
}
bool text_insert(TextBuffer* b, const char* text) {
    if (!text || !*text) return true;
    TextSnapshot* s = &b->current;
    size_t n = strlen(text);
    if (n >= MAX_TEXT_LEN || n + (s->selected ? 0 : s->len) >= MAX_TEXT_LEN) return false;
    checkpoint(b);
    if (s->selected) { s->len = s->cursor = 0; s->text[0] = 0; }
    memmove(s->text + s->cursor + n, s->text + s->cursor, (size_t)(s->len - s->cursor + 1));
    memcpy(s->text + s->cursor, text, n);
    s->cursor += (int)n; s->len += (int)n; s->selected = false;
    return true;
}
void text_clear(TextBuffer* b) {
    if (!b->current.len) return;
    checkpoint(b);
    memset(&b->current, 0, sizeof(b->current));
}
void text_backspace(TextBuffer* b, bool word) {
    TextSnapshot* s = &b->current;
    if (s->selected) { text_clear(b); return; }
    if (!s->cursor) return;
    checkpoint(b);
    int start = previous(s->text, s->cursor);
    if (word) {
        start = s->cursor;
        while (start && isspace((unsigned char)s->text[start - 1])) start = previous(s->text, start);
        while (start && !isspace((unsigned char)s->text[start - 1])) start = previous(s->text, start);
    }
    memmove(s->text + start, s->text + s->cursor, (size_t)(s->len - s->cursor + 1));
    s->len -= s->cursor - start; s->cursor = start;
}
bool text_undo(TextBuffer* b) {
    if (!b->undo_count) return false;
    push(b->redo, &b->redo_count, b->current);
    b->current = b->undo[--b->undo_count]; return true;
}
bool text_redo(TextBuffer* b) {
    if (!b->redo_count) return false;
    push(b->undo, &b->undo_count, b->current);
    b->current = b->redo[--b->redo_count]; return true;
}
void text_move(TextBuffer* b, int direction) {
    TextSnapshot* s = &b->current;
    if (direction == -1) s->cursor = s->selected ? 0 : previous(s->text, s->cursor);
    else if (direction == 1) s->cursor = s->selected ? s->len : next(s->text, s->cursor, s->len);
    else {
        int start = s->cursor;
        while (start > 0 && s->text[start - 1] != '\n') --start;
        int column = 0;
        for (int p = start; p < s->cursor; p = next(s->text, p, s->len)) ++column;
        int target = start;
        if (direction == -2) {
            if (!start) { s->cursor = 0; s->selected = false; return; }
            target = start - 1;
            while (target > 0 && s->text[target - 1] != '\n') --target;
        } else {
            while (target < s->len && s->text[target] != '\n') ++target;
            if (target < s->len) ++target;
        }
        while (column-- > 0 && target < s->len && s->text[target] != '\n') target = next(s->text, target, s->len);
        s->cursor = target;
    }
    s->selected = false;
}
