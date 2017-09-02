@echo off
call emsdk_env
call emcc src/audio.c src/core.c src/ctx.c src/sketch.c src/video.c -s FULL_ES2=1 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS="[""png""]" -O3 -o arcade.html --preload-file asset