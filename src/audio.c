#include "audio.h"
#include <SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 44100
#define MORSE_FREQ 820.0f
#define DIT_DURATION 0.075f   /* 75 ms - crisp & punchy */
#define DAH_DURATION 0.225f   /* 225 ms - 3x standard dit */
#define CLICK_DURATION 0.015f /* 15 ms */

#define MAX_VOICES 4

typedef struct {
    bool active;
    float freq;
    int total_samples;
    int samples_elapsed;
    int attack_samples;
    int decay_samples;
    float phase;
    float gain;
} AudioVoice;

struct AudioEngine {
    SDL_AudioDeviceID device;
    bool enabled;

    SDL_SpinLock lock;
    AudioVoice voices[MAX_VOICES];

    /* DC-blocking high-pass filter state (cleans sub-bass boom/thud) */
    float dc_x_prev;
    float dc_y_prev;
};

static void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioEngine* audio = (AudioEngine*)userdata;
    Sint16* buffer = (Sint16*)stream;
    int sample_count = len / sizeof(Sint16);

    SDL_AtomicLock(&audio->lock);
    bool enabled = audio->enabled;
    if (!enabled) {
        SDL_AtomicUnlock(&audio->lock);
        memset(stream, 0, len);
        return;
    }

    /* Copy active voices locally to minimize lock contention */
    AudioVoice local_voices[MAX_VOICES];
    memcpy(local_voices, audio->voices, sizeof(local_voices));
    float dc_x = audio->dc_x_prev;
    float dc_y = audio->dc_y_prev;
    SDL_AtomicUnlock(&audio->lock);

    bool any_active = false;
    for (int v = 0; v < MAX_VOICES; ++v) {
        if (local_voices[v].active) {
            any_active = true;
            break;
        }
    }

    if (!any_active) {
        memset(stream, 0, len);
        return;
    }

    for (int i = 0; i < sample_count; ++i) {
        float mix = 0.0f;

        for (int v = 0; v < MAX_VOICES; ++v) {
            AudioVoice* voice = &local_voices[v];
            if (!voice->active) continue;

            if (voice->samples_elapsed < voice->total_samples) {
                int elapsed = voice->samples_elapsed;
                int remaining = voice->total_samples - elapsed;

                /* Cosine (Hann) envelope: C1 continuous, zero slope at start/end
                 * Eliminates all click/pop/boom transients */
                float env = 1.0f;
                if (elapsed < voice->attack_samples && voice->attack_samples > 0) {
                    float t = (float)elapsed / (float)voice->attack_samples;
                    env = 0.5f * (1.0f - cosf((float)M_PI * t));
                }
                if (remaining < voice->decay_samples && voice->decay_samples > 0) {
                    float t = (float)remaining / (float)voice->decay_samples;
                    env *= 0.5f * (1.0f - cosf((float)M_PI * t));
                }

                /* Telegraph presence harmonics: clean fundamental + subtle 3rd harmonic */
                float s1 = sinf(voice->phase);
                float s3 = sinf(3.0f * voice->phase);
                float tone = 0.80f * s1 + 0.20f * s3;

                mix += tone * env * voice->gain;

                float phase_inc = (2.0f * (float)M_PI * voice->freq) / (float)SAMPLE_RATE;
                voice->phase += phase_inc;
                if (voice->phase >= 2.0f * (float)M_PI) {
                    voice->phase -= 2.0f * (float)M_PI;
                }
                voice->samples_elapsed++;
            } else {
                voice->active = false;
            }
        }

        /* 1st-Order DC-Blocking High-Pass Filter (~25 Hz)
         * Strips any subsonic DC drift or bass boom that can thump subwoofers/headphones */
        float filtered = mix - dc_x + 0.996f * dc_y;
        dc_x = mix;
        dc_y = filtered;

        /* Soft-knee saturation limiter to prevent digital hard-clipping */
        if (filtered > 0.90f) {
            filtered = 0.90f + 0.10f * tanhf((filtered - 0.90f) / 0.10f);
        } else if (filtered < -0.90f) {
            filtered = -0.90f + 0.10f * tanhf((filtered + 0.90f) / 0.10f);
        }

        buffer[i] = (Sint16)(filtered * 32767.0f);
    }

    /* Write back voice states and filter state */
    SDL_AtomicLock(&audio->lock);
    for (int v = 0; v < MAX_VOICES; ++v) {
        audio->voices[v].samples_elapsed = local_voices[v].samples_elapsed;
        audio->voices[v].phase = local_voices[v].phase;
        if (!local_voices[v].active) {
            audio->voices[v].active = false;
        }
    }
    audio->dc_x_prev = dc_x;
    audio->dc_y_prev = dc_y;
    SDL_AtomicUnlock(&audio->lock);
}

AudioEngine* audio_init(void) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        SDL_Log("[Audio] Failed to init audio subsystem: %s", SDL_GetError());
        return NULL;
    }

    AudioEngine* audio = (AudioEngine*)calloc(1, sizeof(AudioEngine));
    if (!audio) return NULL;

    audio->enabled = true;

    SDL_AudioSpec wanted, obtained;
    SDL_zero(wanted);
    wanted.freq = SAMPLE_RATE;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 1;
    wanted.samples = 512;
    wanted.callback = audio_callback;
    wanted.userdata = audio;

    audio->device = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
    if (audio->device == 0) {
        SDL_Log("[Audio] Failed to open audio device: %s", SDL_GetError());
        free(audio);
        return NULL;
    }

    SDL_PauseAudioDevice(audio->device, 0); /* Unpause */
    return audio;
}

void audio_shutdown(AudioEngine* audio) {
    if (!audio) return;
    if (audio->device > 0) {
        SDL_CloseAudioDevice(audio->device);
    }
    free(audio);
}

void audio_play_tone(AudioEngine* audio, float freq, float duration) {
    if (!audio || !audio->enabled) return;
    int count = (int)(SAMPLE_RATE * duration);
    if (count <= 0) return;

    SDL_AtomicLock(&audio->lock);
    /* Find an inactive voice */
    int slot = -1;
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (!audio->voices[i].active) {
            slot = i;
            break;
        }
    }

    /* If all voices are active, steal the one that has elapsed the most */
    if (slot < 0) {
        int max_elapsed = -1;
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (audio->voices[i].samples_elapsed > max_elapsed) {
                max_elapsed = audio->voices[i].samples_elapsed;
                slot = i;
            }
        }
    }

    if (slot >= 0) {
        AudioVoice* v = &audio->voices[slot];
        v->active = true;
        v->freq = freq;
        v->total_samples = count;
        v->samples_elapsed = 0;
        /* Attack & decay scaled to duration:
         * 3ms attack for short tones, capped at 30% of total duration */
        int max_ramp = count / 3;
        int attack = (int)(SAMPLE_RATE * 0.003f);
        int decay = (int)(SAMPLE_RATE * 0.004f);
        if (attack > max_ramp) attack = max_ramp;
        if (decay > max_ramp) decay = max_ramp;
        if (attack < 4) attack = 4;
        if (decay < 4) decay = 4;

        v->attack_samples = attack;
        v->decay_samples = decay;
        v->phase = 0.0f; /* Start clean at zero crossing with 0 amplitude! */

        /* Balance gain: Morse tones get 0.50f, higher clicks get 0.35f */
        if (freq >= 1000.0f) {
            v->gain = 0.35f;
        } else {
            v->gain = 0.50f;
        }
    }
    SDL_AtomicUnlock(&audio->lock);
}

void audio_play_dit(AudioEngine* audio) {
    audio_play_tone(audio, MORSE_FREQ, DIT_DURATION);
}

void audio_play_dah(AudioEngine* audio) {
    audio_play_tone(audio, MORSE_FREQ, DAH_DURATION);
}

void audio_play_click(AudioEngine* audio) {
    audio_play_tone(audio, 1100.0f, CLICK_DURATION);
}

void audio_set_enabled(AudioEngine* audio, bool enabled) {
    if (audio) audio->enabled = enabled;
}

bool audio_is_enabled(const AudioEngine* audio) {
    return audio ? audio->enabled : false;
}
