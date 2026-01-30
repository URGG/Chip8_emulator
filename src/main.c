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

    // 3. Initialize the Platform (Window Scale: 10x)
    int video_scale = 10;
    platform_init("Gemini Chip-8 Emulator",
                  VIDEO_WIDTH * video_scale,
                  VIDEO_HEIGHT * video_scale,
                  VIDEO_WIDTH, VIDEO_HEIGHT);

    // 4. The Main Execution Loop
    int quit = 0;
    while (!quit) {
        // A. Handle User Input
        quit = platform_process_input(cpu.keypad);

        // B. Run CPU Cycles
        // We run roughly 10 instructions every 16ms (60Hz)
        // to approximate a 600Hz clock speed.
        for (int i = 0; i < 10; i++) {
            cycle(&cpu);
        }

        // C. Update Timers (should run at 60Hz)
        if (cpu.delay_timer > 0) cpu.delay_timer--;
        if (cpu.sound_timer > 0) cpu.sound_timer--;

        // D. Update Screen
        // We pass the internal video buffer to SDL to draw
        platform_update(cpu.video, sizeof(cpu.video[0]) * VIDEO_WIDTH);


        SDL_Delay(16);
    }

    platform_cleanup();
    return 0;
}