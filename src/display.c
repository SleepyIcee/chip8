#include "raylib.h"
#include <stdint.h>
#include "config.h"


void display_draw(uint8_t gfx[][SCREEN_HEIGHT]) {
    BeginDrawing();
    ClearBackground(BLACK);
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        for (int j = 0; j < SCREEN_HEIGHT; j++) {
            if (gfx[i][j] == 1) {
                DrawRectangle(i * 10, j * 10, 10, 10, WHITE);
            }
        }
    }
    EndDrawing();
}