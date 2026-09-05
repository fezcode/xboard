#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

typedef struct AudioEngine AudioEngine;

AudioEngine* audio_init(void);
void audio_shutdown(AudioEngine* audio);

/* Play tone at frequency (Hz) for duration (seconds) with anti-click envelope */
void audio_play_tone(AudioEngine* audio, float freq, float duration);

/* Morse helpers */
void audio_play_dit(AudioEngine* audio);
void audio_play_dah(AudioEngine* audio);
void audio_play_click(AudioEngine* audio);

void audio_set_enabled(AudioEngine* audio, bool enabled);
bool audio_is_enabled(const AudioEngine* audio);

#endif /* AUDIO_H */
