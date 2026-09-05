#include "app_state.h"
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static int clicks;
void audio_play_click(AudioEngine* audio) { (void)audio; ++clicks; }
void audio_play_insert_click(AudioEngine* audio) { (void)audio; ++clicks; }
void mode_dial_update(AppState*,float);
static AppState s;
int main(int argc,char**argv) {
    (void)argc; (void)argv;
    s.mode=MODE_DIAL; s.win_w=1200; s.win_h=900;
    s.dial.active_sector=s.dial.active_pick=-1;
    for (int i=0;i<100;++i) {
        s.ctrl.lx=(i%2)?1:-1; s.ctrl.ly=0; s.ctrl.l_mag=1;
        s.ctrl.rx=0; s.ctrl.ry=(i%2)?1:-1; s.ctrl.r_mag=.5f;
        mode_dial_update(&s,.1f);
    }
    CHECK(clicks==0 && s.editor.current.len==0);
    s.ctrl.lx=0; s.ctrl.ly=-1; s.ctrl.ry=-1; mode_dial_update(&s,.016f);
    s.ctrl.r_mag=1; mode_dial_update(&s,.016f);
    CHECK(clicks==1 && strcmp(s.editor.current.text,"a")==0);
    for(int i=0;i<20;++i) mode_dial_update(&s,.1f);
    CHECK(clicks==1 && s.editor.current.len==1); /* held stick never repeats */
    s.ctrl.r_mag=0; mode_dial_update(&s,.016f);
    s.ctrl.r_mag=1; s.ctrl.buttons_pressed=BTN_A; mode_dial_update(&s,.016f);
    CHECK(clicks==2 && strcmp(s.editor.current.text,"aa")==0); /* flick + A once */
    s.ctrl.buttons_pressed=0;
    memset(&s.editor,0,sizeof(s.editor)); clicks=0;
    s.ctrl.l_mag=s.ctrl.r_mag=0; mode_dial_update(&s,.016f);
    s.ctrl.buttons_pressed=BTN_A; mode_dial_update(&s,.016f);
    CHECK(clicks==1 && strcmp(s.editor.current.text,"a")==0);
    s.ctrl.buttons_pressed=0; app_backspace(&s); CHECK(clicks==1);
    s.mouse.x=420; s.mouse.y=390; s.mouse.left_clicked=true;
    mode_dial_update(&s,.016f); CHECK(clicks==1); /* group selection */
    s.mouse.x=780; s.mouse.y=390;
    mode_dial_update(&s,.016f); CHECK(clicks==2); /* explicit character click */
    s.mouse.left_clicked=false; s.ctrl_active=true; s.ctrl.buttons_pressed=BTN_A;
    mode_dial_update(&s,.016f); CHECK(clicks==2); /* shortcut, not text */
    s.editor.current.selected=false;
    s.ctrl_active=false; memset(s.editor.current.text,'x',MAX_TEXT_LEN-1);
    s.editor.current.text[MAX_TEXT_LEN-1]=0;
    s.editor.current.len=s.editor.current.cursor=MAX_TEXT_LEN-1;
    mode_dial_update(&s,.016f); CHECK(clicks==2); /* rejected insertion */
    puts("Dial navigation silent; flick typing restored; only successful insertion clicks.");
    return 0;
}
