#include "../include/core.h"
#include <stddef.h>
#include <time.h>

static uint64_t random_state[2];
static bool gaussian_ready;
static float gaussian_next;

void core_init() {
    srand((unsigned int) time(NULL));
    random_state[0] = ((uint64_t) rand()) | ((uint64_t) rand()) << 16 | ((uint64_t) rand()) << 32 | ((uint64_t) rand()) << 48;
    random_state[1] = ((uint64_t) rand()) | ((uint64_t) rand()) << 16 | ((uint64_t) rand()) << 32 | ((uint64_t) rand()) << 48;
}

void *malloc_ext(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
#ifdef DEBUG
        printf("malloc: can't allocate %llu bytes\n", (unsigned long long) size);
#endif
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *realloc_ext(void *memory, size_t size) {
    void *ptr = realloc(memory, size);
    if (!ptr) {
#ifdef DEBUG
        printf("realloc: can't reallocate %p to %llu bytes\n", memory, (unsigned long long) size);
#endif
        exit(EXIT_FAILURE);
    }
    return ptr;
}

size_t file_read(const char *filename, char *buffer, size_t size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
#ifdef DEBUG
        printf("file_read: missing file %s\n", filename);
#endif
        return 0;
    }
    size_t count = fread(buffer, sizeof(char), size, file);
    fclose(file);
    buffer[MIN(count, size - 1)] = '\0';
    return count;
}

static bool list_iterator_has_next(iterator_t *iterator) {
    return iterator->ptr_next != NULL;
}

static void *list_iterator_next(iterator_t *iterator) {
    list_node_t *node = iterator->ptr_next;
    iterator->ptr_prev = node;
    iterator->ptr_next = node->next;
    return node->data;
}

static list_node_t *list_node_new(size_t padding, void *item, list_node_t *next, list_node_t *prev) {
    list_node_t *node = malloc_ext(sizeof(*node) + padding);
    node->next = next;
    node->prev = prev;
    if (item) {
        memcpy(node->data, item, padding);
    } else {
        memset(node->data, 0, padding);
    }
    return node;
}

list_t *list_new(size_t padding) {
    list_t *self = malloc_ext(sizeof(*self));
    self->padding = padding;
    self->size = 0;
    self->head = NULL;
    self->tail = NULL;
    return self;
}

void *list_add_first(list_t *self, void *item) {
    list_node_t *node = list_node_new(self->padding, item, self->head, NULL);
    self->head = node;
    if (self->head) {
        self->head->prev = node;
    }
    if (!self->tail) {
        self->tail = node;
    }
    self->size++;
    return node->data;
}

void *list_add_last(list_t *self, void *item) {
    list_node_t *node = list_node_new(self->padding, item, NULL, self->tail);
    if (!self->head) {
        self->head = node;
    }
    if (self->tail) {
        self->tail->next = node;
    }
    self->tail = node;
    self->size++;
    return node->data;
}

iterator_t list_iterator(list_t *self) {
    return (iterator_t) {
            .has_next = list_iterator_has_next,
            .next = list_iterator_next,
            .ptr_base = self,
            .ptr_next = self->head,
            .ptr_prev = NULL
    };
}

void list_delete(list_t *self) {
    list_node_t *node = self->head;
    while (node) {
        list_node_t *tmp = node;
        node = node->next;
        free(tmp);
    }
    free(self);
}

static bool array_iterator_has_next(iterator_t *iterator) {
    array_t *self = iterator->ptr_base;
    ptrdiff_t index = iterator->ptr_next - self->data;
    return index < self->size;
}

static void *array_iterator_next(iterator_t *iterator) {
    array_t *self = iterator->ptr_base;
    ptrdiff_t index = iterator->ptr_next - self->data;
    iterator->ptr_prev = iterator->ptr_next;
    iterator->ptr_next++;
    return array_get(self, (int) index);
}

static void array_iterator_remove(iterator_t *iterator) {
    array_t *self = iterator->ptr_base;
    ptrdiff_t index = iterator->ptr_prev - self->data;
    array_remove(self, (int) index);
}

array_t *array_new(size_t padding) {
    array_t *self = malloc_ext(sizeof(*self));
    self->padding = padding;
    self->size = 0;
    self->capacity = 8;
    self->data = malloc_ext(self->capacity * self->padding);
    return self;
}

void *array_add(array_t *self, int index, void *item) {
    if (index < 0 || index > self->size) {
        return NULL;
    }
    if (self->size >= self->capacity) {
        self->capacity = 2 * self->capacity + 1;
        self->data = realloc_ext(self->data, self->capacity * self->padding);
    }
    size_t count = self->size - index;
    void *prev = self->data + index * self->padding;
    void *next = self->data + (index + 1) * self->padding;
    memmove(next, prev, count * self->padding);
    if (item) {
        memcpy(prev, item, self->padding);
    } else {
        memset(prev, 0, self->padding);
    }
    self->size++;
    return prev;

}
void *array_add_first(array_t *self, void *item) {
    return array_add(self, 0, item);
}

void *array_add_last(array_t *self, void *item) {
    return array_add(self, (int) self->size, item);
}

void *array_get(array_t *self, int index) {
    if (index < 0 || index >= self->size) {
        return NULL;
    }
    return self->data + index * self->padding;
}

void *array_get_first(array_t *self) {
    return array_get(self, 0);
}

void *array_get_last(array_t *self) {
    return array_get(self, (int) (self->size - 1));
}

bool array_remove(array_t *self, int index) {
    if (index < 0 || index >= self->size) {
        return false;
    }
    size_t count = self->size - index;
    void *prev = self->data + index * self->padding;
    void *next = self->data + (index + 1) * self->padding;
    memmove(prev, next, count * self->padding);
    self->size--;
    return true;
}

bool array_remove_first(array_t *self) {
    return array_remove(self, 0);
}

bool array_remove_last(array_t *self) {
    return array_remove(self, (int) (self->size - 1));
}

iterator_t array_iterator(array_t *self) {
    return (iterator_t) {
            .has_next = array_iterator_has_next,
            .next = array_iterator_next,
            .remove = array_iterator_remove,
            .ptr_base = self,
            .ptr_next = self->data,
            .ptr_prev = NULL
    };
}

void array_delete(array_t *self) {
    free(self->data);
    free(self);
}

bool is_pot(int value) {
    return (value & (value - 1)) == 0;
}

double random_double(double min, double max) {
    return random_bits() / (UINT64_MAX / (max - min)) + min;
}

float random_float(float min, float max) {
    return random_bits() / (UINT64_MAX / (max - min)) + min;
}

float random_gaussian() {
    if (gaussian_ready) {
        gaussian_ready = false;
        return gaussian_next;
    }
    float v1, v2, s;
    do {
        v1 = random_float(-1.0f, 1.0f);
        v2 = random_float(-1.0f, 1.0f);
        s = v1 * v1 + v2 * v2;
    } while (s >= 1.0f || s == 0.0f);
    float multiplier = sqrtf(-2.0f * logf(s) / s);
    gaussian_next = v2 * multiplier;
    gaussian_ready = true;
    return v1 * multiplier;
}

unsigned long long random_bits() {
    uint64_t s0 = random_state[0];
    uint64_t s1 = random_state[1];
    uint64_t result = s0 + s1;
    s1 ^= s0;
    random_state[0] = ((s0 << 55) | (s0 >> (64 - 55))) ^ s1 ^ (s1 << 14);
    random_state[1] = ((s1 << 36) | (s1 >> (64 - 36)));
    return result;
}

mat4_t mat4_identity() {
    return (mat4_t) {
            .xx = 1,
            .yy = 1,
            .zz = 1,
            .ww = 1
    };
}

mat4_t mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
    return (mat4_t) {
            .xx = 2 / (right - left),
            .yy = 2 / (top - bottom),
            .zz = -2 / (far - near),
            .wx = -(right + left) / (right - left),
            .wy = -(top + bottom) / (top - bottom),
            .wz = -(far + near) / (far - near),
            .ww = 1
    };
}

mat4_t mat4_transpose(mat4_t m) {
    mat4_t ret = {};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ret.ptr[4 * i + j] = m.ptr[4 * j + i];
        }
    }
    return ret;
}

vec2_t vec2_new(float x, float y) {
    return (vec2_t) {x, y};
}

vec2_t vec2_add(vec2_t a, vec2_t b) {
    return (vec2_t) {
            .x = a.x + b.x,
            .y = a.y + b.y
    };
}

vec4_t vec4_new(float x, float y, float z, float w) {
    return (vec4_t) {x, y, z, w};
}

bool vec4_point_inside(vec4_t vec, vec2_t pos) {
    return pos.x >= vec.x && pos.x <= vec.x + vec.z && pos.y >= vec.y && pos.y <= vec.y + vec.w;
}