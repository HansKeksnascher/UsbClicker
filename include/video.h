#ifndef VIDEO_H
#define VIDEO_H

#include "core.h"

typedef enum {
    VIDEO_DOT,
    VIDEO_STROKE,
    VIDEO_FILL
} video_mode;

struct video_t;
typedef struct video_t video_t;

typedef struct sprite_t {
    unsigned int texture;
    int w, h;
} sprite_t;

typedef struct glyph_t {
    bool enabled;
    vec4_t bounds;
    vec2_t offset;
    float x_adv;
} glyph_t;

typedef struct font_t {
    sprite_t *sprite;
    glyph_t glyphs[128];
} font_t;

typedef struct emitter_t {
    array_t *particles;
    sprite_t *sprite;
} emitter_t;

typedef struct particle_t {
    vec2_t position;
    vec4_t color;
    vec2_t velocity;
    int lifetime;
} particle_t;

video_t *video_new();
void video_cfg_color(video_t *self, vec4_t color);
void video_cfg_mode(video_t *self, video_mode mode);
void video_clear(video_t *self);
void video_rectangle(video_t *self, float x, float y, float w, float h);
void video_triangle(video_t *self, float x0, float y0, float x1, float y1, float x2, float y2);
void video_delete(video_t *self);

sprite_t *sprite_load(const char *filename);
sprite_t *sprite_load_img(const char *filename);
sprite_t *sprite_load_dds(const char *filename);
void video_sprite_begin(video_t *self, sprite_t *sprite);
void video_sprite_item(video_t *self, vec4_t dst, vec4_t src);
void video_sprite_end(video_t *self);
void video_sprite(video_t *self, sprite_t *sprite, float x, float y);
void video_sprite_ext(video_t *self, sprite_t *sprite, vec4_t dst, vec4_t src);
void sprite_delete(sprite_t *self);

font_t *font_load(const char *filename_desc, const char *filename_sprite);
void video_text(video_t *self, font_t *font, const char *str, float x, float y);
void font_delete(font_t *self);

emitter_t *emitter_new(sprite_t *sprite);
particle_t *emitter_emit(emitter_t *self, float x, float y);
void emitter_tick(emitter_t *self);
void emitter_draw(emitter_t *self, struct video_t *video);
void emitter_delete(emitter_t *self);

#endif
