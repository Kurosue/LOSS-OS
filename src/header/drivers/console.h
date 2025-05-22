#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include "drivers/framebuffer.h"
#include "drivers/keyboard.h"
#include "lib/font.h"
#include "drivers/graphics.h"

// Console state
#define MAX_COLS (640/8)
#define MAX_ROWS (480/8)

void console_init(void);
void update_cursor(uint8_t text_color);
void putchar(char c, uint8_t text_color);
void puts(char* string, uint32_t count, uint8_t text_color);

#endif