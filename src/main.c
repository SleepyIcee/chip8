#include "../include/raylib.h"
#include "chip8.h"
#include "display.h"
#include "config.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    srand(time(NULL));
    InitWindow(SCREEN_WIDTH * 10, SCREEN_HEIGHT * 10, "chip8");

    if (argc < 2) {
        printf("Usage: %s <game>\n", argv[0]);
        return 1;
    }

    Chip8 chip8;

    chip8_setup(&chip8);
    chip8_load_game(&chip8, argv[1]);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        if (chip8.runFlag) {
            chip8_update(&chip8);
        }

        if (chip8.drawFlag) {
            display_draw(chip8.gfx);
            chip8.drawFlag = false;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            CloseWindow();
        }

        chip8_set_keys(&chip8);
    }
}
