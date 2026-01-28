#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

#define VIDEO_WIDTH 64
#define VIDEO_HEIGHT 32

typedef struct {
    uint8_t  memory[4096];
    uint8_t  V[16];
    uint16_t I;
    uint16_t pc;
    uint16_t stack[16];
    uint16_t sp;
    uint8_t  delay_timer;
    uint8_t  sound_timer;
    uint8_t  keypad[16];
    uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT];
    uint16_t opcode;
} Chip8;

void init_cpu(Chip8* cpu);
int  load_rom(Chip8* cpu, const char* filename);
void cycle(Chip8* cpu);

#endif