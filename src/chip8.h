#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef struct {
    uint16_t opcode;
    uint8_t ram[MEMORY_SIZE];
    uint8_t v[16]; // registers
    uint16_t I;
    uint16_t pc;

    uint16_t stack[16];
    uint16_t sp; // stackPointer

    uint8_t gfx[SCREEN_WIDTH][SCREEN_HEIGHT];
    bool drawFlag;
    bool runFlag;

    bool keys[16];
    bool wasKeyPressed;

    uint8_t delay_timer;
    uint8_t sound_timer;
} Chip8;

void chip8_setup(Chip8 *chip8);

void chip8_load_game(Chip8 *chip8, char* game);

void chip8_update(Chip8 *chip8);

void chip8_set_keys(Chip8 *chip8);

void chip8_clear_display(Chip8 *chip8);