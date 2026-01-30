#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "chip8.h"
#include "platform.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <ROM_FILE_PATH>\n", argv[0]);
        return 1;
    }

    const char* rom_filename = argv[1];


    Chip8 cpu;
    init_cpu(&cpu);


    if (load_rom(&cpu, rom_filename) != 0) {
        printf("Error: Could not load ROM file: %s\n", rom_filename);
        return 1;
    }


    int video_scale = 10;
    platform_init("Gemini Chip-8 Emulator",
                  VIDEO_WIDTH * video_scale,
                  VIDEO_HEIGHT * video_scale,
                  VIDEO_WIDTH, VIDEO_HEIGHT);


    int quit = 0;
    while (!quit) {

        quit = platform_process_input(cpu.keypad);


        for (int i = 0; i < 10; i++) {
            cycle(&cpu);
        }


        if (cpu.delay_timer > 0) cpu.delay_timer--;
        if (cpu.sound_timer > 0) cpu.sound_timer--;


        platform_update(cpu.video, sizeof(cpu.video[0]) * VIDEO_WIDTH);


        SDL_Delay(16);
    }

    platform_cleanup();
    return 0;
}