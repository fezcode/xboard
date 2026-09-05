#include "app_state.h"
#include "audio.h"
#include "controller.h"
#include "send_input.h"
#include "ui_draw.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void app_set_toast(AppState* s, const char* msg, float duration) {
    snprintf(s->toast, sizeof(s->toast), "%s", msg); s->toast_timer = duration;
}
static void feedback(AppState* s) {
    if (s->mode != MODE_DIAL) audio_play_click(s->audio);
    controller_rumble(.12f, .18f, 30);
}
void app_copy(AppState* s) {
    app_set_toast(s, copy_to_clipboard(s->editor.current.text) ? "Copied to clipboard" : "Clipboard unavailable. Try again.", 2);
}
void app_insert_string(AppState* s, const char* text) {
    if (!text_insert(&s->editor, text)) { app_set_toast(s, "Composer full. Copy or clear your text first.", 2); return; }
    if (s->direct_send_input) send_input_string(text);
    /* Dial mode is silent unless text was actually inserted. */
    if (s->mode == MODE_DIAL && text && *text) audio_play_insert_click(s->audio);
}
void app_paste(AppState* s) {
    char buf[MAX_TEXT_LEN];
    if (paste_from_clipboard(buf, sizeof(buf))) { app_insert_string(s, buf); feedback(s); }
    else app_set_toast(s, "Clipboard empty, unavailable, or text too large", 2);
}
void app_undo(AppState* s) {
    if (s->direct_send_input) { send_input_ctrl_combo('z'); app_set_toast(s, "Undo sent to active app", 2); return; }
    app_set_toast(s, text_undo(&s->editor) ? "Undone" : "Nothing to undo", 1.5f);
}
void app_redo(AppState* s) {
    if (s->direct_send_input) { send_input_ctrl_combo('y'); app_set_toast(s, "Redo sent to active app", 2); return; }
    app_set_toast(s, text_redo(&s->editor) ? "Redone" : "Nothing to redo", 1.5f);
}
void app_select_all(AppState* s) {
    s->editor.current.selected = s->editor.current.len > 0;
    if (s->direct_send_input) send_input_ctrl_combo('a');
}
void app_clear(AppState* s) { text_clear(&s->editor); app_set_toast(s, "Composer cleared. Undo to restore.", 2); }
void app_insert_char(AppState* s, char c) {
    char lower = (char)tolower((unsigned char)c);
    if (s->win_active) { if (s->direct_send_input) send_input_win_combo(lower); return; }
    if (s->ctrl_active) {
        if (s->direct_send_input) { send_input_ctrl_combo(lower); return; }
        switch (lower) {
            case 'c': app_copy(s); return;
            case 'v': app_paste(s); return;
            case 'z': app_undo(s); return;
            case 'y': app_redo(s); return;
            case 'a': app_select_all(s); return;
            case 'x': if (copy_to_clipboard(s->editor.current.text)) app_clear(s); return;
            default: return;
        }
    }
    char text[2] = {c, 0}; app_insert_string(s, text);
}
void app_backspace(AppState* s) {
    text_backspace(&s->editor, false); feedback(s);
    if (s->direct_send_input) send_input_backspace();
}
void app_delete_word(AppState* s) {
    text_backspace(&s->editor, true); feedback(s);
    if (s->direct_send_input) send_input_ctrl_combo('\b');
}
void app_space(AppState* s) { app_insert_char(s, ' '); feedback(s); }
void app_newline(AppState* s) { app_insert_char(s, '\n'); feedback(s); }
void app_cursor_left(AppState* s) { text_move(&s->editor, -1); if (s->direct_send_input) send_input_left(s->shift_active, s->ctrl_active); }
void app_cursor_right(AppState* s) { text_move(&s->editor, 1); if (s->direct_send_input) send_input_right(s->shift_active, s->ctrl_active); }
void app_cursor_up(AppState* s) { text_move(&s->editor, -2); if (s->direct_send_input) send_input_up(s->shift_active, s->ctrl_active); }
void app_cursor_down(AppState* s) { text_move(&s->editor, 2); if (s->direct_send_input) send_input_down(s->shift_active, s->ctrl_active); }
void app_win_key(AppState* s) {
    if (s->direct_send_input) send_input_win_key();
    else app_set_toast(s, "Enable Direct to apps to use the Windows key", 2);
}
void app_cycle_theme(AppState* s) {
    s->theme_index = (s->theme_index + 1) % THEME_COUNT; theme_set(s->theme_index);
}
void app_toggle_direct(AppState* s) {
    s->direct_send_input = !s->direct_send_input;
    s->ctrl_locked = s->win_locked = s->ctrl_active = s->win_active = false;
    s->editor.current.selected = false;
    app_set_toast(s, s->direct_send_input ? "Direct to apps enabled. Focus the destination window." : "Compose locally, then copy when ready", 2.5f);
}
static FILE* preferences(const char* mode) {
    char* dir = SDL_GetPrefPath("xboard", "xboard");
    if (!dir) return NULL;
    char path[1024]; snprintf(path, sizeof(path), "%spreferences.ini", dir); SDL_free(dir);
    return fopen(path, mode);
}
void app_load_preferences(AppState* s) {
    FILE* f = preferences("r"); if (!f) return;
    char line[128]; int value;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "theme=%d", &value) == 1 && value >= 0 && value < THEME_COUNT) s->theme_index = value;
        if (sscanf(line, "mode=%d", &value) == 1 && value >= 0 && value < MODE_COUNT) s->mode = (AppMode)value;
        if (sscanf(line, "sound=%d", &value) == 1 && (value == 0 || value == 1)) s->sound_enabled = value != 0;
    }
    fclose(f); theme_set(s->theme_index);
}
void app_save_preferences(AppState* s) {
    FILE* f = preferences("w"); if (!f) { SDL_Log("Could not save preferences"); return; }
    fprintf(f, "theme=%d\nmode=%d\nsound=%d\n", s->theme_index, s->mode, s->sound_enabled); fclose(f);
}
