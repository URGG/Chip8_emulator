#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <SDL2/SDL.h>

void platform_init(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight);
void platform_cleanup();
void platform_update(void const* buffer, int pitch);
int platform_process_input(uint8_t* keys);

#endif