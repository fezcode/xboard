#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "app_state.h"
#include <SDL.h>

void controller_init(void);
void controller_shutdown(void);
void controller_handle_event(AppState* state, const SDL_Event* ev);
void controller_update(AppState* state, float dt);
void controller_rumble(float low_motor, float high_motor, uint32_t duration_ms);

#endif /* CONTROLLER_H */
