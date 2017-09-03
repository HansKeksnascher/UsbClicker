#include "video_private.h"

sprite_t *sprite_load(const char *filename) {
    char *ext = strstr(filename, ".");
    if (!strcmp(ext, ".dds")) {
        return sprite_load_dds(filename);
    }
    return sprite_load_img(filename);
}

sprite_t *sprite_load_img(const char *filename) {
    SDL_Surface *src = IMG_Load(filename);
    if (!src) {
        return NULL;
    }
    SDL_Surface *dst = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(src);
    if (!dst) {
        return NULL;
    }
    sprite_t *self = malloc_ext(sizeof(*self));
    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dst->w, dst->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    self->w = dst->w;
    self->h = dst->h;
    SDL_FreeSurface(dst);
    return self;
}

sprite_t *sprite_load_dds(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }
    uint32_t magic;
    fread(&magic, sizeof(uint32_t), 1, file);
    if (magic != DDS_MAGIC) {
        fclose(file);
        return NULL;
    }
    dds_header_t header;
    fread(&header, sizeof(dds_header_t), 1, file);
#ifdef __EMSCRIPTEN__
    if (!is_pot(header.w) || !is_pot(header.h)) {
        printf("sprite_load_dds: %s is a Non-Power-Of-Two texture, it will most likely don't work\n", filename);
    }
#endif
    GLenum format;
    size_t block_size;
    switch (header.format.four_cc) {
        case DDS_FOURCC_DXT1:
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            block_size = 8;
            break;
        case DDS_FOURCC_DXT3:
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            block_size = 16;
            break;
        case DDS_FOURCC_DXT5:
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            block_size = 16;
            break;
        default:
            fclose(file);
            return NULL;
    }
    size_t buffer_size = header.mip_map_count > 1 ? 2 * header.pitch : header.pitch;
    char *buffer = malloc_ext(buffer_size * sizeof(char));
    fread(buffer, sizeof(char), buffer_size, file);
    fclose(file);
    sprite_t *self = malloc_ext(sizeof(*self));
    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
    if (header.mip_map_count > 0) {
        size_t offset = 0;
        uint32_t w = header.w;
        uint32_t h = header.h;
        for (int level = 0; level < header.mip_map_count && w > 0 && h > 0; level++) {
            size_t size = ((w + 3) / 4) * ((h + 3) / 4) * block_size;
            glCompressedTexImage2D(GL_TEXTURE_2D, level, format, w, h, 0, (GLsizei) size, buffer + offset);
            offset += size;
            w /= 2;
            h /= 2;
        }
    } else {
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, header.w, header.h, 0, (GLsizei) buffer_size, buffer);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    self->w = header.w;
    self->h = header.h;
    free(buffer);
    return self;
}

void video_sprite_begin(video_t *self, sprite_t *sprite) {
    video_env_set(self, &self->env_textured);
    video_data_clear(self);
    self->batch_size = 0;
    self->batch_sx = 1.0f / sprite->w;
    self->batch_sy = 1.0f / sprite->h;
    glBindTexture(GL_TEXTURE_2D, sprite->texture);
}

void video_sprite_item(video_t *self, vec4_t dst, vec4_t src) {
    float s_min = self->batch_sx * src.x;
    float s_max = self->batch_sx * (src.x + src.z);
    float t_min = self->batch_sy * src.y;
    float t_max = self->batch_sy * (src.y + src.w);
    video_data_put4(self, dst.x, dst.y + dst.w, s_min, t_max);
    video_data_put4(self, dst.x, dst.y, s_min, t_min);
    video_data_put4(self, dst.x + dst.z, dst.y, s_max, t_min);
    video_data_put4(self, dst.x, dst.y + dst.w, s_min, t_max);
    video_data_put4(self, dst.x + dst.z, dst.y, s_max, t_min);
    video_data_put4(self, dst.x + dst.z, dst.y + dst.w, s_max, t_max);
    self->batch_size++;
}

void video_sprite_end(video_t *self) {
    video_data_send(self);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) (6 * self->batch_size));
}

void video_sprite(video_t *self, sprite_t *sprite, float x, float y) {
    vec4_t dst = {x, y, sprite->w, sprite->h};
    vec4_t src = {0, 0, sprite->w, sprite->h};
    video_sprite_ext(self, sprite, dst, src);
}

void video_sprite_ext(video_t *self, sprite_t *sprite, vec4_t dst, vec4_t src) {
    video_sprite_begin(self, sprite);
    video_sprite_item(self, dst, src);
    video_sprite_end(self);
}

void sprite_delete(sprite_t *self) {
    glDeleteTextures(1, &self->texture);
    free(self);
}
