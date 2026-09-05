#include "app_state.h"
#include "app_shell.h"
#include "ui_draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
void mode_grid_update(AppState*,float);
void mode_morse_update(AppState*,float);
static AppState s;
static void click(int x,int y) { s.mouse.x=x; s.mouse.y=y; s.mouse.left_clicked=true; shell_update(&s); }
int main(int argc,char**argv) {
    (void)argc; (void)argv;
    theme_init(); s.win_w=1200; s.win_h=900; s.mode=MODE_GRID;
    app_insert_string(&s,"first"); app_insert_string(&s," second");
    s.ctrl_active=true; app_insert_char(&s,'z'); CHECK(strcmp(s.editor.current.text,"first")==0);
    app_insert_char(&s,'y'); CHECK(strcmp(s.editor.current.text,"first second")==0);
    app_insert_char(&s,'a'); CHECK(s.editor.current.selected);
    s.ctrl_active=false; app_insert_char(&s,'q'); CHECK(strcmp(s.editor.current.text,"q")==0);
    click(180,790); CHECK(strcmp(s.editor.current.text,"qHello! ")==0);
    click(920,120); CHECK(strcmp(s.editor.current.text,"q")==0);
    click(1000,120); CHECK(strcmp(s.editor.current.text,"qHello! ")==0);
    click(550,30); CHECK(s.mode==MODE_MORSE);
    click(400,30); CHECK(s.mode==MODE_GRID);
    click(1140,30); CHECK(s.help_open); click(180,790); CHECK(!s.help_open);
    CHECK(strcmp(s.editor.current.text,"qHello! ")==0);
    s.mouse.left_clicked=false; s.mouse.x=200; s.mouse.y=330; s.mouse_moved=false;
    s.ctrl.buttons_held=BTN_DPAD_RIGHT; mode_grid_update(&s,.016f); CHECK(s.grid.selectors[0].col==1);
    s.ctrl.buttons_held=0; s.ctrl.buttons_pressed=BTN_A; mode_grid_update(&s,.016f);
    CHECK(s.editor.current.text[s.editor.current.len-1]=='1');
    s.ctrl.buttons_pressed=0; s.mode=MODE_MORSE; strcpy(s.morse.sequence,".-"); s.morse.seq_len=2;
    mode_morse_update(&s,.8f); CHECK(s.editor.current.text[s.editor.current.len-1]=='a');
    CHECK(s.morse.seq_len==0);
    /* Independent stick selectors and most recently moved selection. */
    memset(&s,0,sizeof(s)); s.win_w=1200; s.win_h=900; s.mode=MODE_GRID;
    s.ctrl.lx=1; mode_grid_update(&s,.016f);
    CHECK(s.grid.selectors[0].col==1 && s.grid.selectors[1].col==7 && s.grid.active==0);
    s.ctrl.lx=0; s.ctrl.rx=1; s.ctrl.buttons_pressed=BTN_A; mode_grid_update(&s,.016f);
    CHECK(s.grid.selectors[0].col==1 && s.grid.selectors[1].col==8 && s.grid.active==1);
    CHECK(strcmp(s.editor.current.text,"8")==0);
    s.ctrl.rx=0; s.ctrl.lx=1; mode_grid_update(&s,.016f);
    CHECK(s.grid.active==0 && strcmp(s.editor.current.text,"82")==0);
    s.ctrl.buttons_pressed=0; s.ctrl.rx=1; mode_grid_update(&s,.32f);
    CHECK(s.grid.active==1); /* fresh right gesture wins over left repeat */
    s.ctrl.lx=s.ctrl.rx=0; mode_grid_update(&s,.016f);
    s.grid.selectors[0].col=s.grid.selectors[1].col=2;
    s.ctrl.buttons_pressed=BTN_A; mode_grid_update(&s,.016f);
    CHECK(strcmp(s.editor.current.text,"822")==0); /* overlap types once */
    /* Staggered chord: no accidental single-stick modifiers. */
    memset(&s,0,sizeof(s)); s.win_w=1200; s.win_h=900;
    s.ctrl.buttons_held=s.ctrl.buttons_pressed=BTN_LSTICK;
    CHECK(!shell_controller_update(&s,.016f)); CHECK(!s.caps_lock);
    s.ctrl.buttons_held=BTN_LSTICK|BTN_RSTICK; s.ctrl.buttons_pressed=BTN_RSTICK;
    CHECK(shell_controller_update(&s,.016f)); CHECK(s.phrases_focused);
    s.ctrl.buttons_pressed=0;
    for(int i=0;i<20;++i) CHECK(shell_controller_update(&s,.016f));
    CHECK(s.phrases_focused && !s.caps_lock && !s.symbols_active);
    s.ctrl.buttons_held=0; CHECK(shell_controller_update(&s,.016f));
    s.ctrl.rx=1; CHECK(shell_controller_update(&s,.016f)); CHECK(s.phrase_index==1);
    s.ctrl.rx=0; s.ctrl.buttons_pressed=BTN_A;
    CHECK(shell_controller_update(&s,.016f)); CHECK(strcmp(s.editor.current.text,"Thank you. ")==0);
    mode_grid_update(&s,.016f); CHECK(strcmp(s.editor.current.text,"Thank you. ")==0);
    s.ctrl.buttons_held=s.ctrl.buttons_pressed=BTN_LSTICK|BTN_RSTICK;
    CHECK(shell_controller_update(&s,.016f)); CHECK(!s.phrases_focused);
    s.ctrl.buttons_held=BTN_LSTICK; s.ctrl.buttons_pressed=0; shell_controller_update(&s,.016f);
    s.ctrl.buttons_held=BTN_LSTICK|BTN_RSTICK; shell_controller_update(&s,.016f);
    CHECK(!s.phrases_focused); /* both must release before chord re-arms */
    s.ctrl.buttons_held=0; shell_controller_update(&s,.016f);
    s.ctrl.buttons_held=s.ctrl.buttons_pressed=BTN_LSTICK; shell_controller_update(&s,.016f);
    s.ctrl.buttons_held=s.ctrl.buttons_pressed=0; shell_controller_update(&s,.016f);
    CHECK(s.caps_lock && !s.symbols_active);
    puts("Composer shortcuts, toolbar, phrases, help, grid navigation and Morse commit passed.");
    return 0;
}
