#include "../include/ctx.h"
#include "../include/video.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

static SDL_Window *window;
static SDL_GLContext gl;
static video_t *video;
static bool running;
static vec2_t mouse_pos;
static void (*mouse_hook)(vec2_t);

static void ctx_loop(void *arg) {
    SDL_Event event;
    sketch_t *sketch = arg;
    vec2_t viewport = ctx_viewport();
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
                mouse_pos.x = viewport.x * event.tfinger.x;
                mouse_pos.y = viewport.y * event.tfinger.y;
                if (event.tfinger.type == SDL_FINGERDOWN && mouse_hook) {
                    mouse_hook(mouse_pos);
                }
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                mouse_pos.x = event.button.x;
                mouse_pos.y = event.button.y;
                if (event.button.state == SDL_PRESSED && mouse_hook) {
                    mouse_hook(mouse_pos);
                }
                break;
            case SDL_MOUSEMOTION:
                mouse_pos.x = event.motion.x;
                mouse_pos.y = event.motion.y;
                break;
            case SDL_QUIT:
#ifdef __EMSCRIPTEN__
                emscripten_cancel_main_loop();
#else
                running = false;
#endif
                break;
            default:
                break;
        }
    }
    sketch->tick();
    video_clear(video);
    sketch->draw(video);
    SDL_GL_SwapWindow(window);
}

int ctx_main(int argc, char **argv, sketch_t *sketch) {
    core_init();
    if (SDL_Init(SDL_INIT_VIDEO)) {
        return EXIT_FAILURE;
    }
    if (!IMG_Init(IMG_INIT_PNG)) {
        SDL_Quit();
        return EXIT_FAILURE;
    }
    window = SDL_CreateWindow("arcade", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 768, SDL_WINDOW_OPENGL);
    if (!window) {
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
#ifndef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    gl = SDL_GL_CreateContext(window);
    if (!gl) {
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_GL_SetSwapInterval(1);
    video = video_new();
    sketch->init();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(ctx_loop, sketch, 0, 1);
#else
    running = true;
    while (running) {
        ctx_loop(sketch);
    }
#endif
    sketch->shutdown();
    video_delete(video);
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}

vec2_t ctx_viewport() {
    int w, h;
    SDL_GL_GetDrawableSize(window, &w, &h);
    return vec2_new(w, h);
}

void ctx_hook_mouse(void (*hook)(vec2_t)) {
    mouse_hook = hook;
}