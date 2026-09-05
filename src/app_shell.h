#ifndef APP_SHELL_H
#define APP_SHELL_H
#include "app_state.h"
bool shell_controller_update(AppState* s, float dt);
void shell_update(AppState* s);
void shell_draw_header(AppState* s);
void shell_draw_composer(AppState* s);
void shell_draw_footer(AppState* s);
void shell_draw_overlay(AppState* s);
#endif
