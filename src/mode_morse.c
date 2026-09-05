#include "app_state.h"
#include "audio.h"
#include "controller.h"
#include "font.h"
#include "morse.h"
#include "ui_draw.h"
#include "send_input.h"
#include <stdio.h>
#include <string.h>

#define MORSE_AUTO_COMMIT_TIME 0.75f

void mode_morse_update(AppState* state, float dt) {
    MorseModeState* m = &state->morse;

    if (m->dit_flick_cooldown > 0.0f) m->dit_flick_cooldown -= dt;
    if (m->dah_flick_cooldown > 0.0f) m->dah_flick_cooldown -= dt;

    /* Left Analog Stick -> Dit (.) */
    bool l_flick = (state->ctrl.l_mag > 0.55f);
    if (l_flick && m->dit_flick_cooldown <= 0.0f) {
        if (m->seq_len < (int)sizeof(m->sequence) - 1) {
            m->sequence[m->seq_len++] = '.';
            m->sequence[m->seq_len] = '\0';
            audio_play_dit((AudioEngine*)state->audio);
            controller_rumble(0.0f, 0.35f, 60);
            m->silence_timer = 0.0f;
            /* Auto-repeat rate if held vs single flick */
            m->dit_flick_cooldown = m->dit_held ? 0.20f : 0.24f;
            m->candidate_char = morse_decode(m->sequence);
        }
    }
    m->dit_held = l_flick;

    /* Right Analog Stick -> Dah (-) */
    bool r_flick = (state->ctrl.r_mag > 0.55f);
    if (r_flick && m->dah_flick_cooldown <= 0.0f) {
        if (m->seq_len < (int)sizeof(m->sequence) - 1) {
            m->sequence[m->seq_len++] = '-';
            m->sequence[m->seq_len] = '\0';
            audio_play_dah((AudioEngine*)state->audio);
            controller_rumble(0.08f, 0.30f, 90);
            m->silence_timer = 0.0f;
            /* Auto-repeat rate if held vs single flick */
            m->dah_flick_cooldown = m->dah_held ? 0.32f : 0.36f;
            m->candidate_char = morse_decode(m->sequence);
        }
    }
    m->dah_held = r_flick;

    /* If a sequence is active, track silence timer for auto-commit */
    if (m->seq_len > 0) {
        m->silence_timer += dt;
        if (m->silence_timer >= MORSE_AUTO_COMMIT_TIME) {
            char decoded = morse_decode(m->sequence);
            if (decoded != '\0') {
                app_insert_char(state, state->shift_active ? decoded : (char)tolower((unsigned char)decoded));
                controller_rumble(0.15f, 0.15f, 40);
            }
            m->sequence[0] = '\0';
            m->seq_len = 0;
            m->silence_timer = 0.0f;
            m->candidate_char = '\0';
        }
    }

    /* Press A to commit immediately */
    if (state->ctrl.buttons_pressed & BTN_A) {
        if (m->seq_len > 0) {
            char decoded = morse_decode(m->sequence);
            if (decoded != '\0') {
                app_insert_char(state, state->shift_active ? decoded : (char)tolower((unsigned char)decoded));
                controller_rumble(0.15f, 0.15f, 40);
            }
            m->sequence[0] = '\0';
            m->seq_len = 0;
            m->silence_timer = 0.0f;
            m->candidate_char = '\0';
        } else {
            app_space(state);
            controller_rumble(0.10f, 0.10f, 30);
        }
    }

    /* --- Mouse Support for Morse Code --- */
    int card_w = 260;
    int card_h = 150;
    int left_card_x = state->win_w / 2 - card_w - 70;
    int right_card_x = state->win_w / 2 + 70;
    int card_y = 292;

    int center_box_w = 110;
    int center_box_x = state->win_w / 2 - center_box_w / 2;

    int seq_pill_w = 420;
    int seq_pill_h = 44;
    int seq_pill_y = card_y + card_h + 16;
    int seq_pill_x = state->win_w / 2 - seq_pill_w / 2;

    int cheat_y = seq_pill_y + seq_pill_h + 18;
    int cheat_w = state->win_w - 60;
    if (cheat_w > 920) cheat_w = 920;
    int cheat_h = 160;
    int cheat_x = (state->win_w - cheat_w) / 2;

    if (state->mouse.left_clicked) {
        /* Click Left Card: Dit */
        if (state->mouse.x >= left_card_x && state->mouse.x <= left_card_x + card_w &&
            state->mouse.y >= card_y && state->mouse.y <= card_y + card_h) {
            if (m->seq_len < (int)sizeof(m->sequence) - 1) {
                m->sequence[m->seq_len++] = '.';
                m->sequence[m->seq_len] = '\0';
                audio_play_dit((AudioEngine*)state->audio);
                controller_rumble(0.0f, 0.35f, 60);
                m->silence_timer = 0.0f;
                m->candidate_char = morse_decode(m->sequence);
            }
        }
        /* Click Right Card: Dah */
        else if (state->mouse.x >= right_card_x && state->mouse.x <= right_card_x + card_w &&
                 state->mouse.y >= card_y && state->mouse.y <= card_y + card_h) {
            if (m->seq_len < (int)sizeof(m->sequence) - 1) {
                m->sequence[m->seq_len++] = '-';
                m->sequence[m->seq_len] = '\0';
                audio_play_dah((AudioEngine*)state->audio);
                controller_rumble(0.08f, 0.30f, 90);
                m->silence_timer = 0.0f;
                m->candidate_char = morse_decode(m->sequence);
            }
        }
        /* Click Decoded Center Box: Commit */
        else if (state->mouse.x >= center_box_x && state->mouse.x <= center_box_x + center_box_w &&
                 state->mouse.y >= card_y && state->mouse.y <= card_y + card_h) {
            if (m->seq_len > 0) {
                char decoded = morse_decode(m->sequence);
                if (decoded != '\0') {
                    app_insert_char(state, state->shift_active ? decoded : (char)tolower((unsigned char)decoded));
                    controller_rumble(0.15f, 0.15f, 40);
                }
                m->sequence[0] = '\0';
                m->seq_len = 0;
                m->silence_timer = 0.0f;
                m->candidate_char = '\0';
            }
        }
        /* Click Sequence Pill: Backspace/Clear sequence */
        else if (state->mouse.x >= seq_pill_x && state->mouse.x <= seq_pill_x + seq_pill_w &&
                 state->mouse.y >= seq_pill_y && state->mouse.y <= seq_pill_y + seq_pill_h) {
            if (m->seq_len > 0) {
                m->sequence[--m->seq_len] = '\0';
                m->candidate_char = morse_decode(m->sequence);
                m->silence_timer = 0.0f;
            }
        }
        /* Click Cheat Sheet: directly insert character */
        else if (state->mouse.x >= cheat_x && state->mouse.x <= cheat_x + cheat_w &&
                 state->mouse.y >= cheat_y && state->mouse.y <= cheat_y + cheat_h) {
            int count = 0;
            const MorseEntry* table = morse_get_table(&count);
            int cols = 9;
            int rows = 4;
            int cell_w = cheat_w / cols;
            int cell_h = cheat_h / rows;
            int col = (state->mouse.x - cheat_x) / cell_w;
            int row = (state->mouse.y - cheat_y) / cell_h;
            int idx = row * cols + col;
            if (idx >= 0 && idx < count) {
                char ch = table[idx].ch;
                app_insert_char(state, state->shift_active ? ch : (char)tolower((unsigned char)ch));
                audio_play_click((AudioEngine*)state->audio);
                controller_rumble(0.18f, 0.20f, 40);
                m->sequence[0] = '\0';
                m->seq_len = 0;
                m->silence_timer = 0.0f;
                m->candidate_char = '\0';
            }
        }
    }
}

void mode_morse_render(AppState* state) {
    SDL_Renderer* r = state->renderer;
    FontManager* fm = (FontManager*)state->fonts;
    MorseModeState* m = &state->morse;

    /* Stomp Pad & HUD Geometry */
    int card_w = 260;
    int card_h = 150;
    int left_card_x = state->win_w / 2 - card_w - 70;
    int right_card_x = state->win_w / 2 + 70;
    int card_y = 292;

    /* --- LEFT STOMP PAD: DIT (.) --- */
    bool dit_active = (state->ctrl.l_mag > 0.55f);
    bool dit_mouse = state->mouse.left_down && (state->mouse.x >= left_card_x && state->mouse.x <= left_card_x + card_w &&
                                                state->mouse.y >= card_y && state->mouse.y <= card_y + card_h);
    bool dit_depressed = dit_active || dit_mouse;

    ui_draw_neu_panel(r, left_card_x, card_y, card_w, card_h, 16, dit_depressed, dit_depressed, NEU_CYAN);

    /* Recessed banner inside pad */
    int banner_w = card_w - 40;
    ui_draw_neu_panel(r, left_card_x + 20, card_y + 12, banner_w, 24, 12, true, false, (SDL_Color){0});
    font_draw_text(fm, r, "LEFT ANALOG", left_card_x + card_w / 2, card_y + 24, FONT_SIZE_SMALL, NEU_CYAN, true, true);

    font_draw_text(fm, r, "DIT ( . )", left_card_x + card_w / 2, card_y + 64, FONT_SIZE_LARGE,
                   dit_depressed ? NEU_CYAN : COLOR_TEXT_BRIGHT, true, true);
    font_draw_text(fm, r, "Short Tone (820 Hz)", left_card_x + card_w / 2, card_y + 98, FONT_SIZE_SMALL, COLOR_TEXT_MUTED, true, true);

    /* Left stick thumb well */
    ui_draw_neu_well_circle(r, left_card_x + card_w / 2, card_y + 126, 12);
    int l_thumb_x = left_card_x + card_w / 2 + (int)(state->ctrl.lx * 8);
    int l_thumb_y = card_y + 126 + (int)(state->ctrl.ly * 5);
    ui_draw_filled_circle(r, l_thumb_x, l_thumb_y, 5, dit_depressed ? NEU_CYAN : COLOR_TEXT_MUTED);

    /* --- RIGHT STOMP PAD: DAH (-) --- */
    bool dah_active = (state->ctrl.r_mag > 0.55f);
    bool dah_mouse = state->mouse.left_down && (state->mouse.x >= right_card_x && state->mouse.x <= right_card_x + card_w &&
                                                state->mouse.y >= card_y && state->mouse.y <= card_y + card_h);
    bool dah_depressed = dah_active || dah_mouse;

    ui_draw_neu_panel(r, right_card_x, card_y, card_w, card_h, 16, dah_depressed, dah_depressed, NEU_ORANGE);

    /* Recessed banner inside pad */
    ui_draw_neu_panel(r, right_card_x + 20, card_y + 12, banner_w, 24, 12, true, false, (SDL_Color){0});
    font_draw_text(fm, r, "RIGHT ANALOG", right_card_x + card_w / 2, card_y + 24, FONT_SIZE_SMALL, NEU_ORANGE, true, true);

    font_draw_text(fm, r, "DAH ( - )", right_card_x + card_w / 2, card_y + 64, FONT_SIZE_LARGE,
                   dah_depressed ? NEU_ORANGE : COLOR_TEXT_BRIGHT, true, true);
    font_draw_text(fm, r, "Long Tone (820 Hz)", right_card_x + card_w / 2, card_y + 98, FONT_SIZE_SMALL, COLOR_TEXT_MUTED, true, true);

    /* Right stick thumb well */
    ui_draw_neu_well_circle(r, right_card_x + card_w / 2, card_y + 126, 12);
    int r_thumb_x = right_card_x + card_w / 2 + (int)(state->ctrl.rx * 8);
    int r_thumb_y = card_y + 126 + (int)(state->ctrl.ry * 5);
    ui_draw_filled_circle(r, r_thumb_x, r_thumb_y, 5, dah_depressed ? NEU_ORANGE : COLOR_TEXT_MUTED);

    /* --- CENTER DECODED CANDIDATE HUD --- */
    int center_box_w = 110;
    int center_box_h = card_h;
    int center_box_x = state->win_w / 2 - center_box_w / 2;
    bool cand_active = (m->candidate_char != '\0');

    ui_draw_neu_panel(r, center_box_x, card_y, center_box_w, center_box_h, 16, true, cand_active, NEU_GREEN);

    font_draw_text(fm, r, "DECODED", state->win_w / 2, card_y + 22, FONT_SIZE_SMALL, COLOR_TEXT_MUTED, true, true);

    if (m->candidate_char != '\0') {
        char cand_str[2] = { m->candidate_char, '\0' };
        font_draw_text(fm, r, cand_str, state->win_w / 2, card_y + 68, FONT_SIZE_HUGE, NEU_GREEN, true, true);
    } else {
        font_draw_text(fm, r, "?", state->win_w / 2, card_y + 68, FONT_SIZE_HUGE, COLOR_TEXT_MUTED, true, true);
    }

    /* Auto-commit countdown progress bar */
    if (m->seq_len > 0) {
        float progress = m->silence_timer / MORSE_AUTO_COMMIT_TIME;
        if (progress > 1.0f) progress = 1.0f;
        int bar_w = center_box_w - 24;
        int bar_h = 6;
        int bar_x = center_box_x + 12;
        int bar_y = card_y + center_box_h - 22;

        ui_draw_rounded_rect(r, bar_x, bar_y, bar_w, bar_h, 3, (SDL_Color){ 20, 24, 34, 255 }, true);
        ui_draw_rounded_rect(r, bar_x, bar_y, (int)(bar_w * (1.0f - progress)), bar_h, 3, NEU_CYAN, true);
    }

    /* --- ACTIVE MORSE SEQUENCE DISPLAY TUBE --- */
    int seq_pill_w = 420;
    int seq_pill_h = 44;
    int seq_pill_y = card_y + card_h + 16;
    int seq_pill_x = state->win_w / 2 - seq_pill_w / 2;

    ui_draw_neu_panel(r, seq_pill_x, seq_pill_y, seq_pill_w, seq_pill_h, 22, true, (m->seq_len > 0), NEU_GREEN);

    if (m->seq_len > 0) {
        font_draw_text(fm, r, m->sequence, state->win_w / 2, seq_pill_y + seq_pill_h / 2,
                       FONT_SIZE_LARGE, NEU_GREEN, true, true);
    } else {
        font_draw_text(fm, r, "Flick Left ( . ) or Right ( - ) Stick to Begin...", state->win_w / 2,
                       seq_pill_y + seq_pill_h / 2, FONT_SIZE_SMALL, COLOR_TEXT_MUTED, true, true);
    }

    /* --- MORSE ALPHABET CHEAT SHEET CHASSIS --- */
    int cheat_y = seq_pill_y + seq_pill_h + 18;
    int cheat_w = state->win_w - 60;
    if (cheat_w > 920) cheat_w = 920;
    int cheat_h = 160;
    int cheat_x = (state->win_w - cheat_w) / 2;

    /* Sunken Chassis Well */
    ui_draw_neu_panel(r, cheat_x - 10, cheat_y - 10, cheat_w + 20, cheat_h + 20, 16, true, false, (SDL_Color){0});

    int count = 0;
    const MorseEntry* table = morse_get_table(&count);
    int cols = 9;
    int rows = 4;
    int cell_w = cheat_w / cols;
    int cell_h = cheat_h / rows;

    for (int i = 0; i < 36 && i < count; ++i) {
        int col = i % cols;
        int row = i / cols;
        int tx = cheat_x + col * cell_w + 3;
        int ty = cheat_y + row * cell_h + 3;
        int tw = cell_w - 6;
        int th = cell_h - 6;

        bool is_match = (table[i].ch == m->candidate_char);

        if (is_match) {
            ui_draw_neu_panel(r, tx, ty, tw, th, 6, true, true, NEU_GREEN);
        } else {
            ui_draw_neu_panel(r, tx, ty, tw, th, 6, false, false, (SDL_Color){0});
        }

        char entry_str[32];
        snprintf(entry_str, sizeof(entry_str), "%c: %s", table[i].ch, table[i].code);
        SDL_Color entry_col = is_match ? NEU_GREEN : COLOR_TEXT_BRIGHT;
        font_draw_text(fm, r, entry_str, tx + tw / 2, ty + th / 2, FONT_SIZE_SMALL, entry_col, true, true);
    }
}
