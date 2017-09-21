#ifndef VIDEO_PRIVATE_H
#define VIDEO_PRIVATE_H

#include "../include/video.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#ifdef __EMSCRIPTEN__
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <epoxy/gl.h>
#endif

#define VIDEO_BUFFER_SIZE 65536

typedef enum {
    VIDEO_PRIMITIVE,
    VIDEO_TEXTURED,
    VIDEO_PARTICLE,
    VIDEO_PARTICLE_TEXTURED
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
    GLuint attrib_instance_offset;
    GLuint attrib_instance_color;
    GLuint uniform_projection;
    GLuint uniform_color;
    bool dirty;
} video_env_t;

struct video_t {
    float *buffer;
    size_t buffer_size;
    GLuint vbo[2];
    video_env_t env_primitive;
    video_env_t env_textured;
    video_env_t env_particles;
    video_env_t env_particles_textured;
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

GLenum video_env_set(video_t *self, video_env_t *env);
void video_data_clear(video_t *self);
void video_data_put2(video_t *self, float p0, float p1);
void video_data_put4(video_t *self, float p0, float p1, float p2, float p3);
void video_data_send(video_t *self, int vbo_index);

#endif
