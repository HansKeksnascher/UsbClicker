#include "video_private.h"

font_t *font_load(const char *filename_desc, const char *filename_sprite) {
    FILE *file = fopen(filename_desc, "r");
    if (!file) {
        return NULL;
    }
    sprite_t *sprite = sprite_load(filename_sprite);
    if (!sprite) {
        fclose(file);
        return NULL;
    }
    font_t *self = malloc_ext(sizeof(*self));
    self->sprite = sprite;
    for (int i = 0; i < 128; i++) {
        self->glyphs[i].enabled = false;
    }
    char buffer[1024];
    while (fgets(buffer, 1024, file)) {
        int c;
        int x, y, w, h;
        int x_off, y_off, x_adv;
        if (sscanf(buffer, "char id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d", &c, &x, &y, &w, &h, &x_off, &y_off, &x_adv) != 8) {
            continue;
        }
        if (c >= 0 && c < 127) {
            self->glyphs[c].enabled = true;
            self->glyphs[c].bounds.x = x;
            self->glyphs[c].bounds.y = y;
            self->glyphs[c].bounds.z = w;
            self->glyphs[c].bounds.w = h;
            self->glyphs[c].offset.x = x_off;
            self->glyphs[c].offset.y = y_off;
            self->glyphs[c].x_adv = x_adv;
        }
    }
    fclose(file);
    return self;
}

void video_text(video_t *self, font_t *font, const char *str, float x, float y) {
    video_sprite_begin(self, font->sprite);
    while (*str) {
        glyph_t glyph = font->glyphs[*str];
        if (glyph.enabled) {
            vec4_t dst = {x + glyph.offset.x, y + glyph.offset.y, glyph.bounds.z, glyph.bounds.w};
            video_sprite_item(self, dst, glyph.bounds);
            x += glyph.x_adv;
        }
        str++;
    }
    video_sprite_end(self);
}

void font_delete(font_t *self) {
    sprite_delete(self->sprite);
    free(self);
}
