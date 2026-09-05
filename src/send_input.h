#ifndef SEND_INPUT_H
#define SEND_INPUT_H

#include <stdbool.h>
#include <stddef.h>

void send_input_char(char c);
void send_input_ctrl_combo(char c);
void send_input_win_key(void);
void send_input_win_combo(char c);
void send_input_win_down(void);
void send_input_win_up(void);
void send_input_backspace(void);
void send_input_enter(void);
void send_input_space(void);
void send_input_left(bool shift, bool ctrl);
void send_input_right(bool shift, bool ctrl);
void send_input_up(bool shift, bool ctrl);
void send_input_down(bool shift, bool ctrl);
void send_input_string(const char* str);
void send_input_set_target(void* hwnd, void* self_hwnd);
void send_input_ensure_target(void);
bool copy_to_clipboard(const char* text);
bool paste_from_clipboard(char* out_buf, size_t max_len);

#endif /* SEND_INPUT_H */
