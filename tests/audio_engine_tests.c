/* Exercise the actual mixer and policy without opening an audio device. */
#include "../src/audio.c"
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static int active(AudioEngine* a) { int n=0; for(int i=0;i<MAX_VOICES;++i) n+=a->voices[i].active; return n; }
int main(int argc,char**argv) {
    (void)argc; (void)argv;
    AudioEngine a={0}; a.enabled=true;
    audio_play_dah(&a); CHECK(active(&a)==1);
    audio_set_insertion_only(&a,true); CHECK(active(&a)==0);
    audio_play_tone(&a,650,.1f); audio_play_dit(&a); audio_play_dah(&a); audio_play_click(&a);
    CHECK(active(&a)==0);
    Sint16 samples[512]; memset(samples,1,sizeof(samples));
    audio_callback(&a,(Uint8*)samples,sizeof(samples));
    for(int i=0;i<512;++i) CHECK(samples[i]==0);
    audio_play_insert_click(&a); CHECK(active(&a)==1);
    audio_callback(&a,(Uint8*)samples,sizeof(samples));
    int nonzero=0; for(int i=0;i<512;++i) nonzero+=samples[i]!=0; CHECK(nonzero>0);
    audio_set_enabled(&a,false); CHECK(active(&a)==0);
    audio_play_insert_click(&a); CHECK(active(&a)==0);
    audio_set_enabled(&a,true); CHECK(active(&a)==0);
    audio_set_insertion_only(&a,false); audio_play_dit(&a); CHECK(active(&a)==1);
    puts("All generic/Morse tones blocked in quiet dial; mixer silent; insertion permitted; mute clears voices.");
    return 0;
}
