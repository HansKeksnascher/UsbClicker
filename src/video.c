#include "../include/video.h"
#include "../include/ctx.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#ifdef __EMSCRIPTEN__
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <epoxy/gl.h>
#endif

#define VIDEO_BUFFER_SIZE 8192

typedef enum {
    VIDEO_PRIMITIVE,
    VIDEO_TEXTURED
} video_clazz;

typedef struct video_cfg_t {
    video_mode mode;
    mat4_t projection;
    vec4_t color;
} video_cfg_t;

typedef struct video_env_t {
    video_clazz clazz;
    GLuint vao;
    GLuint program;
    GLuint attrib_position;
    GLuint attrib_tex_coord;
    GLuint uniform_projection;
    GLuint uniform_color;
    bool dirty;
} video_env_t;

struct video_t {
    float *buffer;
    size_t buffer_size;
    GLuint vbo;
    video_env_t env_primitive;
    video_env_t env_textured;
    video_env_t *env;
    array_t *configs;
    size_t batch_size;
    float batch_sx;
    float batch_sy;
};

#define DDS_MAGIC       0x20534444
#define DDS_FOURCC_DXT1 0x31545844
#define DDS_FOURCC_DXT3 0x33545844
#define DDS_FOURCC_DXT5 0x35545844

typedef struct dds_pixel_format_t {
    uint32_t size;
    uint32_t flags;
    uint32_t four_cc;
    uint32_t bit_count;
    uint32_t r_mask;
    uint32_t g_mask;
    uint32_t b_mask;
    uint32_t a_mask;
} dds_pixel_format_t;

typedef struct dds_header_t {
    uint32_t size;
    uint32_t flags;
    uint32_t h;
    uint32_t w;
    uint32_t pitch;
    uint32_t depth;
    uint32_t mip_map_count;
    uint32_t reserved0[11];
    dds_pixel_format_t format;
    uint32_t caps[4];
    uint32_t reserved1;
} dds_header_t;

static GLuint video_shader_load(GLenum type, const char *filename) {
    char *buffer = malloc_ext(1024 * sizeof(char));
    if (!file_read(filename, buffer, 1024)) {
        free(buffer);
        return 0;
    }
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char**) &buffer, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        glGetShaderInfoLog(shader, 1024 * sizeof(char), NULL, buffer);
        printf("%s: %s\n", filename, buffer);
    }
    free(buffer);
    return shader;
}

static void video_env_init(video_clazz clazz, video_env_t *env) {
    env->clazz = clazz;
    env->program = glCreateProgram();
    switch (env->clazz) {
        case VIDEO_PRIMITIVE:
            glAttachShader(env->program, video_shader_load(GL_FRAGMENT_SHADER, "asset/shader/primitive.frag"));
            glAttachShader(env->program, video_shader_load(GL_VERTEX_SHADER, "asset/shader/primitive.vert"));
            break;
        case VIDEO_TEXTURED:
            glAttachShader(env->program, video_shader_load(GL_FRAGMENT_SHADER, "asset/shader/textured.frag"));
            glAttachShader(env->program, video_shader_load(GL_VERTEX_SHADER, "asset/shader/textured.vert"));
            break;
    }
    glLinkProgram(env->program);
    GLint status;
    glGetProgramiv(env->program, GL_LINK_STATUS, &status);
    if (!status) {
        char buffer[1024];
        glGetProgramInfoLog(env->program, sizeof(buffer), NULL, buffer);
        printf("%s\n", buffer);
    }
    env->attrib_position = (GLuint) glGetAttribLocation(env->program, "position");
    env->attrib_tex_coord = (GLuint) glGetAttribLocation(env->program, "texCoord");
    env->uniform_projection = (GLuint) glGetUniformLocation(env->program, "projection");
    env->uniform_color = (GLuint) glGetUniformLocation(env->program, "color");
    glGenVertexArraysOES(1, &env->vao);
    glBindVertexArrayOES(env->vao);
    switch (env->clazz) {
        case VIDEO_PRIMITIVE:
            glEnableVertexAttribArray(env->attrib_position);
            glVertexAttribPointer(env->attrib_position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
            break;
        case VIDEO_TEXTURED:
            glEnableVertexAttribArray(env->attrib_position);
            glEnableVertexAttribArray(env->attrib_tex_coord);
            glVertexAttribPointer(env->attrib_position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
            glVertexAttribPointer(env->attrib_tex_coord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));
            break;
    }
}

static GLenum video_env_set(video_t *self, video_env_t *env) {
    if (self->env != env) {
        glUseProgram(env->program);
        glBindVertexArrayOES(env->vao);
        self->env = env;
    }
    video_cfg_t *cfg = array_get_last(self->configs);
    if (env->dirty) {
        glUniformMatrix4fv(env->uniform_projection, 1, GL_FALSE, mat4_transpose(cfg->projection).ptr);
        glUniform4fv(env->uniform_color, 1, cfg->color.ptr);
        env->dirty = false;
    }
    switch (cfg->mode) {
        case VIDEO_DOT:
            return GL_POINTS;
        case VIDEO_STROKE:
            return GL_LINE_LOOP;
        default:
            break;
    }
    return GL_TRIANGLE_FAN;
}

static void video_env_shutdown(video_env_t *env) {
    glDeleteVertexArraysOES(1, &env->vao);
    glDeleteProgram(env->program);
}

static void video_data_clear(video_t *self) {
    self->buffer_size = 0;
}

static void video_data_put2(video_t *self, float p0, float p1) {
    size_t i = self->buffer_size;
    self->buffer[i] = p0;
    self->buffer[i + 1] = p1;
    self->buffer_size += 2;
}

static void video_data_put4(video_t *self, float p0, float p1, float p2, float p3) {
    size_t i = self->buffer_size;
    self->buffer[i] = p0;
    self->buffer[i + 1] = p1;
    self->buffer[i + 2] = p2;
    self->buffer[i + 3] = p3;
    self->buffer_size += 4;
}

static void video_data_send(video_t *self) {
    glBufferSubData(GL_ARRAY_BUFFER, 0, self->buffer_size * sizeof(float), self->buffer);
}

static void video_mark_dirty(video_t *self) {
    self->env_primitive.dirty = true;
    self->env_textured.dirty = true;
}

video_t *video_new() {
    video_t *self = malloc_ext(sizeof(*self));
    self->buffer = malloc_ext(VIDEO_BUFFER_SIZE * sizeof(float));
    glGenBuffers(1, &self->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
    glBufferData(GL_ARRAY_BUFFER, VIDEO_BUFFER_SIZE * sizeof(float), NULL, GL_STREAM_DRAW);
    video_env_init(VIDEO_PRIMITIVE, &self->env_primitive);
    video_env_init(VIDEO_TEXTURED, &self->env_textured);
    self->env = NULL;
    vec2_t viewport = ctx_viewport();
    self->configs = array_new(sizeof(video_cfg_t));
    video_cfg_t *cfg = array_add_last(self->configs, NULL);
    cfg->mode = VIDEO_FILL;
    cfg->projection = mat4_ortho(0, viewport.x, viewport.y, 0, -1, 127);
    cfg->color = COLOR_RGBA(255, 255, 255, 255);
    glFrontFace(GL_CW);
    glViewport(0, 0, (GLsizei) viewport.x, (GLsizei) viewport.y);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    video_mark_dirty(self);
    return self;
}

void video_cfg_color(video_t *self, vec4_t color) {
    video_cfg_t *cfg = array_get_last(self->configs);
    cfg->color = color;
    video_mark_dirty(self);
}

void video_cfg_mode(video_t *self, video_mode mode) {
    video_cfg_t *cfg = array_get_last(self->configs);
    cfg->mode = mode;
}

void video_clear(video_t *self) {
    glClear(GL_COLOR_BUFFER_BIT);
}

void video_rectangle(video_t *self, float x, float y, float w, float h) {
    GLenum mode = video_env_set(self, &self->env_primitive);
    video_data_clear(self);
    video_data_put2(self, x, y);
    video_data_put2(self, x + w, y);
    video_data_put2(self, x + w, y + h);
    video_data_put2(self, x, y + h);
    video_data_send(self);
    glDrawArrays(mode, 0, 4);
}

void video_triangle(video_t *self, float x0, float y0, float x1, float y1, float x2, float y2) {
    GLenum mode = video_env_set(self, &self->env_primitive);
    video_data_clear(self);
    video_data_put2(self, x0, y0);
    video_data_put2(self, x1, y1);
    video_data_put2(self, x2, y2);
    video_data_send(self);
    glDrawArrays(mode, 0, 3);
}

void video_delete(video_t *self) {
    array_delete(self->configs);
    video_env_shutdown(&self->env_primitive);
    video_env_shutdown(&self->env_textured);
    glDeleteBuffers(1, &self->vbo);
    free(self->buffer);
    free(self);
}

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