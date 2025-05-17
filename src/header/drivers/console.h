#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include "../header/drivers/framebuffer.h"
#include "../header/drivers/keyboard.h"
#include "../header/lib/font.h"
#include "../header/drivers/graphics.h"

// Console state
#define MAX_COLS (640/8)
#define MAX_ROWS (480/8)

void console_init(void);
void console_putc(char c);
void update_cursor(void);
void console_poll_input(void);


#endif // CONSOLE_H