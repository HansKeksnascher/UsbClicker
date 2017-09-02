#ifndef CORE_H
#define CORE_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

void core_init();
void *malloc_ext(size_t size);
void *realloc_ext(void *memory, size_t size);
size_t file_read(const char *filename, char *buffer, size_t size);

typedef struct iterator_t {
    bool (*has_next)(struct iterator_t*);
    void *(*next)(struct iterator_t*);
    void (*remove)(struct iterator_t*);
    void *ptr_base;
    void *ptr_next;
    void *ptr_prev;
} iterator_t;

typedef struct array_t {
    size_t size;
    size_t capacity;
    size_t padding;
    void *data;
} array_t;

array_t *array_new(size_t padding);
void *array_add(array_t *self, int index, void *item);
void *array_add_first(array_t *self, void *item);
void *array_add_last(array_t *self, void *item);
void *array_get(array_t *self, int index);
void *array_get_first(array_t *self);
void *array_get_last(array_t *self);
bool array_remove(array_t *self, int index);
bool array_remove_first(array_t *self);
bool array_remove_last(array_t *self);
void array_delete(array_t *self);

typedef struct list_node_t {
    struct list_node_t *next;
    struct list_node_t *prev;
    char data[];
} list_node_t;

typedef struct list_t {
    size_t size;
    size_t padding;
    list_node_t *head;
    list_node_t *tail;
} list_t;

list_t *list_new(size_t padding);
void *list_add_first(list_t *self, void *item);
void *list_add_last(list_t *self, void *item);
iterator_t list_iterator(list_t *self);
void list_delete(list_t *self);

typedef union mat4_t {
    struct {
        float xx, yx, zx, wx;
        float xy, yy, zy, wy;
        float xz, yz, zz, wz;
        float xw, yw, zw, ww;
    };
    float ptr[16];
} mat4_t;

typedef union vec2_t {
    struct {
        float x, y;
    };
    float ptr[2];
} vec2_t;

typedef union vec4_t {
    struct {
        float x, y, z, w;
    };
    float ptr[4];
} vec4_t;

bool is_pot(int value);
double random_double(double min, double max);
float random_float(float min, float max);
unsigned long long random_u64();

mat4_t mat4_identity();
mat4_t mat4_ortho(float left, float right, float bottom, float top, float near, float far);
mat4_t mat4_transpose(mat4_t m);

vec2_t vec2_new(float x, float y);

#define COLOR_RGBA(r, g, b, a) vec4_new((r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a) / 255.0f)
vec4_t vec4_new(float x, float y, float z, float w);
bool vec4_point_inside(vec4_t vec, vec2_t pos);

#endif
