#include "controller.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <xinput.h>
static int g_xinput_slot = -1;
static float g_rumble_timer = 0.0f;
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static SDL_GameController* g_controller = NULL;
static int g_controller_index = -1;

#define STICK_DEADZONE 0.22f
#define TRIGGER_DEADZONE 0.15f

static void find_xinput_slot(AppState* state) {
#ifdef _WIN32
    for (DWORD i = 0; i < 4; ++i) {
        XINPUT_STATE xs;
        if (XInputGetState(i, &xs) == ERROR_SUCCESS) {
            if (g_xinput_slot != (int)i) {
                g_xinput_slot = (int)i;
                state->ctrl.connected = true;
                snprintf(state->ctrl.name, sizeof(state->ctrl.name), "Xbox Controller (Slot %lu)", i);
                char msg[128];
                snprintf(msg, sizeof(msg), "Connected: %s", state->ctrl.name);
                app_set_toast(state, msg, 2.5f);
                printf("[Controller] XInput controller found on slot %lu\n", i);
            }
            return;
        }
    }
    g_xinput_slot = -1;
#endif
}

static void open_first_available_controller(AppState* state) {
    if (g_controller) return;

    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; ++i) {
        if (SDL_IsGameController(i)) {
            g_controller = SDL_GameControllerOpen(i);
            if (g_controller) {
                g_controller_index = i;
                state->ctrl.connected = true;
                const char* name = SDL_GameControllerName(g_controller);
                snprintf(state->ctrl.name, sizeof(state->ctrl.name), "%s", name ? name : "Xbox Controller");
                char msg[128];
                snprintf(msg, sizeof(msg), "Connected: %s", state->ctrl.name);
                app_set_toast(state, msg, 2.5f);
                printf("[Controller] Opened: %s (index %d)\n", state->ctrl.name, i);
                return;
            }
        }
    }
}

void controller_init(void) {
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "[Controller] Failed to init GameController: %s\n", SDL_GetError());
    }
}

void controller_shutdown(void) {
#ifdef _WIN32
    if (g_xinput_slot >= 0) {
        XINPUT_VIBRATION zero_vib = { 0, 0 };
        XInputSetState((DWORD)g_xinput_slot, &zero_vib);
        g_xinput_slot = -1;
    }
#endif
    if (g_controller) {
        SDL_GameControllerClose(g_controller);
        g_controller = NULL;
        g_controller_index = -1;
    }
}

void controller_rumble(float low_motor, float high_motor, uint32_t duration_ms) {
#ifdef _WIN32
    if (g_xinput_slot >= 0) {
        if (low_motor > 1.0f) low_motor = 1.0f;
        if (low_motor < 0.0f) low_motor = 0.0f;
        if (high_motor > 1.0f) high_motor = 1.0f;
        if (high_motor < 0.0f) high_motor = 0.0f;

        XINPUT_VIBRATION vib;
        vib.wLeftMotorSpeed = (WORD)(low_motor * 65535.0f);
        vib.wRightMotorSpeed = (WORD)(high_motor * 65535.0f);
        XInputSetState((DWORD)g_xinput_slot, &vib);
        g_rumble_timer = (float)duration_ms / 1000.0f;
    }
#endif
}

void controller_handle_event(AppState* state, const SDL_Event* ev) {
    if (ev->type == SDL_CONTROLLERDEVICEADDED) {
        if (!g_controller) {
            open_first_available_controller(state);
        }
    } else if (ev->type == SDL_CONTROLLERDEVICEREMOVED) {
        if (g_controller && ev->cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(g_controller))) {
            SDL_GameControllerClose(g_controller);
            g_controller = NULL;
            g_controller_index = -1;
            state->ctrl.connected = false;
            app_set_toast(state, "Controller Disconnected", 3.0f);
            printf("[Controller] Controller disconnected.\n");
            /* Try to pick up another controller if one remains */
            open_first_available_controller(state);
        }
    }
}

static float apply_deadzone(float val, float dz) {
    if (fabsf(val) < dz) return 0.0f;
    if (val > 0.0f) {
        return (val - dz) / (1.0f - dz);
    } else {
        return (val + dz) / (1.0f - dz);
    }
}

void controller_update(AppState* state, float dt) {
#ifdef _WIN32
    if (g_rumble_timer > 0.0f) {
        g_rumble_timer -= dt;
        if (g_rumble_timer <= 0.0f) {
            g_rumble_timer = 0.0f;
            if (g_xinput_slot >= 0) {
                XINPUT_VIBRATION zero_vib = { 0, 0 };
                XInputSetState((DWORD)g_xinput_slot, &zero_vib);
            }
        }
    }
#endif

    uint32_t prev_held = state->ctrl.buttons_held;
    uint32_t current_held = 0;

#ifdef _WIN32
    if (g_xinput_slot < 0) {
        find_xinput_slot(state);
    }

    if (g_xinput_slot >= 0) {
        XINPUT_STATE xstate;
        DWORD res = XInputGetState((DWORD)g_xinput_slot, &xstate);
        if (res == ERROR_SUCCESS) {
            state->ctrl.connected = true;

            /* Left Stick: in XInput, +Y is UP, -Y is DOWN. Screen coords want -Y up, +Y down */
            float norm_lx = xstate.Gamepad.sThumbLX / 32767.0f;
            float norm_ly = -xstate.Gamepad.sThumbLY / 32767.0f;
            if (norm_lx > 1.0f) norm_lx = 1.0f; else if (norm_lx < -1.0f) norm_lx = -1.0f;
            if (norm_ly > 1.0f) norm_ly = 1.0f; else if (norm_ly < -1.0f) norm_ly = -1.0f;

            float l_mag = sqrtf(norm_lx * norm_lx + norm_ly * norm_ly);
            if (l_mag < STICK_DEADZONE) {
                state->ctrl.lx = state->ctrl.ly = state->ctrl.l_mag = state->ctrl.l_angle = 0.0f;
            } else {
                float scaled = (l_mag - STICK_DEADZONE) / (1.0f - STICK_DEADZONE);
                if (scaled > 1.0f) scaled = 1.0f;
                state->ctrl.lx = (norm_lx / l_mag) * scaled;
                state->ctrl.ly = (norm_ly / l_mag) * scaled;
                state->ctrl.l_mag = scaled;
                state->ctrl.l_angle = atan2f(norm_ly, norm_lx);
            }

            /* Right Stick */
            float norm_rx = xstate.Gamepad.sThumbRX / 32767.0f;
            float norm_ry = -xstate.Gamepad.sThumbRY / 32767.0f;
            if (norm_rx > 1.0f) norm_rx = 1.0f; else if (norm_rx < -1.0f) norm_rx = -1.0f;
            if (norm_ry > 1.0f) norm_ry = 1.0f; else if (norm_ry < -1.0f) norm_ry = -1.0f;

            float r_mag = sqrtf(norm_rx * norm_rx + norm_ry * norm_ry);
            if (r_mag < STICK_DEADZONE) {
                state->ctrl.rx = state->ctrl.ry = state->ctrl.r_mag = state->ctrl.r_angle = 0.0f;
            } else {
                float scaled = (r_mag - STICK_DEADZONE) / (1.0f - STICK_DEADZONE);
                if (scaled > 1.0f) scaled = 1.0f;
                state->ctrl.rx = (norm_rx / r_mag) * scaled;
                state->ctrl.ry = (norm_ry / r_mag) * scaled;
                state->ctrl.r_mag = scaled;
                state->ctrl.r_angle = atan2f(norm_ry, norm_rx);
            }

            /* Triggers: 0..255 */
            state->ctrl.lt = apply_deadzone(xstate.Gamepad.bLeftTrigger / 255.0f, TRIGGER_DEADZONE);
            state->ctrl.rt = apply_deadzone(xstate.Gamepad.bRightTrigger / 255.0f, TRIGGER_DEADZONE);

            /* Digital buttons */
            WORD wb = xstate.Gamepad.wButtons;
            if (wb & XINPUT_GAMEPAD_A)              current_held |= BTN_A;
            if (wb & XINPUT_GAMEPAD_B)              current_held |= BTN_B;
            if (wb & XINPUT_GAMEPAD_X)              current_held |= BTN_X;
            if (wb & XINPUT_GAMEPAD_Y)              current_held |= BTN_Y;
            if (wb & XINPUT_GAMEPAD_BACK)           current_held |= BTN_BACK;
            if (wb & XINPUT_GAMEPAD_START)          current_held |= BTN_START;
            if (wb & XINPUT_GAMEPAD_LEFT_THUMB)     current_held |= BTN_LSTICK;
            if (wb & XINPUT_GAMEPAD_RIGHT_THUMB)    current_held |= BTN_RSTICK;
            if (wb & XINPUT_GAMEPAD_LEFT_SHOULDER)  current_held |= BTN_LBUMPER;
            if (wb & XINPUT_GAMEPAD_RIGHT_SHOULDER) current_held |= BTN_RBUMPER;
            if (wb & XINPUT_GAMEPAD_DPAD_UP)        current_held |= BTN_DPAD_UP;
            if (wb & XINPUT_GAMEPAD_DPAD_DOWN)      current_held |= BTN_DPAD_DOWN;
            if (wb & XINPUT_GAMEPAD_DPAD_LEFT)      current_held |= BTN_DPAD_LEFT;
            if (wb & XINPUT_GAMEPAD_DPAD_RIGHT)     current_held |= BTN_DPAD_RIGHT;

            state->ctrl.buttons_held = current_held;
            state->ctrl.buttons_pressed = current_held & ~prev_held;
            state->ctrl.buttons_released = prev_held & ~current_held;
            return;
        } else {
            /* Lost XInput connection */
            g_xinput_slot = -1;
        }
    }
#endif

    /* SDL GameController Fallback */
    if (!g_controller) {
        open_first_available_controller(state);
    }

    if (g_controller && SDL_GameControllerGetAttached(g_controller)) {
        state->ctrl.connected = true;

        /* Left Stick */
        int raw_lx = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX);
        int raw_ly = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY);
        float norm_lx = raw_lx / 32767.0f;
        float norm_ly = raw_ly / 32767.0f;
        if (norm_lx > 1.0f) norm_lx = 1.0f; else if (norm_lx < -1.0f) norm_lx = -1.0f;
        if (norm_ly > 1.0f) norm_ly = 1.0f; else if (norm_ly < -1.0f) norm_ly = -1.0f;

        float l_mag = sqrtf(norm_lx * norm_lx + norm_ly * norm_ly);
        if (l_mag < STICK_DEADZONE) {
            state->ctrl.lx = 0.0f;
            state->ctrl.ly = 0.0f;
            state->ctrl.l_mag = 0.0f;
            state->ctrl.l_angle = 0.0f;
        } else {
            float scaled_mag = (l_mag - STICK_DEADZONE) / (1.0f - STICK_DEADZONE);
            if (scaled_mag > 1.0f) scaled_mag = 1.0f;
            state->ctrl.lx = (norm_lx / l_mag) * scaled_mag;
            state->ctrl.ly = (norm_ly / l_mag) * scaled_mag;
            state->ctrl.l_mag = scaled_mag;
            state->ctrl.l_angle = atan2f(norm_ly, norm_lx);
        }

        /* Right Stick */
        int raw_rx = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTX);
        int raw_ry = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTY);
        float norm_rx = raw_rx / 32767.0f;
        float norm_ry = raw_ry / 32767.0f;
        if (norm_rx > 1.0f) norm_rx = 1.0f; else if (norm_rx < -1.0f) norm_rx = -1.0f;
        if (norm_ry > 1.0f) norm_ry = 1.0f; else if (norm_ry < -1.0f) norm_ry = -1.0f;

        float r_mag = sqrtf(norm_rx * norm_rx + norm_ry * norm_ry);
        if (r_mag < STICK_DEADZONE) {
            state->ctrl.rx = 0.0f;
            state->ctrl.ry = 0.0f;
            state->ctrl.r_mag = 0.0f;
            state->ctrl.r_angle = 0.0f;
        } else {
            float scaled_mag = (r_mag - STICK_DEADZONE) / (1.0f - STICK_DEADZONE);
            if (scaled_mag > 1.0f) scaled_mag = 1.0f;
            state->ctrl.rx = (norm_rx / r_mag) * scaled_mag;
            state->ctrl.ry = (norm_ry / r_mag) * scaled_mag;
            state->ctrl.r_mag = scaled_mag;
            state->ctrl.r_angle = atan2f(norm_ry, norm_rx);
        }

        /* Triggers */
        int raw_lt = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        int raw_rt = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        state->ctrl.lt = apply_deadzone(raw_lt / 32767.0f, TRIGGER_DEADZONE);
        state->ctrl.rt = apply_deadzone(raw_rt / 32767.0f, TRIGGER_DEADZONE);

        /* Digital buttons */
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_A))              current_held |= BTN_A;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_B))              current_held |= BTN_B;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_X))              current_held |= BTN_X;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_Y))              current_held |= BTN_Y;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_BACK))           current_held |= BTN_BACK;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_GUIDE))          current_held |= BTN_GUIDE;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_START))          current_held |= BTN_START;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_LEFTSTICK))      current_held |= BTN_LSTICK;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK))     current_held |= BTN_RSTICK;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))   current_held |= BTN_LBUMPER;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))  current_held |= BTN_RBUMPER;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))        current_held |= BTN_DPAD_UP;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))      current_held |= BTN_DPAD_DOWN;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))      current_held |= BTN_DPAD_LEFT;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))     current_held |= BTN_DPAD_RIGHT;
    } else {
        state->ctrl.connected = false;
        state->ctrl.lx = state->ctrl.ly = state->ctrl.l_mag = state->ctrl.l_angle = 0.0f;
        state->ctrl.rx = state->ctrl.ry = state->ctrl.r_mag = state->ctrl.r_angle = 0.0f;
        state->ctrl.lt = state->ctrl.rt = 0.0f;
    }

    state->ctrl.buttons_held = current_held;
    state->ctrl.buttons_pressed = current_held & ~prev_held;
    state->ctrl.buttons_released = prev_held & ~current_held;
}
