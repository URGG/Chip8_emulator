#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// --- CONFIGURATION ---
#define MEMORY_SIZE 4096
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

typedef struct {
    uint8_t  memory[MEMORY_SIZE];
    uint8_t  V[16];          // Registers V0-VF
    uint16_t I;              // Index register
    uint16_t pc;             // Program counter
    uint16_t stack[16];
    uint16_t sp;             // Stack pointer
    uint8_t  delay_timer;
    uint8_t  sound_timer;
    uint8_t  display[DISPLAY_WIDTH * DISPLAY_HEIGHT]; // Monochrome pixel array
    uint16_t opcode;
} Chip8;


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

// --- CORE FUNCTIONS ---
void initialize(Chip8 *cpu) {
    memset(cpu, 0, sizeof(Chip8));
    cpu->pc = 0x200;
    for (int i = 0; i < 80; ++i) cpu->memory[i] = fontset[i];
}

void cycle(Chip8 *cpu) {
    // FETCH: Opcode is 2 bytes
    cpu->opcode = (cpu->memory[cpu->pc] << 8) | cpu->memory[cpu->pc + 1];

    uint16_t nnn = cpu->opcode & 0x0FFF;
    uint8_t  nn  = cpu->opcode & 0x00FF;
    uint8_t  x   = (cpu->opcode & 0x0F00) >> 8;
    uint8_t  y   = (cpu->opcode & 0x00F0) >> 4;
    uint8_t  n   = cpu->opcode & 0x000F;

    // DECODE & EXECUTE
    switch (cpu->opcode & 0xF000) {
        case 0x0000:
            if (cpu->opcode == 0x00E0) { // CLS
                memset(cpu->display, 0, sizeof(cpu->display));
                cpu->pc += 2;
            } else if (cpu->opcode == 0x00EE) { // RET
                cpu->sp--;
                cpu->pc = cpu->stack[cpu->sp];
                cpu->pc += 2;
            }
            break;

        case 0x1000: cpu->pc = nnn; break; // JP addr
        case 0x2000: // CALL addr
            cpu->stack[cpu->sp] = cpu->pc;
            cpu->sp++;
            cpu->pc = nnn;
            break;

        case 0x3000: cpu->pc += (cpu->V[x] == nn) ? 4 : 2; break; // SE Vx, byte
        case 0x4000: cpu->pc += (cpu->V[x] != nn) ? 4 : 2; break; // SNE Vx, byte
        case 0x6000: cpu->V[x] = nn; cpu->pc += 2; break;        // LD Vx, byte
        case 0x7000: cpu->V[x] += nn; cpu->pc += 2; break;       // ADD Vx, byte

        case 0x8000: // Arithmetic Group
            switch (n) {
                case 0x0: cpu->V[x] = cpu->V[y]; break;
                case 0x2: cpu->V[x] &= cpu->V[y]; break;
                case 0x4: { // ADD with Carry
                    uint16_t res = cpu->V[x] + cpu->V[y];
                    cpu->V[0xF] = (res > 255) ? 1 : 0;
                    cpu->V[x] = res & 0xFF;
                } break;
                case 0x5: // SUB (Vx = Vx - Vy)
                    cpu->V[0xF] = (cpu->V[x] > cpu->V[y]) ? 1 : 0;
                    cpu->V[x] -= cpu->V[y];
                    break;
            }
            cpu->pc += 2;
            break;

        case 0xF000:
            switch (cpu->opcode & 0x00FF) {
            case 0x07: cpu->V[x] = cpu->delay_timer; break; // Set Vx to delay_timer
            case 0x15: cpu->delay_timer = cpu->V[x]; break; // Set delay_timer to Vx
            case 0x18: cpu->sound_timer = cpu->V[x]; break; // Set sound_timer to Vx
            case 0x1E: cpu->I += cpu->V[x]; break;          // Add Vx to I
            case 0x29: cpu->I = cpu->V[x] * 5; break;       // Point I to font character in Vx
            case 0x33: // Store Binary Coded Decimal of Vx at I, I+1, I+2
                    cpu->memory[cpu->I]     = cpu->V[x] / 100;
                    cpu->memory[cpu->I + 1] = (cpu->V[x] / 10) % 10;
                    cpu->memory[cpu->I + 2] = cpu->V[x] % 10;
                    break;
            case 0x55: // Store V0 through Vx in memory starting at I
                    for (int i = 0; i <= x; i++) cpu->memory[cpu->I + i] = cpu->V[i];
                    break;
            case 0x65: // Read memory starting at I into V0 through Vx
                    for (int i = 0; i <= x; i++) cpu->V[i] = cpu->memory[cpu->I + i];
                    break;
            }
            cpu->pc += 2;
            break;


        case 0xA000: cpu->I = nnn; cpu->pc += 2; break; // LD I, addr

        case 0xD000: { // DXYN: DRAW SPRITE
            uint8_t posX = cpu->V[x] % DISPLAY_WIDTH;
            uint8_t posY = cpu->V[y] % DISPLAY_HEIGHT;
            cpu->V[0xF] = 0;

            for (int row = 0; row < n; row++) {
                uint8_t spriteByte = cpu->memory[cpu->I + row];
                for (int col = 0; col < 8; col++) {
                    uint8_t spritePixel = spriteByte & (0x80 >> col);
                    uint32_t screenIdx = (posX + col) + ((posY + row) * DISPLAY_WIDTH);

                    if (spritePixel) {
                        if (cpu->display[screenIdx] == 1) cpu->V[0xF] = 1; // Collision
                        cpu->display[screenIdx] ^= 1;
                    }
                }
            }
            cpu->pc += 2;
        } break;

        default:
            printf("Opcode Not Implemented: 0x%X\n", cpu->opcode);
            cpu->pc += 2;
            break;
    }

    // Update Timers (should ideally be synced to 60Hz)
    if (cpu->delay_timer > 0) cpu->delay_timer--;
    if (cpu->sound_timer > 0) cpu->sound_timer--;
}

int main(int argc, char **argv) {
    Chip8 myChip8;
    initialize(&myChip8);

    printf("Chip-8 Initialized. Program Counter at: 0x%X\n", myChip8.pc);

    // To actually run a game, you would call load_rom() here.
    // while(1) { cycle(&myChip8); }

    return 0;
}