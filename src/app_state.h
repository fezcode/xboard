#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>

#define XBOARD_VERSION "2.0.0"
#include "text_buffer.h"

typedef enum {
    MODE_DIAL = 0,      /* Dual-stick dial (Left stick sector, Right stick pick) */
    MODE_GRID = 1,      /* Virtual QWERTY keyboard grid */
    MODE_MORSE = 2,     /* Morse code: Left stick Dit, Right stick Dah + Sound */
    MODE_COUNT
} AppMode;

/* Normalized controller input */
typedef struct {
    bool connected;
    char name[64];

    /* Left Stick (-1.0 to 1.0, deadzone applied) */
    float lx, ly;
    float l_mag;       /* 0.0 to 1.0 */
    float l_angle;     /* Radians, 0 = right, pi/2 = down */

    /* Right Stick (-1.0 to 1.0, deadzone applied) */
    float rx, ry;
    float r_mag;       /* 0.0 to 1.0 */
    float r_angle;     /* Radians */

    /* Triggers: 0.0 to 1.0 */
    float lt, rt;

    /* Digital buttons: held, pressed (just down this frame), released */
    uint32_t buttons_held;
    uint32_t buttons_pressed;
    uint32_t buttons_released;
} ControllerInput;

enum ControllerButtonBits {
    BTN_A         = 1 << 0,
    BTN_B         = 1 << 1,
    BTN_X         = 1 << 2,
    BTN_Y         = 1 << 3,
    BTN_BACK      = 1 << 4,
    BTN_GUIDE     = 1 << 5,
    BTN_START     = 1 << 6,
    BTN_LSTICK    = 1 << 7,
    BTN_RSTICK    = 1 << 8,
    BTN_LBUMPER   = 1 << 9,
    BTN_RBUMPER   = 1 << 10,
    BTN_DPAD_UP   = 1 << 11,
    BTN_DPAD_DOWN = 1 << 12,
    BTN_DPAD_LEFT = 1 << 13,
    BTN_DPAD_RIGHT= 1 << 14,
};

/* Dual-stick dial state */
typedef struct {
    int active_sector;       /* 0..7, or -1 if centered */
    int active_pick;         /* 0..3 (Up, Right, Down, Left) or -1 */
    int confirmed_char;
    float flick_cooldown;    /* Throttle repeat typing */
    bool was_picked;
} DialModeState;

/* Virtual keyboard grid state */
typedef struct {
    int row;                 /* 0..4 */
    int col;                 /* column in row */
    float nav_repeat_timer;
    int last_nav_dx;
    int last_nav_dy;
} GridSelector;

typedef struct {
    GridSelector selectors[2]; /* left/red, right/blue */
    int active;
    bool initialized;
} GridModeState;

/* Morse mode state */
typedef struct {
    char sequence[16];       /* e.g. ".-." */
    int seq_len;
    float silence_timer;     /* Time since last dit/dah */
    float auto_commit_delay; /* Default ~0.7s */
    char candidate_char;     /* Real-time decoded character */
    bool dit_held;
    bool dah_held;
    float dit_flick_cooldown;
    float dah_flick_cooldown;
} MorseModeState;

typedef struct {
    int x;
    int y;
    bool left_down;
    bool left_clicked;
    bool right_down;
    bool right_clicked;
    int wheel_y;
} MouseInput;

/* Main application state */
typedef struct AppState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    int win_w;
    int win_h;
    bool running;

    AppMode mode;
    ControllerInput ctrl;
    MouseInput mouse;
    bool mouse_moved;

    /* Text buffer */
    TextBuffer editor;
    bool help_open;
    bool phrases_focused;
    int phrase_index;
    int phrase_direction;
    float phrase_repeat_timer;
    bool stick_chord_latched;
    uint32_t stick_click_pending;
    bool navigation_needs_neutral;

    /* Modifiers & Options */
    bool shift_active;        /* Uppercase vs lowercase (LT held or Shift) */
    bool ctrl_active;         /* Ctrl modifier (RT held or Ctrl) */
    bool caps_lock;           /* Caps lock toggle */
    bool symbols_active;      /* Special symbols & numbers */
    bool direct_send_input;   /* SendInput keystrokes to background app */
    bool sound_enabled;       /* Audio side-tone */
    float b_repeat_timer;     /* Hold timer for backspace */
    float dpad_repeat_timer;  /* Hold timer for D-pad cursor movement */
    int dpad_last_dir;        /* Last D-pad direction (1:L, 2:R, 3:U, 4:D) */
    bool ctrl_locked;         /* Ctrl toggle lock via mouse click */
    bool win_active;          /* Windows key modifier active (L3 held, Win button, or Win lock) */
    bool win_locked;          /* Windows key toggle lock via mouse/hotkey */
    int theme_index;          /* Active visual theme */

    /* Toast notification */
    char toast[128];
    float toast_timer;

    /* Mode-specific substates */
    DialModeState dial;
    GridModeState grid;
    MorseModeState morse;

    /* Audio system handle */
    void* audio;

    /* Fonts handle */
    void* fonts;
} AppState;

/* Text buffer operations */
void app_insert_char(AppState* state, char c);
void app_insert_string(AppState* state, const char* str);
void app_backspace(AppState* state);
void app_delete_word(AppState* state);
void app_space(AppState* state);
void app_newline(AppState* state);
void app_clear(AppState* state);
void app_copy(AppState* state);
void app_undo(AppState* state);
void app_redo(AppState* state);
void app_toggle_direct(AppState* state);
void app_load_preferences(AppState* state);
void app_save_preferences(AppState* state);
void app_paste(AppState* state);
void app_select_all(AppState* state);
void app_win_key(AppState* state);
void app_cycle_theme(AppState* state);
void app_cursor_left(AppState* state);
void app_cursor_right(AppState* state);
void app_cursor_up(AppState* state);
void app_cursor_down(AppState* state);
void app_set_toast(AppState* state, const char* msg, float duration);

#endif /* APP_STATE_H */
