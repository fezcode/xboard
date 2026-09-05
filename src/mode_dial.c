#include "app_state.h"
#include "audio.h"
#include "controller.h"
#include "font.h"
#include "ui_draw.h"
#include "send_input.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 8 sectors x 4 characters */
static const char* SECTORS_LETTERS_LOWER[8] = {
    "abcd",  /* Sector 0: Up */
    "efgh",  /* Sector 1: Up-Right */
    "ijkl",  /* Sector 2: Right */
    "mnop",  /* Sector 3: Down-Right */
    "qrst",  /* Sector 4: Down */
    "uvwx",  /* Sector 5: Down-Left */
    "yz.,",  /* Sector 6: Left */
    "!?'-"   /* Sector 7: Up-Left */
};

static const char* SECTORS_LETTERS_UPPER[8] = {
    "ABCD",
    "EFGH",
    "IJKL",
    "MNOP",
    "QRST",
    "UVWX",
    "YZ:;",
    "!?\"_"
};

static const char* SECTORS_SYMBOLS[8] = {
    "1234",  /* Sector 0: Up */
    "5678",  /* Sector 1: Up-Right */
    "90+=",  /* Sector 2: Right */
    "!@#$",  /* Sector 3: Down-Right */
    "%^&*",  /* Sector 4: Down */
    "()[]",  /* Sector 5: Down-Left */
    "{}:;",  /* Sector 6: Left */
    "/\\<>"  /* Sector 7: Up-Left */
};

/* Sector angles (centered at -PI/2 = Up):
 * Sector 0: Up (-PI/2)
 * Sector 1: Up-Right (-PI/4)
 * Sector 2: Right (0)
 * Sector 3: Down-Right (PI/4)
 * Sector 4: Down (PI/2)
 * Sector 5: Down-Left (3PI/4)
 * Sector 6: Left (PI)
 * Sector 7: Up-Left (-3PI/4)
 */

static int get_sector_from_stick(float x, float y, float mag) {
    if (mag < 0.25f) return -1;
    float angle = atan2f(y, x); /* -PI to PI, 0 is right, PI/2 is down, -PI/2 is up */
    /* Rotate so that -PI/2 (Up) becomes 0: */
    float norm_ang = angle + (float)M_PI * 0.5f;
    while (norm_ang < 0.0f) norm_ang += 2.0f * (float)M_PI;
    while (norm_ang >= 2.0f * (float)M_PI) norm_ang -= 2.0f * (float)M_PI;

    /* Add half sector (PI/8) for rounding */
    float sector_size = (float)M_PI / 4.0f;
    int sector = (int)((norm_ang + sector_size * 0.5f) / sector_size) % 8;
    return sector;
}

static int get_pick_from_stick(float x, float y, float mag) {
    if (mag < 0.35f) return -1;
    /* 4 quadrants:
     * Up: y < 0 and |y| > |x| -> 0
     * Right: x > 0 and |x| >= |y| -> 1
     * Down: y > 0 and |y| > |x| -> 2
     * Left: x < 0 and |x| >= |y| -> 3
     */
    if (fabsf(x) > fabsf(y)) {
        return (x > 0.0f) ? 1 : 3;
    } else {
        return (y < 0.0f) ? 0 : 2;
    }
}

static void get_dial_layout(AppState* state, int* out_cy, int* out_left_cx, int* out_right_cx, int* out_radius, int* out_inner_radius) {
    int cy = 486;
    int max_radius = 120;
    int avail_half = (state->win_w - 40) / 2;
    int radius = avail_half / 3;
    if (radius > max_radius) radius = max_radius;
    if (radius < 85) radius = 85;
    int inner_radius = (radius * 40) / 100;
    int dial_spacing = radius + 60;
    if (dial_spacing > 185) dial_spacing = 185;

    if (out_cy) *out_cy = cy;
    if (out_left_cx) *out_left_cx = state->win_w / 2 - dial_spacing;
    if (out_right_cx) *out_right_cx = state->win_w / 2 + dial_spacing;
    if (out_radius) *out_radius = radius;
    if (out_inner_radius) *out_inner_radius = inner_radius;
}

void mode_dial_update(AppState* state, float dt) {
    DialModeState* d = &state->dial;

    if (d->flick_cooldown > 0.0f) d->flick_cooldown = fmaxf(0.0f, d->flick_cooldown - dt);

    /* Track left stick sector */
    int new_sector = get_sector_from_stick(state->ctrl.lx, state->ctrl.ly, state->ctrl.l_mag);
    if (new_sector >= 0) {
        d->active_sector = new_sector;
    }

    /* Track right stick pick */
    int pick = get_pick_from_stick(state->ctrl.rx, state->ctrl.ry, state->ctrl.r_mag);
    if (pick >= 0) d->active_pick = pick;

    /* A deliberate right-stick flick commits once. Return to neutral to
     * re-arm; selecting a group or previewing below threshold stays silent. */
    bool flick_committed = false;
    if (state->ctrl.r_mag < 0.35f) d->was_picked = false;
    if (state->ctrl.r_mag > 0.60f && d->active_sector >= 0 && pick >= 0 &&
        !d->was_picked && d->flick_cooldown <= 0.0f) {
        const char* const* set = state->symbols_active ? SECTORS_SYMBOLS :
                                 (state->shift_active ? SECTORS_LETTERS_UPPER : SECTORS_LETTERS_LOWER);
        app_insert_char(state, set[d->active_sector][pick]);
        controller_rumble(0.20f, 0.25f, 40);
        d->was_picked = true;
        d->flick_cooldown = 0.28f;
        flick_committed = true;
    }

    /* Button A: commit currently highlighted letter */
    if ((state->ctrl.buttons_pressed & BTN_A) && !flick_committed) {
        if (d->active_sector >= 0 && d->active_pick >= 0) {
            const char* const* set = state->symbols_active ? SECTORS_SYMBOLS :
                                     (state->shift_active ? SECTORS_LETTERS_UPPER : SECTORS_LETTERS_LOWER);
            char ch = set[d->active_sector][d->active_pick];
            app_insert_char(state, ch);
            controller_rumble(0.20f, 0.25f, 40);
        }
    }

    /* --- Mouse Support for Dual Dial --- */
    int cy, left_cx, right_cx, radius, inner_radius;
    get_dial_layout(state, &cy, &left_cx, &right_cx, &radius, &inner_radius);

    /* Left Dial mouse click: select sector */
    float dlx = (float)(state->mouse.x - left_cx);
    float dly = (float)(state->mouse.y - cy);
    float dist_l = sqrtf(dlx * dlx + dly * dly);
    if (dist_l <= (float)(radius + 14) && dist_l >= (float)inner_radius) {
        int m_sec = get_sector_from_stick(dlx, dly, 1.0f);
        if (m_sec >= 0) {
            if (state->mouse.left_clicked) {
                d->active_sector = m_sec;
            }
        }
    }

    /* Right Dial mouse click: pick character */
    float drx = (float)(state->mouse.x - right_cx);
    float dry = (float)(state->mouse.y - cy);
    float dist_r = sqrtf(drx * drx + dry * dry);
    if (dist_r <= (float)(radius + 14)) {
        if (dist_r <= (float)inner_radius) {
            /* Center click: commit candidate */
            if (state->mouse.left_clicked && d->active_sector >= 0 && d->active_pick >= 0) {
                const char* const* set = state->symbols_active ? SECTORS_SYMBOLS :
                                         (state->shift_active ? SECTORS_LETTERS_UPPER : SECTORS_LETTERS_LOWER);
                char ch = set[d->active_sector][d->active_pick];
                app_insert_char(state, ch);
                controller_rumble(0.20f, 0.25f, 40);
            }
        } else {
            int m_pick = get_pick_from_stick(drx, dry, 1.0f);
            if (m_pick >= 0) {
                if (state->mouse.left_clicked) {
                    d->active_pick = m_pick;
                    if (d->active_sector >= 0) {
                        const char* const* set = state->symbols_active ? SECTORS_SYMBOLS :
                                                 (state->shift_active ? SECTORS_LETTERS_UPPER : SECTORS_LETTERS_LOWER);
                        char ch = set[d->active_sector][m_pick];
                        app_insert_char(state, ch);
                        controller_rumble(0.20f, 0.25f, 40);
                    }
                }
            }
        }
    }
}

void mode_dial_render(AppState* state) {
    SDL_Renderer* r = state->renderer;
    FontManager* fm = (FontManager*)state->fonts;
    DialModeState* d = &state->dial;

    int cy, left_cx, right_cx, radius, inner_radius;
    get_dial_layout(state, &cy, &left_cx, &right_cx, &radius, &inner_radius);

    const char* const* set = state->symbols_active ? SECTORS_SYMBOLS :
                             (state->shift_active ? SECTORS_LETTERS_UPPER : SECTORS_LETTERS_LOWER);

    /* --- LEFT DIAL (Sector Selection Turntable) --- */
    /* 3D Extruded Dial Base Plate */
    ui_draw_neu_circle(r, left_cx, cy, radius + 14, false, false, (SDL_Color){0});
    /* Concentric grooved dial ring */
    ui_draw_circle(r, left_cx, cy, radius + 8, 1, (SDL_Color){ 14, 16, 22, 220 });
    ui_draw_circle(r, left_cx, cy, radius + 9, 1, (SDL_Color){ 52, 60, 80, 100 });

    /* Render 8 sectors */
    float sector_angle = (float)M_PI / 4.0f; /* 45 deg */
    for (int i = 0; i < 8; ++i) {
        float mid_ang = -((float)M_PI * 0.5f) + (float)i * sector_angle;
        float start_ang = mid_ang - sector_angle * 0.47f;
        float end_ang = mid_ang + sector_angle * 0.47f;

        bool is_active = (d->active_sector == i);

        if (is_active) {
            /* Luminous Soft Azure Sector */
            ui_draw_sector_ring(r, left_cx, cy, inner_radius + 4, radius, start_ang, end_ang, NEU_ACCENT_BLUE);
            ui_draw_sector_ring(r, left_cx, cy, inner_radius + 6, radius - 2, start_ang + 0.02f, end_ang - 0.02f, (SDL_Color){ 125, 211, 252, 230 });
        } else {
            /* Tactile Titanium Sector */
            ui_draw_sector_ring(r, left_cx, cy, inner_radius + 4, radius, start_ang, end_ang, (SDL_Color){ 28, 31, 39, 230 });
            ui_draw_sector_ring(r, left_cx, cy, inner_radius + 6, radius - 2, start_ang + 0.02f, end_ang - 0.02f, (SDL_Color){ 38, 42, 54, 180 });
        }

        /* Sector label (4 characters in sector) */
        float text_r = (inner_radius + radius) * 0.52f;
        int tx = left_cx + (int)(text_r * cosf(mid_ang));
        int ty = cy + (int)(text_r * sinf(mid_ang));

        char label[8];
        snprintf(label, sizeof(label), "%c%c%c%c", set[i][0], set[i][1], set[i][2], set[i][3]);
        SDL_Color text_col = is_active ? (SDL_Color){ 10, 18, 28, 255 } : COLOR_TEXT_MUTED;
        font_draw_text(fm, r, label, tx, ty, FONT_SIZE_SMALL, text_col, true, true);
    }

    /* Left Stick Concave Dish / Socket */
    ui_draw_neu_well_circle(r, left_cx, cy, inner_radius);

    /* Left stick 3D Dome Thumbstick Cap */
    int stick_dot_x = left_cx + (int)(state->ctrl.lx * (inner_radius - 14));
    int stick_dot_y = cy + (int)(state->ctrl.ly * (inner_radius - 14));
    bool l_stick_active = (state->ctrl.l_mag > 0.25f);
    ui_draw_neu_circle(r, stick_dot_x, stick_dot_y, 14, false, l_stick_active, NEU_ACCENT_BLUE);
    ui_draw_filled_circle(r, stick_dot_x, stick_dot_y, 4, NEU_ACCENT_BLUE);

    /* Neumorphic Header & Subtitle Pills */
    int banner_w = 120;
    ui_draw_neu_panel(r, left_cx - banner_w / 2, cy - radius - 38, banner_w, 24, 12, false, false, (SDL_Color){0});
    font_draw_text(fm, r, "LEFT STICK", left_cx, cy - radius - 26, FONT_SIZE_SMALL, NEU_ACCENT_BLUE, true, true);
    font_draw_text(fm, r, "SELECT GROUP", left_cx, cy + radius + 28, FONT_SIZE_SMALL, COLOR_TEXT_MUTED, true, true);

    /* --- RIGHT DIAL (Character Pick Turntable) --- */
    ui_draw_neu_circle(r, right_cx, cy, radius + 14, false, false, (SDL_Color){0});
    /* Concentric grooved dial ring */
    ui_draw_circle(r, right_cx, cy, radius + 8, 1, (SDL_Color){ 14, 16, 22, 220 });
    ui_draw_circle(r, right_cx, cy, radius + 9, 1, (SDL_Color){ 52, 60, 80, 100 });

    int sec = (d->active_sector >= 0) ? d->active_sector : 0;
    const char* cur_group = set[sec];

    /* 4 petals for Up, Right, Down, Left */
    static const float PICK_ANGLES[4] = { -((float)M_PI * 0.5f), 0.0f, ((float)M_PI * 0.5f), (float)M_PI };
    static const char* const PICK_DIRS[4] = { "UP", "RIGHT", "DOWN", "LEFT" };

    /* 1. Draw Petal Geometries First */
    for (int p = 0; p < 4; ++p) {
        float mid_ang = PICK_ANGLES[p];
        float start_ang = mid_ang - (float)M_PI * 0.22f;
        float end_ang = mid_ang + (float)M_PI * 0.22f;

        bool is_picked = (d->active_pick == p);

        if (is_picked) {
            /* Luminous Soft Mint Petal */
            ui_draw_sector_ring(r, right_cx, cy, inner_radius + 4, radius, start_ang, end_ang, NEU_ACCENT_MINT);
            ui_draw_sector_ring(r, right_cx, cy, inner_radius + 6, radius - 2, start_ang + 0.03f, end_ang - 0.03f, (SDL_Color){ 110, 231, 183, 230 });
        } else {
            /* Tactile Titanium Petal */
            ui_draw_sector_ring(r, right_cx, cy, inner_radius + 4, radius, start_ang, end_ang, (SDL_Color){ 28, 31, 39, 230 });
            ui_draw_sector_ring(r, right_cx, cy, inner_radius + 6, radius - 2, start_ang + 0.03f, end_ang - 0.03f, (SDL_Color){ 38, 42, 54, 180 });
        }
    }

    /* 2. Draw Center Porthole / Well */
    ui_draw_neu_circle(r, right_cx, cy, inner_radius, true, (d->active_pick >= 0), NEU_ACCENT_MINT);

    /* Show candidate big character in center if selected */
    if (d->active_pick >= 0 && d->active_sector >= 0) {
        char cand[2] = { cur_group[d->active_pick], '\0' };
        font_draw_text(fm, r, cand, right_cx, cy, FONT_SIZE_HUGE, NEU_ACCENT_MINT, true, true);
    } else {
        /* Right stick 3D Dome Thumbstick Cap */
        int r_stick_x = right_cx + (int)(state->ctrl.rx * (inner_radius - 14));
        int r_stick_y = cy + (int)(state->ctrl.ry * (inner_radius - 14));
        bool r_stick_active = (state->ctrl.r_mag > 0.25f);
        ui_draw_neu_circle(r, r_stick_x, r_stick_y, 14, false, r_stick_active, NEU_ACCENT_MINT);
        ui_draw_filled_circle(r, r_stick_x, r_stick_y, 4, NEU_ACCENT_MINT);
    }

    /* 3. Render Direction Labels & Characters AFTER Center Well so they are 100% visible and unclipped */
    for (int p = 0; p < 4; ++p) {
        float mid_ang = PICK_ANGLES[p];
        bool is_picked = (d->active_pick == p);

        /* Direction hint: placed cleanly at r=66px (well clear of the 48px inner socket) */
        float dir_r = 66.0f;
        int dir_x = right_cx + (int)(dir_r * cosf(mid_ang));
        int dir_y = cy + (int)(dir_r * sinf(mid_ang));
        SDL_Color dir_col = is_picked ? (SDL_Color){ 10, 25, 18, 255 } : NEU_ACCENT_BLUE;
        font_draw_text(fm, r, PICK_DIRS[p], dir_x, dir_y, FONT_SIZE_SMALL, dir_col, true, true);

        /* Character: placed at r=96px closer to outer rim */
        float char_r = 96.0f;
        int cx_pos = right_cx + (int)(char_r * cosf(mid_ang));
        int cy_pos = cy + (int)(char_r * sinf(mid_ang));

        char ch_str[2] = { cur_group[p], '\0' };
        SDL_Color ch_color = is_picked ? (SDL_Color){ 10, 25, 18, 255 } : COLOR_TEXT_BRIGHT;
        font_draw_text(fm, r, ch_str, cx_pos, cy_pos, FONT_SIZE_LARGE, ch_color, true, true);
    }

    /* Neumorphic Header & Subtitle Pills */
    ui_draw_neu_panel(r, right_cx - banner_w / 2, cy - radius - 38, banner_w, 24, 12, false, false, (SDL_Color){0});
    font_draw_text(fm, r, "RIGHT STICK", right_cx, cy - radius - 26, FONT_SIZE_SMALL, NEU_GREEN, true, true);
    font_draw_text(fm, r, "FLICK OR [A] TO TYPE", right_cx, cy + radius + 28, FONT_SIZE_SMALL, COLOR_TEXT_MUTED, true, true);

    /* Center connection conduit */
    int mid_x = state->win_w / 2;
    /* Recessed bridge groove */
    ui_draw_thick_line(r, left_cx + radius + 14, cy, right_cx - radius - 14, cy, 4, (SDL_Color){ 14, 16, 22, 255 });
    ui_draw_thick_line(r, left_cx + radius + 14, cy + 1, right_cx - radius - 14, cy + 1, 2, (SDL_Color){ 45, 54, 75, 120 });

    bool connection_active = (d->active_sector >= 0 && d->active_pick >= 0);
    ui_draw_neu_circle(r, mid_x, cy, 16, false, connection_active, NEU_CYAN);
    font_draw_text(fm, r, "->", mid_x, cy, FONT_SIZE_SMALL, connection_active ? NEU_CYAN : COLOR_TEXT_MUTED, true, true);
}
