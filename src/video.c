#include "video_private.h"
#include "../include/ctx.h"

static GLuint video_shader_load(GLenum type, const char *filename) {
    char *buffer = malloc_ext(1024 * sizeof(char));
    if (!file_read(filename, buffer, 1024)) {
        free(buffer);
        return 0;
    }
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char**) &buffer, NULL);
    glCompileShader(shader);
#ifdef DEBUG
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        glGetShaderInfoLog(shader, 1024 * sizeof(char), NULL, buffer);
        printf("%s: %s\n", filename, buffer);
    }
#endif
    free(buffer);
    return shader;
}

static void video_env_init(video_t *self, video_clazz clazz, video_env_t *env) {
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
        case VIDEO_PARTICLE:
            glAttachShader(env->program, video_shader_load(GL_FRAGMENT_SHADER, "asset/shader/particle.frag"));
            glAttachShader(env->program, video_shader_load(GL_VERTEX_SHADER, "asset/shader/particle.vert"));
            break;
    }
    glLinkProgram(env->program);
#ifdef DEBUG
    GLint status;
    glGetProgramiv(env->program, GL_LINK_STATUS, &status);
    if (!status) {
        char buffer[1024];
        glGetProgramInfoLog(env->program, sizeof(buffer), NULL, buffer);
        printf("%s\n", buffer);
    }
#endif
    env->attrib_position = (GLuint) glGetAttribLocation(env->program, "position");
    env->attrib_tex_coord = (GLuint) glGetAttribLocation(env->program, "texCoord");
    env->attrib_instance_offset = (GLuint) glGetAttribLocation(env->program, "instanceOffset");
    env->attrib_instance_color = (GLuint) glGetAttribLocation(env->program, "instanceColor");
    env->uniform_projection = (GLuint) glGetUniformLocation(env->program, "projection");
    env->uniform_color = (GLuint) glGetUniformLocation(env->program, "color");
    glGenVertexArraysOES(1, &env->vao);
    glBindVertexArrayOES(env->vao);
    glBindBuffer(GL_ARRAY_BUFFER, self->vbo[0]);
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
        case VIDEO_PARTICLE:
            glEnableVertexAttribArray(env->attrib_position);
            glVertexAttribPointer(env->attrib_position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
            glBindBuffer(GL_ARRAY_BUFFER, self->vbo[1]);
            glEnableVertexAttribArray(env->attrib_instance_offset);
            glVertexAttribPointer(env->attrib_instance_offset, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), NULL);
            glVertexAttribDivisorANGLE(env->attrib_instance_offset, 1);
            glEnableVertexAttribArray(env->attrib_instance_color);
            glVertexAttribPointer(env->attrib_instance_color, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (2 * sizeof(float)));
            glVertexAttribDivisorANGLE(env->attrib_instance_color, 1);
            break;
    }
}

GLenum video_env_set(video_t *self, video_env_t *env) {
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

void video_data_clear(video_t *self) {
    self->buffer_size = 0;
}

void video_data_put2(video_t *self, float p0, float p1) {
    size_t i = self->buffer_size;
    self->buffer[i] = p0;
    self->buffer[i + 1] = p1;
    self->buffer_size += 2;
}

void video_data_put4(video_t *self, float p0, float p1, float p2, float p3) {
    size_t i = self->buffer_size;
    self->buffer[i] = p0;
    self->buffer[i + 1] = p1;
    self->buffer[i + 2] = p2;
    self->buffer[i + 3] = p3;
    self->buffer_size += 4;
}

void video_data_send(video_t *self, int vbo_index) {
    glBindBuffer(GL_ARRAY_BUFFER, self->vbo[vbo_index]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, self->buffer_size * sizeof(float), self->buffer);
}

static void video_mark_dirty(video_t *self) {
    self->env_primitive.dirty = true;
    self->env_textured.dirty = true;
    self->env_particles.dirty = true;
}

video_t *video_new() {
    video_t *self = malloc_ext(sizeof(*self));
    self->buffer = malloc_ext(VIDEO_BUFFER_SIZE * sizeof(float));
    glGenBuffers(ARRAY_LENGTH(self->vbo), self->vbo);
    for (int i = 0; i < ARRAY_LENGTH(self->vbo); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, self->vbo[i]);
        glBufferData(GL_ARRAY_BUFFER, VIDEO_BUFFER_SIZE * sizeof(float), NULL, GL_STREAM_DRAW);
    }
    glBindBuffer(GL_ARRAY_BUFFER, self->vbo[0]);
    video_env_init(self, VIDEO_PRIMITIVE, &self->env_primitive);
    video_env_init(self, VIDEO_TEXTURED, &self->env_textured);
    video_env_init(self, VIDEO_PARTICLE, &self->env_particles);
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
    video_data_send(self, 0);
    glDrawArrays(mode, 0, 4);
}

void video_triangle(video_t *self, float x0, float y0, float x1, float y1, float x2, float y2) {
    GLenum mode = video_env_set(self, &self->env_primitive);
    video_data_clear(self);
    video_data_put2(self, x0, y0);
    video_data_put2(self, x1, y1);
    video_data_put2(self, x2, y2);
    video_data_send(self, 0);
    glDrawArrays(mode, 0, 3);
}

void video_delete(video_t *self) {
    array_delete(self->configs);
    video_env_shutdown(&self->env_primitive);
    video_env_shutdown(&self->env_textured);
    video_env_shutdown(&self->env_particles);
    glDeleteBuffers(ARRAY_LENGTH(self->vbo), self->vbo);
    free(self->buffer);
    free(self);
}