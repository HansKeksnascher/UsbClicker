#include "../include/audio.h"
#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

typedef struct audio_handle_t {
    sound_t *sound;
    uint32_t offset;
} audio_handle_t;

struct audio_t {
    array_t *handles;
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    SDL_AudioDeviceID device;
};

#ifdef __EMSCRIPTEN__
static int audio_apple_fix(void *userdata, SDL_Event *event) {
    int audio_started;
    switch(event->type) {
        case SDL_FINGERDOWN:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_KEYDOWN:
            audio_started = EM_ASM_INT_V({
                if (SDL2.audioContext && SDL2.audioContext.currentTime == 0) {
                    var buffer = SDL2.audioContext.createBuffer(2, 4096, 44100);
                    var source = SDL2.audioContext.createBufferSource();
                    source.buffer = buffer;
                    source.connect(SDL2.audioContext.destination);
                    source.start();
                } else if (SDL2.audioContext && SDL2.audioContext.currentTime != 0) {
                    return 1;
                }
                return 0;
            });
            if (audio_started) {
                SDL_SetEventFilter(NULL, NULL);
                return 0;
            }
            break;
        default:
            break;
    }
    return 1;
}
#endif

static void audio_callback(void *userdata, uint8_t *stream, int32_t len) {
    audio_t *self = userdata;
    iterator_t iterator = array_iterator(self->handles);
    memset(stream, 0, (size_t) len);
    while (iterator_has_next(iterator)) {
        audio_handle_t *handle = iterator_next(iterator);
        if (handle->offset < handle->sound->buffer_size) {
            uint8_t *src = handle->sound->buffer + handle->offset;
            uint32_t count = MIN(handle->sound->buffer_size - handle->offset, (uint32_t) len);
            SDL_MixAudioFormat(stream, src, self->obtained.format, count, SDL_MIX_MAXVOLUME);
            handle->offset += count;
        } else {
            iterator_remove(iterator);
        }
    }
}

audio_t *audio_new() {
#ifdef __EMSCRIPTEN__
    SDL_SetEventFilter(audio_apple_fix, NULL);
#endif
    audio_t *self = malloc_ext(sizeof(*self));
    self->desired = (SDL_AudioSpec) {
            .freq = 44100,
            .format = AUDIO_F32,
            .channels = 2,
            .samples = 4096,
            .callback = audio_callback,
            .userdata = self
    };
    self->device = SDL_OpenAudioDevice(NULL, 0, &self->desired, &self->obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (!self->device) {
#ifdef DEBUG
        printf("audio_new: failed to open audio device\n");
#endif
        free(self);
        return NULL;
    }
    self->handles = array_new(sizeof(audio_handle_t));
    SDL_PauseAudioDevice(self->device, 0);
    return self;
}

sound_t *audio_load_sound(audio_t *self, const char *filename) {
    sound_t *sound = malloc_ext(sizeof(*sound));
    SDL_AudioSpec loaded;
    if (!SDL_LoadWAV(filename, &loaded, &sound->buffer, &sound->buffer_size)) {
        free(sound);
        return NULL;
    }
    SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt, loaded.format, loaded.channels, loaded.freq, self->obtained.format, self->obtained.channels, self->obtained.freq);
    cvt.len = sound->buffer_size;
    cvt.buf = malloc_ext((size_t) (cvt.len * cvt.len_mult));
    memcpy(cvt.buf, sound->buffer, sound->buffer_size);
    SDL_ConvertAudio(&cvt);
    SDL_FreeWAV(sound->buffer);
    sound->buffer = cvt.buf;
    sound->buffer_size = (uint32_t) cvt.len_cvt;
    return sound;
}

void audio_sound_play(audio_t *self, sound_t *sound) {
    SDL_LockAudioDevice(self->device);
    audio_handle_t *handle = array_add_last(self->handles, NULL);
    handle->sound = sound;
    handle->offset = 0;
    SDL_UnlockAudioDevice(self->device);
}

void audio_sound_delete(sound_t *sound) {
    SDL_FreeWAV(sound->buffer);
    free(sound);
}

void audio_delete(audio_t *self) {
    SDL_PauseAudioDevice(self->device, 1);
    SDL_CloseAudioDevice(self->device);
    array_delete(self->handles);
    free(self);
}
