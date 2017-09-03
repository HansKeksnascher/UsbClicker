cmake_minimum_required(VERSION 3.6)
project(arcade)

include(FindPkgConfig)
pkg_check_modules(EPOXY REQUIRED epoxy>=1.4.3)
pkg_check_modules(SDL2 REQUIRED sdl2>=2.0.5)
pkg_check_modules(SDL2_IMAGE REQUIRED sdl2_image>=2.0.1)
include_directories(${EPOXY_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(SOURCE_FILES src/ctx.c include/ctx.h src/core.c include/core.h src/video.c include/video.h src/sketch.c src/audio.c include/audio.h src/video_private.h src/sprite.c src/font.c)
add_executable(arcade ${SOURCE_FILES})
target_link_libraries(arcade ${EPOXY_LIBRARIES} ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})