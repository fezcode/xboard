#ifndef APP_SHELL_H
#define APP_SHELL_H
#include "app_state.h"
bool shell_controller_update(AppState* s, float dt);
/* Mode-independent controller actions: mode switching, output target, clipboard,
 * modifiers, cursor movement and the universal space/backspace/enter buttons. */
void shell_global_shortcuts(AppState* s, float dt, bool mode_input_blocked);
void shell_update(AppState* s);
/* Position of one quick-phrase button. Drawing and hit testing share this so
 * the click targets can never drift from what is on screen. */
void shell_phrase_rect(AppState* s, int index, int* out_x, int* out_w);
void shell_draw_header(AppState* s);
void shell_draw_composer(AppState* s);
void shell_draw_footer(AppState* s);
void shell_draw_overlay(AppState* s);
#endif
