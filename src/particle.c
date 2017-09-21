#include "video_private.h"

emitter_t *emitter_new(sprite_t *sprite) {
    emitter_t *self = malloc_ext(sizeof(*self));
    self->particles = array_new(sizeof(particle_t));
    self->sprite = sprite;
    return self;
}

particle_t *emitter_emit(emitter_t *self, float x, float y) {
    particle_t *particle = array_add_last(self->particles, NULL);
    particle->position.x = x;
    particle->position.y = y;
    particle->lifetime = 300;
    return particle;
}

void emitter_tick(emitter_t *self) {
    iterator_t iterator = array_iterator(self->particles);
    while (iterator_has_next(iterator)) {
        particle_t *particle = iterator_next(iterator);
        if (particle->lifetime > 0) {
            particle->position = vec2_add(particle->position, particle->velocity);
            particle->lifetime--;
        } else {
            iterator_remove(iterator);
        }
    }
}

void emitter_draw(emitter_t *self, video_t *video) {
    video_data_clear(video);
    if (self->sprite) {
        video_env_set(video, &video->env_particles_textured);
        sprite_t *sprite = self->sprite;
        glBindTexture(GL_TEXTURE_2D, sprite->texture);
        video_data_put4(video, 0, 0, 0, 0);
        video_data_put4(video, sprite->w, 0, 1, 0);
        video_data_put4(video, sprite->w, sprite->h, 1, 1);
        video_data_put4(video, 0, sprite->h, 0, 1);
    } else {
        video_env_set(video, &video->env_particles);
        video_data_put2(video, 0, 0);
        video_data_put2(video, 5, 0);
        video_data_put2(video, 5, 5);
        video_data_put2(video, 0, 5);
    }
    video_data_send(video, 0);
    int count = 0;
    video_data_clear(video);
    iterator_t iterator = array_iterator(self->particles);
    while (iterator_has_next(iterator)) {
        particle_t *particle = iterator_next(iterator);
        video_data_put2(video, particle->position.x, particle->position.y);
        video_data_put4(video, particle->color.x, particle->color.y, particle->color.z, particle->color.w);
        count++;
    }
    video_data_send(video, 1);
    glDrawArraysInstancedANGLE(GL_TRIANGLE_FAN, 0, 4, count);
}

void emitter_delete(emitter_t *self) {
    array_delete(self->particles);
    free(self);
}