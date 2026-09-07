#include "app_shell.h"
#include "phrases.h"
#include "controller.h"
#include "morse.h"
#include "ui_draw.h"
#include "font.h"
#include "audio.h"
#include <stdio.h>
#include <string.h>

static const char* names[] = {"Dual dial", "Keyboard", "Morse code"};
static const char* descriptions[] = {
    "Choose a group with the left stick. Pick a letter with the right.",
    "Left stick: red. Right stick: blue. A types the last moved selection.",
    "A little rhythm goes a long way. Short dots, long dashes."
};
/* Phrase row geometry. The row starts right of the QUICK PHRASES caption and
 * always ends inside the window, however many phrases the user configured. */
#define PHRASE_ROW_X 150
#define PHRASE_ROW_GAP 10
#define PHRASE_MIN_W 86
#define PHRASE_MAX_W 250

static int phrase_width(AppState* s, int index) {
    int tw = 0;
    font_measure_text(s->fonts, phrase_text(s, index), FONT_SIZE_SMALL, &tw, NULL);
    /* Without a font manager, estimate from the text so layout stays deterministic. */
    if (tw <= 0) tw = (int)strlen(phrase_text(s, index)) * 8;
    int w = tw + 36;
    if (w < PHRASE_MIN_W) w = PHRASE_MIN_W;
    if (w > PHRASE_MAX_W) w = PHRASE_MAX_W;
    return w;
}
void shell_phrase_rect(AppState* s, int index, int* out_x, int* out_w) {
    int count = phrase_count(s), total = 0;
    for (int i = 0; i < count; ++i) total += phrase_width(s, i);
    int available = s->win_w - PHRASE_ROW_X - 28 - (count - 1) * PHRASE_ROW_GAP;
    int x = PHRASE_ROW_X;
    for (int i = 0; i < count; ++i) {
        int w = phrase_width(s, i);
        if (total > available && available > 0) w = w * available / total;
        if (i == index) { *out_x = x; *out_w = w; return; }
        x += w + PHRASE_ROW_GAP;
    }
    *out_x = x; *out_w = 0;
}
static bool hit(AppState* s, int x, int y, int w, int h) {
    return s->mouse.x >= x && s->mouse.x < x+w && s->mouse.y >= y && s->mouse.y < y+h;
}
static void label(AppState* s, const char* text, int x, int y, FontSize size, SDL_Color color) {
    font_draw_text(s->fonts, s->renderer, text, x, y, size, color, false, true);
}
static void button(AppState* s, const char* text, int x, int y, int w, bool active) {
    bool hover = hit(s,x,y,w,34) && !s->help_open;
    ui_draw_neu_panel(s->renderer,x,y,w,34,8,active,active || hover,NEU_CYAN);
    font_draw_text(s->fonts,s->renderer,text,x+w/2,y+17,FONT_SIZE_SMALL,active ? NEU_CYAN : COLOR_TEXT_BRIGHT,true,true);
}
/* Defer individual stick-click actions until release, so staggered chord
 * presses never toggle Caps/Symbols as a side effect. Re-arm after both release. */
bool shell_controller_update(AppState* s, float dt) {
    const uint32_t clicks = BTN_LSTICK | BTN_RSTICK;
    uint32_t held = s->ctrl.buttons_held & clicks;
    if (s->help_open) {
        s->stick_click_pending = 0;
        s->stick_chord_latched = held != 0;
        return true;
    }
    if (held == clicks && !s->stick_chord_latched) {
        s->stick_chord_latched = true;
        s->stick_click_pending = 0;
        s->phrases_focused = !s->phrases_focused;
        s->navigation_needs_neutral = true;
        s->phrase_direction = 0;
        s->phrase_repeat_timer = 0;
        app_set_toast(s, s->phrases_focused ? "Quick phrases: stick / D-pad to choose, A to insert" : "Returned to keyboard mode", 2);
        return true;
    }
    if (s->stick_chord_latched) {
        if (!held) s->stick_chord_latched = false;
    } else {
        s->stick_click_pending |= s->ctrl.buttons_pressed & clicks;
        uint32_t released = s->stick_click_pending & ~held;
        if (!s->phrases_focused) {
            if (released & BTN_LSTICK) s->caps_lock = !s->caps_lock;
            if (released & BTN_RSTICK) s->symbols_active = !s->symbols_active;
        }
        s->stick_click_pending &= held;
    }
    bool neutral = SDL_fabsf(s->ctrl.lx) < .45f && SDL_fabsf(s->ctrl.ly) < .45f &&
                   SDL_fabsf(s->ctrl.rx) < .45f && SDL_fabsf(s->ctrl.ry) < .45f &&
                   !(s->ctrl.buttons_held & (BTN_DPAD_LEFT|BTN_DPAD_RIGHT|BTN_DPAD_UP|BTN_DPAD_DOWN));
    if (s->navigation_needs_neutral) {
        if (neutral) s->navigation_needs_neutral = false;
        else return true;
    }
    if (!s->phrases_focused) return false;
    int count = phrase_count(s);
    if (s->phrase_index >= count) s->phrase_index = count - 1;
    int direction = 0;
    if (s->ctrl.lx < -.45f || s->ctrl.rx < -.45f || (s->ctrl.buttons_held & BTN_DPAD_LEFT)) direction = -1;
    else if (s->ctrl.lx > .45f || s->ctrl.rx > .45f || (s->ctrl.buttons_held & BTN_DPAD_RIGHT)) direction = 1;
    if (direction) {
        s->phrase_repeat_timer -= dt;
        if (direction != s->phrase_direction || s->phrase_repeat_timer <= 0) {
            s->phrase_index = (s->phrase_index + direction + count) % count;
            s->phrase_repeat_timer = direction != s->phrase_direction ? .32f : .12f;
        }
    }
    s->phrase_direction = direction;
    if (s->ctrl.buttons_pressed & BTN_A) app_insert_string(s, phrase_text(s, s->phrase_index));
    return true;
}

/* Controller actions that behave the same in every mode.
 *
 * The dial audio policy is re-applied at the end rather than once per frame:
 * LB / RB can change the mode midway through, and any click emitted above must
 * still follow the policy of the mode it came from, which is what keeps dial
 * mode silent outside successful insertion. */
void shell_global_shortcuts(AppState* s, float dt, bool mode_input_blocked) {
    static const char* mode_names[] = { "Mode: Dual Dial", "Mode: Virtual Grid", "Mode: Morse Code" };

    /* LB / RB: switch modes */
    if (s->ctrl.buttons_pressed & BTN_LBUMPER) {
        s->mode = (s->mode == 0) ? (AppMode)(MODE_COUNT - 1) : (AppMode)(s->mode - 1);
        app_set_toast(s, mode_names[s->mode], 2.0f);
        if (s->mode != MODE_DIAL) audio_play_click(s->audio);
        controller_rumble(0.15f, 0.25f, 40);
    }
    if (s->ctrl.buttons_pressed & BTN_RBUMPER) {
        s->mode = (AppMode)((s->mode + 1) % MODE_COUNT);
        app_set_toast(s, mode_names[s->mode], 2.0f);
        if (s->mode != MODE_DIAL) audio_play_click(s->audio);
        controller_rumble(0.15f, 0.25f, 40);
    }

    /* Back / View: Toggle Direct SendInput */
    if (s->ctrl.buttons_pressed & BTN_BACK) {
        app_toggle_direct(s);
        app_set_toast(s, s->direct_send_input ? "Direct SendInput: ON" : "Direct SendInput: OFF", 2.0f);
        if (s->mode != MODE_DIAL) audio_play_click(s->audio);
        controller_rumble(0.30f, 0.30f, 60);
    }

    /* Start / Menu: Copy to Clipboard */
    if (s->ctrl.buttons_pressed & BTN_START) {
        app_copy(s);
    }

    /* LT acts like Shift */
    bool lt_held = (s->ctrl.lt > 0.18f);
    s->shift_active = s->caps_lock ? !lt_held : lt_held;

    /* RT acts like Ctrl */
    s->ctrl_active = s->ctrl_locked || (s->ctrl.rt > 0.18f);

    /* Guide opens Start; L3 is reserved for Caps Lock. */
    s->win_active = s->win_locked;

    if (s->ctrl.buttons_pressed & BTN_GUIDE) {
        app_win_key(s);
    }

    /* D-pad Cursor Movement:
     * Navigates cursor in external apps (Left, Right, Up, Down arrow keys)
     * and moves cursor in text buffer. Supports smooth hold-to-repeat.
     */
    int dpad_dir = 0;
    if (s->ctrl.buttons_held & BTN_DPAD_LEFT)       dpad_dir = 1;
    else if (s->ctrl.buttons_held & BTN_DPAD_RIGHT) dpad_dir = 2;
    else if (s->ctrl.buttons_held & BTN_DPAD_UP)    dpad_dir = 3;
    else if (s->ctrl.buttons_held & BTN_DPAD_DOWN)  dpad_dir = 4;

    if (s->mode == MODE_GRID || mode_input_blocked) dpad_dir = 0;
    if (dpad_dir != 0) {
        if (dpad_dir != s->dpad_last_dir) {
            s->dpad_last_dir = dpad_dir;
            s->dpad_repeat_timer = 0.25f; /* 250ms initial delay */
            if (dpad_dir == 1)      app_cursor_left(s);
            else if (dpad_dir == 2) app_cursor_right(s);
            else if (dpad_dir == 3) app_cursor_up(s);
            else if (dpad_dir == 4) app_cursor_down(s);
        } else {
            s->dpad_repeat_timer -= dt;
            if (s->dpad_repeat_timer <= 0.0f) {
                if (dpad_dir == 1)      app_cursor_left(s);
                else if (dpad_dir == 2) app_cursor_right(s);
                else if (dpad_dir == 3) app_cursor_up(s);
                else if (dpad_dir == 4) app_cursor_down(s);
                s->dpad_repeat_timer = 0.05f; /* 50ms rapid repeat */
            }
        }
        /* Consume D-pad pressed flags so modes don't double process */
        s->ctrl.buttons_pressed &= ~(BTN_DPAD_LEFT | BTN_DPAD_RIGHT | BTN_DPAD_UP | BTN_DPAD_DOWN);
    } else {
        s->dpad_last_dir = 0;
        s->dpad_repeat_timer = 0.0f;
    }

    /* Universal Space */
    if (s->ctrl.buttons_pressed & BTN_X) {
        app_space(s);
    }

    /* Universal Backspace:
     * Press once: delete exactly 1 character (or 1 in-flight Morse symbol).
     * Hold: smooth auto-repeat delete after initial delay.
     */
    bool b_down = (s->ctrl.buttons_held & BTN_B) != 0;
    if (s->ctrl.buttons_pressed & BTN_B) {
        if (s->mode == MODE_MORSE && s->morse.seq_len > 0) {
            s->morse.sequence[--s->morse.seq_len] = '\0';
            s->morse.candidate_char = morse_decode(s->morse.sequence);
            s->morse.silence_timer = 0.0f;
            controller_rumble(0.20f, 0.0f, 35);
        } else {
            if (s->ctrl_active) {
                app_delete_word(s);
            } else {
                app_backspace(s);
            }
        }
        s->b_repeat_timer = 0.40f; /* 400ms initial hold delay */
    } else if (b_down) {
        s->b_repeat_timer -= dt;
        if (s->b_repeat_timer <= 0.0f) {
            if (s->mode == MODE_MORSE && s->morse.seq_len > 0) {
                s->morse.sequence[--s->morse.seq_len] = '\0';
                s->morse.candidate_char = morse_decode(s->morse.sequence);
                s->morse.silence_timer = 0.0f;
            } else {
                if (s->ctrl_active) {
                    app_delete_word(s);
                } else {
                    app_backspace(s);
                }
            }
            s->b_repeat_timer = 0.06f; /* Rapid repeat every 60ms */
        }
    } else {
        s->b_repeat_timer = 0.0f;
    }

    /* Universal Enter */
    if (s->ctrl.buttons_pressed & BTN_Y) {
        app_newline(s);
    }

    audio_set_insertion_only(s->audio, s->mode == MODE_DIAL);
}

void shell_update(AppState* s) {
    if (!s->mouse.left_clicked) return;
    int w=s->win_w, h=s->win_h;
    if (s->help_open) { s->help_open=false; s->mouse.left_clicked=false; return; }
    for (int i=0;i<3;++i) if(hit(s,252+i*134,19,126,34)) { s->mode=(AppMode)i; s->morse.seq_len=0; s->morse.sequence[0]=0; s->morse.candidate_char=0; }
    if(hit(s,w-296,19,88,34)) app_cycle_theme(s);
    if(hit(s,w-198,19,104,34)) { s->sound_enabled=!s->sound_enabled; audio_set_enabled(s->audio,s->sound_enabled); }
    if(hit(s,w-84,19,56,34)) s->help_open=true;
    int x=w-476;
    if(hit(s,x,104,152,34)) app_toggle_direct(s);
    if(hit(s,x+162,104,80,34)) app_undo(s);
    if(hit(s,x+252,104,80,34)) app_redo(s);
    if(hit(s,x+342,104,82,34)) app_copy(s);
    if(hit(s,w-202,170,76,34)) app_paste(s);
    if(hit(s,w-116,170,64,34)) app_clear(s);
    for(int i=0;i<phrase_count(s);++i) {
        int px,bw; shell_phrase_rect(s,i,&px,&bw);
        if(hit(s,px,h-124,bw,34)) app_insert_string(s,phrase_text(s,i));
    }
    int fx=28;
    if(hit(s,fx,h-65,98,34)) { s->caps_lock=!s->caps_lock; s->shift_active=s->caps_lock; }
    if(hit(s,fx+108,h-65,90,34)) { s->ctrl_locked=!s->ctrl_locked; s->ctrl_active=s->ctrl_locked; }
    if(hit(s,fx+208,h-65,114,34)) s->symbols_active=!s->symbols_active;
    if(s->mouse.y<232 || s->mouse.y>=h-140) s->mouse.left_clicked=false;
}
void shell_draw_header(AppState* s) {
    int w=s->win_w;
    ui_draw_rounded_rect(s->renderer,28,22,30,30,9,NEU_CYAN,true);
    font_draw_text(s->fonts,s->renderer,"x",43,36,FONT_SIZE_MEDIUM,NEU_BG,true,true);
    label(s,"xboard",69,35,FONT_SIZE_LARGE,COLOR_TEXT_BRIGHT);
    label(s,"02",186,37,FONT_SIZE_SMALL,COLOR_TEXT_MUTED);
    for(int i=0;i<3;++i) button(s,names[i],252+i*134,19,126,s->mode==(AppMode)i);
    button(s,THEMES[s->theme_index].name,w-296,19,88,false);
    button(s,s->sound_enabled?"Sound on":"Sound off",w-198,19,104,s->sound_enabled);
    button(s,"Help",w-84,19,56,false);
    ui_draw_thick_line(s->renderer,28,72,w-28,72,1,NEU_BORDER);
}
void shell_draw_composer(AppState* s) {
    int w=s->win_w;
    ui_draw_neu_panel(s->renderer,28,90,w-56,136,14,false,false,NEU_CYAN);
    label(s,s->direct_send_input?"LIVE OUTPUT":"COMPOSER",48,120,FONT_SIZE_SMALL,NEU_CYAN);
    char count[80]; int chars=0;
    for(int i=0;i<s->editor.current.len;++i) if(((unsigned char)s->editor.current.text[i]&0xc0)!=0x80) ++chars;
    snprintf(count,sizeof(count),"%d characters%s",chars,s->editor.current.selected?"  /  all selected":"");
    label(s,count,170,120,FONT_SIZE_SMALL,COLOR_TEXT_MUTED);
    int x=w-476;
    button(s,s->direct_send_input?"Direct to apps":"Compose locally",x,104,152,s->direct_send_input);
    button(s,"Undo",x+162,104,80,false); button(s,"Redo",x+252,104,80,false); button(s,"Copy text",x+342,104,82,true);
    button(s,"Paste",w-202,170,76,false); button(s,"Clear",w-116,170,64,false);
    TextSnapshot* t=&s->editor.current;
    char line[MAX_TEXT_LEN]; int start=t->cursor, end=t->cursor;
    while(start>0 && t->text[start-1]!='\n') --start;
    while(end<t->len && t->text[end]!='\n') ++end;
    memcpy(line,t->text+start,(size_t)(end-start)); line[end-start]=0;
    char prefix[MAX_TEXT_LEN]; memcpy(prefix,t->text+start,(size_t)(t->cursor-start)); prefix[t->cursor-start]=0;
    int cw=0; font_measure_text(s->fonts,prefix,FONT_SIZE_MEDIUM,&cw,NULL);
    int available=w-292, scroll=cw>available-10?cw-available+10:0;
    SDL_Rect clip={48,154,available,58}; SDL_RenderSetClipRect(s->renderer,&clip);
    if(t->selected) ui_draw_rounded_rect(s->renderer,48,164,available,38,5,NEU_SURFACE_HIGH,true);
    label(s,t->len?line:"Your next words start here...",48-scroll,186,FONT_SIZE_MEDIUM,t->len?COLOR_TEXT_BRIGHT:COLOR_TEXT_MUTED);
    if((SDL_GetTicks()/500)%2==0) ui_draw_thick_line(s->renderer,48+cw-scroll,174,48+cw-scroll,198,2,NEU_CYAN);
    SDL_RenderSetClipRect(s->renderer,NULL);
    label(s,names[s->mode],32,264,FONT_SIZE_MEDIUM,COLOR_TEXT_BRIGHT);
    label(s,descriptions[s->mode],190,264,FONT_SIZE_SMALL,COLOR_TEXT_MUTED);
    ui_draw_led_indicator(s->renderer,w-182,263,4,s->ctrl.connected,NEU_GREEN);
    label(s,s->ctrl.connected?"Controller ready":"Mouse ready",w-168,264,FONT_SIZE_SMALL,COLOR_TEXT_MUTED);
}
void shell_draw_footer(AppState* s) {
    int w=s->win_w,h=s->win_h;
    label(s,"QUICK PHRASES",28,h-107,FONT_SIZE_SMALL,COLOR_TEXT_MUTED);
    label(s,s->phrases_focused ? "PHRASES FOCUSED  /  A insert  /  L3 + R3 return" : "L3 + R3  Focus quick phrases",150,h-143,FONT_SIZE_SMALL,NEU_CYAN);
    for(int i=0;i<phrase_count(s);++i) {
        int x,bw; shell_phrase_rect(s,i,&x,&bw);
        button(s,phrase_text(s,i),x,h-124,bw,s->phrases_focused && s->phrase_index==i);
    }
    ui_draw_thick_line(s->renderer,28,h-78,w-28,h-78,1,NEU_BORDER);
    button(s,"LT  Shift",28,h-65,98,s->shift_active);
    button(s,"RT  Ctrl",136,h-65,90,s->ctrl_active);
    button(s,"R3  Symbols",236,h-65,114,s->symbols_active);
    label(s,"A  Select     B  Delete     X  Space     Y  Enter",380,h-48,FONT_SIZE_SMALL,COLOR_TEXT_MUTED);
    label(s,"LB / RB   Switch mode",w-202,h-48,FONT_SIZE_SMALL,COLOR_TEXT_MUTED);
}
void shell_draw_overlay(AppState* s) {
    if(s->toast_timer>0 && !s->help_open) {
        int tw=0; font_measure_text(s->fonts,s->toast,FONT_SIZE_SMALL,&tw,NULL);
        ui_draw_neu_panel(s->renderer,(s->win_w-tw-40)/2,s->win_h-181,tw+40,38,10,false,true,NEU_CYAN);
        font_draw_text(s->fonts,s->renderer,s->toast,s->win_w/2,s->win_h-162,FONT_SIZE_SMALL,COLOR_TEXT_BRIGHT,true,true);
    }
    if(!s->help_open) return;
    ui_draw_rounded_rect(s->renderer,0,0,s->win_w,s->win_h,0,(SDL_Color){0,0,0,190},true);
    int x=(s->win_w-680)/2,y=185;
    ui_draw_neu_panel(s->renderer,x,y,680,466,18,false,false,NEU_CYAN);
    label(s,"Make yourself comfortable.",x+32,y+46,FONT_SIZE_LARGE,COLOR_TEXT_BRIGHT);
    const char* lines[]={
        "Compose locally, then copy your text into any app.",
        "Direct to apps types into the currently focused window.",
        "A  Select     B  Backspace (hold to repeat)     X  Space     Y  Enter",
        "LB / RB  Switch mode     LT  Shift     RT  Ctrl shortcuts",
        "L3 + R3  Quick phrases     A  Insert phrase     L3 + R3  Return",
        "L3 alone  Caps     R3 alone  Symbols     Menu  Copy composer",
        "Keyboard: 1 / 2 / 3 modes, F1 help, F2 theme, Esc close",
        "Undo / Redo restore local edits; in direct mode they go to the app.",
        "Theme, mode and sound are saved. Your text is never saved."
    };
    for(int i=0;i<9;++i) label(s,lines[i],x+32,y+102+i*33,FONT_SIZE_SMALL,COLOR_TEXT_MUTED);
    label(s,"Click anywhere to return",x+32,y+428,FONT_SIZE_SMALL,NEU_CYAN);
}
