#include "app_state.h"
#include "audio.h"
#include "controller.h"
#include "font.h"
#include "ui_draw.h"
#include "send_input.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

typedef struct {
    const char* label;
    const char* shift_label;
    int width_units; /* standard key = 2 units */
    char action_type; /* 'C' = char, 'B' = backspace, 'S' = space, 'E' = enter, 'T' = shift, 'Y' = symbols */
    char ch_normal;
    char ch_shift;
} KeyDef;

#define MAX_COLS 14

static const KeyDef ROW0[] = {
    { "`", "~", 2, 'C', '`', '~' },
    { "1", "!", 2, 'C', '1', '!' },
    { "2", "@", 2, 'C', '2', '@' },
    { "3", "#", 2, 'C', '3', '#' },
    { "4", "$", 2, 'C', '4', '$' },
    { "5", "%", 2, 'C', '5', '%' },
    { "6", "^", 2, 'C', '6', '^' },
    { "7", "&", 2, 'C', '7', '&' },
    { "8", "*", 2, 'C', '8', '*' },
    { "9", "(", 2, 'C', '9', '(' },
    { "0", ")", 2, 'C', '0', ')' },
    { "-", "_", 2, 'C', '-', '_' },
    { "=", "+", 2, 'C', '=', '+' },
    { "Bksp", "Bksp", 3, 'B', 0, 0 }
};

static const KeyDef ROW1[] = {
    { "Tab", "Tab", 3, 'C', '\t', '\t' },
    { "q", "Q", 2, 'C', 'q', 'Q' },
    { "w", "W", 2, 'C', 'w', 'W' },
    { "e", "E", 2, 'C', 'e', 'E' },
    { "r", "R", 2, 'C', 'r', 'R' },
    { "t", "T", 2, 'C', 't', 'T' },
    { "y", "Y", 2, 'C', 'y', 'Y' },
    { "u", "U", 2, 'C', 'u', 'U' },
    { "i", "I", 2, 'C', 'i', 'I' },
    { "o", "O", 2, 'C', 'o', 'O' },
    { "p", "P", 2, 'C', 'p', 'P' },
    { "[", "{", 2, 'C', '[', '{' },
    { "]", "}", 2, 'C', ']', '}' },
    { "\\", "|", 2, 'C', '\\', '|' }
};

static const KeyDef ROW2[] = {
    { "Caps", "Caps", 3, 'T', 0, 0 },
    { "a", "A", 2, 'C', 'a', 'A' },
    { "s", "S", 2, 'C', 's', 'S' },
    { "d", "D", 2, 'C', 'd', 'D' },
    { "f", "F", 2, 'C', 'f', 'F' },
    { "g", "G", 2, 'C', 'g', 'G' },
    { "h", "H", 2, 'C', 'h', 'H' },
    { "j", "J", 2, 'C', 'j', 'J' },
    { "k", "K", 2, 'C', 'k', 'K' },
    { "l", "L", 2, 'C', 'l', 'L' },
    { ";", ":", 2, 'C', ';', ':' },
    { "'", "\"", 2, 'C', '\'', '\"' },
    { "Enter", "Enter", 4, 'E', '\n', '\n' }
};

static const KeyDef ROW3[] = {
    { "Shift", "Shift", 4, 'T', 0, 0 },
    { "z", "Z", 2, 'C', 'z', 'Z' },
    { "x", "X", 2, 'C', 'x', 'X' },
    { "c", "C", 2, 'C', 'c', 'C' },
    { "v", "V", 2, 'C', 'v', 'V' },
    { "b", "B", 2, 'C', 'b', 'B' },
    { "n", "N", 2, 'C', 'n', 'N' },
    { "m", "M", 2, 'C', 'm', 'M' },
    { ",", "<", 2, 'C', ',', '<' },
    { ".", ">", 2, 'C', '.', '>' },
    { "/", "?", 2, 'C', '/', '?' },
    { "Clear", "Clear", 4, 'X', 0, 0 }
};

static const KeyDef ROW4[] = {
    { "Win", "Win", 3, 'W', 0, 0 },
    { "Sym", "Sym", 3, 'Y', 0, 0 },
    { "Space", "Space", 12, 'S', ' ', ' ' },
    { "Copy", "Copy", 4, 'P', 0, 0 }
};

static const struct {
    const KeyDef* keys;
    int count;
} GRID_ROWS[] = {
    { ROW0, sizeof(ROW0) / sizeof(ROW0[0]) },
    { ROW1, sizeof(ROW1) / sizeof(ROW1[0]) },
    { ROW2, sizeof(ROW2) / sizeof(ROW2[0]) },
    { ROW3, sizeof(ROW3) / sizeof(ROW3[0]) },
    { ROW4, sizeof(ROW4) / sizeof(ROW4[0]) },
};

#define ROW_COUNT (sizeof(GRID_ROWS) / sizeof(GRID_ROWS[0]))

static void initialize_selectors(GridModeState* g) {
    if (g->initialized) return;
    g->selectors[1].col = 7;
    g->initialized = true;
}

static void activate_key(AppState* s, const KeyDef* key) {
    audio_play_click(s->audio);
    controller_rumble(.18f, .22f, 40);
    switch (key->action_type) {
        case 'C': app_insert_char(s, (s->shift_active || s->symbols_active) ? key->ch_shift : key->ch_normal); break;
        case 'B': app_backspace(s); break;
        case 'S': app_space(s); break;
        case 'E': app_newline(s); break;
        case 'T': s->caps_lock = !s->caps_lock; s->shift_active = s->caps_lock; break;
        case 'Y': s->symbols_active = !s->symbols_active; break;
        case 'X': app_clear(s); break;
        case 'P': app_copy(s); break;
        case 'W': app_win_key(s); break;
    }
}

/* Each selector has its own repeat timer. A fresh directional gesture takes
 * precedence over another stick's repeat; exact simultaneous gestures keep
 * the previously active selector. */
static int navigate(GridSelector* g, float x, float y, float dt) {
    int dx = x < -.45f ? -1 : x > .45f ? 1 : 0;
    int dy = y < -.45f ? -1 : y > .45f ? 1 : 0;
    if (dx && dy) { if (fabsf(x) >= fabsf(y)) dy = 0; else dx = 0; }
    bool fresh = dx != g->last_nav_dx || dy != g->last_nav_dy;
    g->last_nav_dx = dx; g->last_nav_dy = dy;
    if (!dx && !dy) { g->nav_repeat_timer = 0; return 0; }
    g->nav_repeat_timer -= dt;
    if (!fresh && g->nav_repeat_timer > 0) return 0;
    g->nav_repeat_timer = fresh ? .32f : .12f;
    if (dy) {
        g->row = (g->row + dy + (int)ROW_COUNT) % (int)ROW_COUNT;
        if (g->col >= GRID_ROWS[g->row].count) g->col = GRID_ROWS[g->row].count - 1;
    }
    if (dx) g->col = (g->col + dx + GRID_ROWS[g->row].count) % GRID_ROWS[g->row].count;
    return fresh ? 2 : 1;
}

void mode_grid_update(AppState* state, float dt) {
    if (state->phrases_focused) return;
    GridModeState* g = &state->grid;
    initialize_selectors(g);
    float x[2] = {state->ctrl.lx, state->ctrl.rx};
    float y[2] = {state->ctrl.ly, state->ctrl.ry};
    /* D-pad is an alternative for the currently active selector. */
    if (state->ctrl.buttons_held & BTN_DPAD_LEFT) x[g->active] = -1;
    if (state->ctrl.buttons_held & BTN_DPAD_RIGHT) x[g->active] = 1;
    if (state->ctrl.buttons_held & BTN_DPAD_UP) y[g->active] = -1;
    if (state->ctrl.buttons_held & BTN_DPAD_DOWN) y[g->active] = 1;
    int moved[2];
    for (int i=0;i<2;++i) moved[i] = navigate(&g->selectors[i], x[i], y[i], dt);
    if (moved[0] > moved[1]) g->active = 0;
    else if (moved[1] > moved[0]) g->active = 1;
    if (state->ctrl.buttons_pressed & BTN_A) {
        GridSelector* selected = &g->selectors[g->active];
        activate_key(state, &GRID_ROWS[selected->row].keys[selected->col]);
    }
    /* Mouse clicks activate a key without moving either stick selector. */
    if (!state->mouse.left_clicked) return;
    int kb_w = state->win_w - 60;
    if (kb_w > 1080) kb_w = 1080;
    int start_x = (state->win_w - kb_w) / 2;
    int key_h = (330 - ((int)ROW_COUNT - 1) * 8) / (int)ROW_COUNT;
    for (int row=0;row<(int)ROW_COUNT;++row) {
        int yy = 302 + row * (key_h + 8);
        if (state->mouse.y < yy || state->mouse.y >= yy + key_h) continue;
        int count = GRID_ROWS[row].count, units = 0;
        for (int c=0;c<count;++c) units += GRID_ROWS[row].keys[c].width_units;
        float unit_w = (float)(kb_w - (count - 1) * 7) / units;
        int xx = start_x;
        for (int c=0;c<count;++c) {
            const KeyDef* key = &GRID_ROWS[row].keys[c];
            int kw = (int)(key->width_units * unit_w);
            if (state->mouse.x >= xx && state->mouse.x < xx+kw) { activate_key(state,key); return; }
            xx += kw + 7;
        }
    }
}

void mode_grid_render(AppState* state) {
    SDL_Renderer* r = state->renderer;
    FontManager* fm = (FontManager*)state->fonts;
    GridModeState* g = &state->grid;
    initialize_selectors(g);
    const SDL_Color selector_colors[2] = {{255, 103, 120, 255}, {92, 166, 255, 255}};

    int kb_w = state->win_w - 60;
    if (kb_w > 1080) kb_w = 1080;
    int kb_h = 330;
    int start_x = (state->win_w - kb_w) / 2;
    int start_y = 302;

    int row_spacing = 8;
    int col_spacing = 7;
    int key_h = (kb_h - (ROW_COUNT - 1) * row_spacing) / ROW_COUNT;

    /* Neumorphic Sunken Chassis Tray */
    ui_draw_neu_panel(r, start_x - 14, start_y - 14, kb_w + 28, kb_h + 28, 16, true, false, (SDL_Color){0});

    for (int row = 0; row < (int)ROW_COUNT; ++row) {
        int key_count = GRID_ROWS[row].count;
        const KeyDef* keys = GRID_ROWS[row].keys;

        /* Calculate total units in this row */
        int total_units = 0;
        for (int i = 0; i < key_count; ++i) {
            total_units += keys[i].width_units;
        }

        int available_w = kb_w - (key_count - 1) * col_spacing;
        float unit_w = (float)available_w / (float)total_units;

        float curr_x = (float)start_x;
        int curr_y = start_y + row * (key_h + row_spacing);

        for (int col = 0; col < key_count; ++col) {
            const KeyDef* key = &keys[col];
            int kw = (int)(key->width_units * unit_w);
            bool red = row == g->selectors[0].row && col == g->selectors[0].col;
            bool blue = row == g->selectors[1].row && col == g->selectors[1].col;
            bool is_selected = g->active == 0 ? red : blue;
            SDL_Color focus_color = selector_colors[g->active];
            ui_draw_neu_panel(r, (int)curr_x, curr_y, kw, key_h, 7, false,
                              (red || blue) && !state->phrases_focused,
                              red && !blue ? selector_colors[0] : blue && !red ? selector_colors[1] : focus_color);
            for (int i=0;i<2;++i) {
                if ((i==0 && !red) || (i==1 && !blue)) continue;
                int marker_x = (int)curr_x + (i==0 ? 8 : kw-23);
                font_draw_text(fm,r,i==0?"L":"R",marker_x,curr_y+key_h-12,FONT_SIZE_SMALL,selector_colors[i],false,true);
                if (g->active==i && !state->phrases_focused)
                    ui_draw_thick_line(r,marker_x,curr_y+key_h-3,marker_x+14,curr_y+key_h-3,2,selector_colors[i]);
            }

            /* Text label */
            const char* lbl = (state->shift_active || state->symbols_active) ? key->shift_label : key->label;
            SDL_Color text_col = is_selected && !state->phrases_focused ? focus_color :
                                 ((key->action_type != 'C') ? (SDL_Color){ 160, 190, 230, 255 } : COLOR_TEXT_BRIGHT);
            FontSize sz = (strlen(lbl) > 2) ? FONT_SIZE_SMALL : FONT_SIZE_MEDIUM;

            font_draw_text(fm, r, lbl, (int)curr_x + kw / 2, curr_y + key_h / 2, sz, text_col, true, true);

            curr_x += kw + col_spacing;
        }
    }

    /* Help footer pill */
    int footer_y = start_y + kb_h + 32;
    char help_str[256];
    snprintf(help_str, sizeof(help_str),
             g->active == 0 ? "Left stick: RED     Right stick: BLUE     A: RED selection     L3 + R3: Quick phrases" : "Left stick: RED     Right stick: BLUE     A: BLUE selection     L3 + R3: Quick phrases");
    int hw = 0;
    font_measure_text(fm, help_str, FONT_SIZE_SMALL, &hw, NULL);
    int pill_w = hw + 36;
    ui_draw_neu_panel(r, state->win_w / 2 - pill_w / 2, footer_y - 14, pill_w, 28, 14, false, false, (SDL_Color){0});
    font_draw_text(fm, r, help_str, state->win_w / 2, footer_y, FONT_SIZE_SMALL, COLOR_TEXT_MUTED, true, true);
}
