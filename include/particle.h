#ifndef PARTICLE_H
#define PARTICLE_H

#include "core.h"

struct video_t;

struct emitter_t;
typedef struct emitter_t emitter_t;

typedef struct particle_t {
    vec2_t position;
    vec4_t color;
    vec2_t velocity;
    int lifetime;
} particle_t;

emitter_t *emitter_new();
particle_t *emitter_emit(emitter_t *self, float x, float y);
void emitter_tick(emitter_t *self);
void emitter_draw(emitter_t *self, struct video_t *video);
void emitter_delete(emitter_t *self);

#endif
