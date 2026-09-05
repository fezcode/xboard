#include "text_buffer.h"
#include "morse.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static TextBuffer b;
int main(void) {
    CHECK(text_insert(&b,"hello world"));
    b.current.cursor=5; text_backspace(&b,true); CHECK(strcmp(b.current.text," world")==0);
    CHECK(text_undo(&b)); CHECK(strcmp(b.current.text,"hello world")==0); CHECK(b.current.cursor==5);
    CHECK(text_redo(&b)); CHECK(strcmp(b.current.text," world")==0);
    b.current.selected=true; CHECK(text_insert(&b,"caf\xc3\xa9"));
    text_move(&b,-1); CHECK(b.current.cursor==3); text_move(&b,1); CHECK(b.current.cursor==5);
    text_backspace(&b,false); CHECK(strcmp(b.current.text,"caf")==0); CHECK(text_undo(&b));
    text_clear(&b); CHECK(b.current.len==0); CHECK(text_undo(&b)); CHECK(b.current.len==5);
    CHECK(text_insert(&b,"!")); CHECK(!text_redo(&b));
    memset(&b,0,sizeof(b)); CHECK(text_insert(&b,"abc\nx\ndef"));
    b.current.cursor=2; text_move(&b,2); CHECK(b.current.cursor==5); text_move(&b,2); CHECK(b.current.cursor==7);
    char full[MAX_TEXT_LEN]; memset(full,'x',sizeof(full)-1); full[sizeof(full)-1]=0;
    memset(&b,0,sizeof(b)); CHECK(text_insert(&b,full)); CHECK(!text_insert(&b,"y")); CHECK(b.current.len==4095);
    b.current.selected=true; CHECK(text_insert(&b,"ok")); CHECK(strcmp(b.current.text,"ok")==0);
    for(int i=0;i<100;++i) CHECK(text_insert(&b,"a"));
    CHECK(b.undo_count==HISTORY_DEPTH); for(int i=0;i<HISTORY_DEPTH;++i) CHECK(text_undo(&b)); CHECK(!text_undo(&b));
    int count=0; const MorseEntry* entries=morse_get_table(&count);
    for(int i=0;i<count;++i) CHECK(morse_decode(entries[i].code)==entries[i].ch);
    CHECK(morse_decode("........")==0);
    puts("Text editing, UTF-8 boundaries, capacity, history and Morse checks passed.");
    return 0;
}
