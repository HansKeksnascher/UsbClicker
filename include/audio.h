#ifndef AUDIO_H
#define AUDIO_H

#include "core.h"

struct audio_t;
typedef struct audio_t audio_t;

typedef struct sound_t {
    uint32_t buffer_size;
    uint8_t *buffer;
} sound_t;

audio_t *audio_new();
sound_t *audio_load_sound(audio_t *self, const char *filename);
void audio_sound_play(audio_t *self, sound_t *sound);
void audio_sound_delete(sound_t *sound);
void audio_delete(audio_t *self);

#endif
