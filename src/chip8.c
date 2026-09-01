#include "chip8.h"
#include "config.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "raylib.h"


static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

bool drawFlag = true;

void chip8_setup(Chip8 *chip8) {
    chip8->pc = ROM_START_ADDRESS;
    chip8->I = 0;
    chip8->sp = 0;

    // Clear memory
	for(int i = 0; i < 4096; ++i)
		chip8->ram[i] = 0;

    // Load font
    for (int i = 0; i < 80; ++i)
        chip8->ram[i] = chip8_fontset[i];
    
	chip8_clear_display(chip8);

	// Clear stack
	for(int i = 0; i < 16; ++i)
		chip8->stack[i] = 0;

	for(int i = 0; i < 16; ++i)
		chip8->keys[i] = chip8->v[i] = 0;

    chip8->delay_timer = 0;
    chip8->sound_timer = 0;

    chip8->drawFlag = false;
    chip8->runFlag = true;
}

void chip8_load_game(Chip8 *chip8, char* game) {
    FILE *file = fopen(game, "rb");

    if (!file) {
        perror("Error: Unable to open file");
        if (file) {
            fclose(file);
        }
        return;
    }

    size_t bytesRead = fread(&chip8->ram[ROM_START_ADDRESS], sizeof(uint8_t), MEMORY_SIZE - ROM_START_ADDRESS, file);

    fclose(file);
}

void excute_opcodes(Chip8 *chip8, uint16_t opcode) {
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode) {
                case 0x00E0: // Clears the screen
                    chip8_clear_display(chip8);
                    chip8->drawFlag = true;
                break;
                
                case 0x00EE: // Return from a subroutine
                    chip8->sp--;
                    chip8->pc = chip8->stack[chip8->sp];
                    chip8->pc += 2;
                break;
            }
            break;

        case 0x1000: // Jumb to location nnn
            chip8->pc = opcode & 0x0FFF;
            break;

        case 0x2000: // Calles a subroutine at nnn
            chip8->stack[chip8->sp] = chip8->pc;
            chip8->sp++;
            chip8->pc = opcode & 0x0FFF;
            break;

        case 0x3000: { // Skip next instruction if Vx == kk
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;

            if (chip8->v[x] == kk) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;
        }

        case 0x4000: { // Skip next instruction if Vx != kk.
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;

            if (chip8->v[x] != kk) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;
        }

        case 0x5000: { // Skip next instruction if Vx = Vy.
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            if (chip8->v[x] == chip8->v[y]) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;
        }

        case 0x6000: { // Put the value kk into vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;
            
            chip8->v[x] = kk;
            chip8->pc += 2;
            break;
        }

        case 0x7000: { // Add kk to vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;
            
            chip8->v[x] += kk;
            chip8->pc += 2;
            break;
        }

        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;

                    chip8->v[x] = chip8->v[y];
                    chip8->pc += 2;
                    break;
                }

                case 0x0001: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;

                    chip8->v[x] = chip8->v[x] | chip8->v[y];
                    chip8->pc += 2;
                    break;
                }

                case 0x0002: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;

                    chip8->v[x] = chip8->v[x] & chip8->v[y];
                    chip8->pc += 2;
                    break;
                }

                case 0x0003: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;

                    chip8->v[x] = chip8->v[x] ^ chip8->v[y];
                    chip8->pc += 2;
                    break;
                }

                case 0x0004: { // Set Vx = Vx + Vy, set VF = carry
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;

                    uint16_t sum = chip8->v[x] + chip8->v[y];

                    if (sum > 255) {
                        chip8->v[0xF] = 1;
                    } else {
                        chip8->v[0xF] = 0;
                    }

                    chip8->v[x] = (uint8_t)sum;
                    chip8->pc += 2;
                    break;
                }
                    
                case 0x0005: { // Set Vx = Vx - Vy, set VF = NOT borrow
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;

                    if (chip8->v[x] > chip8->v[y]) {
                        chip8->v[0xF] = 1;
                    } else {
                        chip8->v[0xF] = 0;
                    }

                    chip8->v[x] -= chip8->v[y];
                    chip8->pc += 2;
                    break;
                }

                case 0x0006: { // Check if Vx is even or not, dvide Vx by 2
                    uint8_t x = (opcode & 0x0F00) >> 8;

                    if (chip8->v[x] % 2 == 1) {
                        chip8->v[0xF] = 1;
                    } else {
                        chip8->v[0xF] = 0;
                    }

                    chip8->v[x] /= 2;
                    chip8->pc += 2;
                    break;
                }

                case 0x0007: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;

                    if (chip8->v[y] > chip8->v[x]) {
                        chip8->v[0xF] = 1;
                    } else {
                        chip8->v[0xF] = 0;
                    }

                    chip8->v[x] = chip8->v[y] - chip8->v[x];
                    chip8->pc += 2;
                    break;
                }

                case 0x000E: { // If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2
                    uint8_t x = (opcode & 0x0F00) >> 8;

                    if ((chip8->v[x] & 0x80) != 0) {
                        chip8->v[0xF] = 1;
                    } else {
                        chip8->v[0xF] = 0;
                    }

                    chip8->v[x] *= 2;
                    chip8->pc += 2;
                    break;
                }

            }
            break;

        case 0x9000: { // Skip next instruction if Vx != Vy
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            if (chip8->v[x] != chip8->v[y]) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;
        }

        case 0xA000: // Set I to the value nnn
            chip8->I = opcode & 0x0FFF;
            chip8->pc += 2;
            break;

        case 0xB000: { // Jump to location nnn + V0
            uint16_t nnn = opcode & 0x0FFF;
            chip8->pc = nnn + chip8->v[0x0];
            break;
        }

        case 0xC000: { // Set Vx = random byte AND kk
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;

            chip8->v[x] = rand() % 256 & kk;
            chip8->pc += 2;
            break;
        }

        case 0xD000: { // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
            uint8_t vx = chip8->v[(opcode & 0x0F00) >> 8];
            uint8_t vy = chip8->v[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F; // n value
            uint8_t pixel;
            
            chip8->v[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                pixel = chip8->ram[chip8->I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        int px = (vx + xline) % SCREEN_WIDTH;
                        int py = (vy + yline) % SCREEN_HEIGHT;

                        if (chip8->gfx[px][py] == 1) {
                            chip8->v[0xF] = 1;
                        }
                        chip8->gfx[px][py] ^= 1;
                    }
                }
            }
            
            chip8->drawFlag = true;
            chip8->pc += 2;
            break;
        }

        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E: { // Skip next instruction if key with the value of Vx is pressed
                    uint8_t x = (opcode & 0x0F00) >> 8;

                    if (chip8->keys[chip8->v[x]] != false) {
                        chip8->pc += 4;
                    } else {
                        chip8->pc += 2;
                    }
                    break;
                }

                case 0x00A1: { // Skip next instruction if key with the value of Vx is not pressed
                    uint8_t x = (opcode & 0x0F00) >> 8;

                    if (chip8->keys[chip8->v[x]] == false) {
                        chip8->pc += 4;
                    } else {
                        chip8->pc += 2;
                    }
                    break;
                }
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: { // Set Vx = delay timer value
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip8->v[x] = chip8->delay_timer;
                    chip8->pc += 2;
                    break;
                }

                case 0x000A: { // Wait for a key press, store the value of the key in Vx
                    uint8_t x = (opcode & 0x0F00) >> 8;

                    chip8->wasKeyPressed = false;

                    for (int i = 0; i < 16; i++) {
                        if (chip8->keys[i] == true) {
                            chip8->v[x] = i;
                            chip8->wasKeyPressed = true;
                        }
                    }

                    if (chip8->wasKeyPressed) {
                        return;
                    }

                    chip8->pc += 2;
                    break;
                }

                case 0x0015: { // Set delay timer = Vx
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip8->delay_timer = chip8->v[x];
                    chip8->pc += 2;
                    break;
                }

                case 0x0018: { // Set sound timer = Vx
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip8->sound_timer = chip8->v[x];
                    chip8->pc += 2;
                    break;
                }

                case 0x001E: { // Set I = I + Vx
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip8->I += chip8->v[x];
                    chip8->pc += 2;
                    break;
                }

                case 0x0029: { // Set I = location of sprite for digit Vx
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip8->I = chip8->v[x] * 0x5;
                    chip8->pc += 2;
                    printf("opcode excuted\n");
                    break;
                }

                case 0x0033: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip8->ram[chip8->I] = chip8->v[x] / 100;
                    chip8->ram[chip8->I + 1] = (chip8->v[x] / 10) % 10;
                    chip8->ram[chip8->I + 2] = (chip8->v[x] % 100) % 10;
                    chip8->pc += 2;
                    break;
                }

                case 0x0055: {
                    uint8_t x = (opcode & 0x0F00) >> 8;

                    for (int i = 0; i <= x; i++) {
                        chip8->ram[chip8->I + i] = chip8->v[i];
                    }

                    chip8->pc += 2;
                    break;
                }

                case 0x0065: {
                    uint8_t x = (opcode & 0x0F00) >> 8;

                    for (int i = 0; i <= x; i++) {
                        chip8->v[i] = chip8->ram[chip8->I + i];
                    }

                    chip8->pc += 2;
                    break;
                }
            }
        break;

        default:
            printf("unknown opcode : %X\n", opcode);
        break;
    }
}

void chip8_update(Chip8 *chip8) {
    if (chip8->pc >= MEMORY_SIZE - 1) {
        printf("pc out of ram bounds\n");
        return;
    }

    uint16_t opcode = chip8->ram[chip8->pc] << 8 | chip8->ram[chip8->pc + 1];

    excute_opcodes(chip8, opcode);

    if (chip8->delay_timer > 0) {
        chip8->delay_timer--;
    }

    if (chip8->sound_timer > 0) {
        chip8->sound_timer--;
        if (chip8->sound_timer == 0) {
            printf("BEEP!\n");
        }
    }
}

void chip8_set_keys(Chip8 *chip8) {
    switch (GetKeyPressed()) {
        case KEY_ONE:
            chip8->keys[0x1] = true;
        break;
        case KEY_TWO:
            chip8->keys[0x2] = true;
        break;
        case KEY_THREE:
            chip8->keys[0x3] = true;
        break;
        case KEY_FOUR:
            chip8->keys[0xC] = true;
        break;
        case KEY_Q:
            chip8->keys[0x4] = true;
        break;
        case KEY_W:
            chip8->keys[0x5] = true;
        break;
        case KEY_E:
            chip8->keys[0x6] = true;
        break;
        case KEY_R:
            chip8->keys[0xD] = true;
        break;
        case KEY_A:
            chip8->keys[0x7] = true;
        break;
        case KEY_S:
            chip8->keys[0x8] = true;
        break;
        case KEY_D:
            chip8->keys[0x9] = true;
        break;
        case KEY_F:
            chip8->keys[0xE] = true;
        break;
        case KEY_Z:
            chip8->keys[0xA] = true;
        break;
        case KEY_X:
            chip8->keys[0x0] = true;
        break;
        case KEY_C:
            chip8->keys[0xB] = true;
        break;
        case KEY_V:
            chip8->keys[0xF] = true;
        break;
    }
}

void chip8_clear_display(Chip8* chip8) {
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        for (int j = 0; j < SCREEN_HEIGHT; j++) {
            chip8->gfx[i][j] = 0;
        }
    }
}