#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80
};

void init_cpu(Chip8 *cpu) {
    memset(cpu, 0, sizeof(Chip8));
    cpu->pc = 0x200;
    for (int i = 0; i < 80; ++i) cpu->memory[i] = fontset[i];
}

int load_rom(Chip8 *cpu, const char *filename) {
    // 1. Open file
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error detail"); // This prints WHY it failed to the console
        return -1;
    }

    // 2. Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("ROM Size: %ld bytes\n", size); // Debug print

    // 3. Check if ROM fits in memory (4096 - 512 byte offset)
    if (size > (4096 - 0x200)) {
        printf("Error: ROM is too big!\n");
        fclose(file);
        return -1;
    }

    // 4. Read into memory starting at 0x200
    size_t bytes_read = fread(&cpu->memory[0x200], 1, size, file);
    if (bytes_read != size) {
        printf("Error: Failed to read the full file.\n");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0; // Success!
}

void cycle(Chip8 *cpu) {
    cpu->opcode = (cpu->memory[cpu->pc] << 8) | cpu->memory[cpu->pc + 1];

    uint16_t nnn = cpu->opcode & 0x0FFF;
    uint8_t  nn  = cpu->opcode & 0x00FF;
    uint8_t  x   = (cpu->opcode & 0x0F00) >> 8;
    uint8_t  y   = (cpu->opcode & 0x00F0) >> 4;
    uint8_t  n   = cpu->opcode & 0x000F;

    switch (cpu->opcode & 0xF000) {
        case 0x0000:
            if (cpu->opcode == 0x00E0) {
                memset(cpu->video, 0, sizeof(cpu->video));
                cpu->pc += 2;
            } else if (cpu->opcode == 0x00EE) {
                cpu->sp--;
                cpu->pc = cpu->stack[cpu->sp];
                cpu->pc += 2;
            }
            break;

        case 0x1000: cpu->pc = nnn; break;
        case 0x2000: cpu->stack[cpu->sp] = cpu->pc; cpu->sp++; cpu->pc = nnn; break;
        case 0x3000: cpu->pc += (cpu->V[x] == nn) ? 4 : 2; break;
        case 0x4000: cpu->pc += (cpu->V[x] != nn) ? 4 : 2; break;
        case 0x5000: cpu->pc += (cpu->V[x] == cpu->V[y]) ? 4 : 2; break;
        case 0x6000: cpu->V[x] = nn; cpu->pc += 2; break;
        case 0x7000: cpu->V[x] += nn; cpu->pc += 2; break;

        case 0x8000:
            switch (n) {
                case 0x0: cpu->V[x] = cpu->V[y]; break;
                case 0x1: cpu->V[x] |= cpu->V[y]; break;
                case 0x2: cpu->V[x] &= cpu->V[y]; break;
                case 0x3: cpu->V[x] ^= cpu->V[y]; break;
                case 0x4: {
                    uint16_t res = cpu->V[x] + cpu->V[y];
                    cpu->V[0xF] = (res > 255) ? 1 : 0;
                    cpu->V[x] = res & 0xFF;
                } break;
                case 0x5:
                    cpu->V[0xF] = (cpu->V[x] > cpu->V[y]) ? 1 : 0;
                    cpu->V[x] -= cpu->V[y];
                    break;
                case 0x6: cpu->V[0xF] = cpu->V[x] & 0x1; cpu->V[x] >>= 1; break;
                case 0xE: cpu->V[0xF] = (cpu->V[x] & 0x80) >> 7; cpu->V[x] <<= 1; break;
            }
            cpu->pc += 2;
            break;

        case 0x9000: cpu->pc += (cpu->V[x] != cpu->V[y]) ? 4 : 2; break;
        case 0xA000: cpu->I = nnn; cpu->pc += 2; break;
        case 0xB000: cpu->pc = nnn + cpu->V[0]; break;
        case 0xC000: cpu->V[x] = (rand() % 256) & nn; cpu->pc += 2; break;

        case 0xD000: {
            uint8_t posX = cpu->V[x] % VIDEO_WIDTH;
            uint8_t posY = cpu->V[y] % VIDEO_HEIGHT;
            cpu->V[0xF] = 0;
            for (int row = 0; row < n; row++) {
                uint8_t spriteByte = cpu->memory[cpu->I + row];
                for (int col = 0; col < 8; col++) {
                    if (spriteByte & (0x80 >> col)) {
                        uint32_t* pixel = &cpu->video[(posY + row) * VIDEO_WIDTH + (posX + col)];
                        if (*pixel == 0xFFFFFFFF) cpu->V[0xF] = 1;
                        *pixel ^= 0xFFFFFFFF;
                    }
                }
            }
            cpu->pc += 2;
        } break;

        case 0xE000:
            if (nn == 0x9E) cpu->pc += (cpu->keypad[cpu->V[x]]) ? 4 : 2;
            else if (nn == 0xA1) cpu->pc += (!cpu->keypad[cpu->V[x]]) ? 4 : 2;
            break;

        case 0xF000:
            switch (nn) {
                case 0x07: cpu->V[x] = cpu->delay_timer; break;
                case 0x0A: {
                    int pressed = 0;
                    for (int i = 0; i < 16; i++) {
                        if (cpu->keypad[i]) { cpu->V[x] = i; pressed = 1; break; }
                    }
                    if (!pressed) return; // Wait: don't increment PC
                } break;
                case 0x15: cpu->delay_timer = cpu->V[x]; break;
                case 0x18: cpu->sound_timer = cpu->V[x]; break;
                case 0x1E: cpu->I += cpu->V[x]; break;
                case 0x29: cpu->I = cpu->V[x] * 5; break;
                case 0x33:
                    cpu->memory[cpu->I] = cpu->V[x] / 100;
                    cpu->memory[cpu->I+1] = (cpu->V[x] / 10) % 10;
                    cpu->memory[cpu->I+2] = cpu->V[x] % 10;
                    break;
                case 0x55: for(int i=0; i<=x; i++) cpu->memory[cpu->I+i] = cpu->V[i]; break;
                case 0x65: for(int i=0; i<=x; i++) cpu->V[i] = cpu->memory[cpu->I+i]; break;
            }
            cpu->pc += 2;
            break;
    }
}