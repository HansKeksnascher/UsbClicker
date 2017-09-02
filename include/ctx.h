#ifndef CTX_H
#define CTX_H

#include "core.h"

struct video_t;

typedef struct sketch_t {
    void (*init)();
    void (*tick)();
    void (*draw)(struct video_t*);
    void (*shutdown)();
} sketch_t;

int ctx_main(int argc, char **argv, sketch_t *sketch);
vec2_t ctx_viewport();
void ctx_hook_mouse(void (*hook)(vec2_t));

#endif
