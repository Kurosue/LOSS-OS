#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../cpu/interrupt.h"

// Scancode definitions for important keys
#define SCANCODE_SHIFT_LEFT  0x2A
#define SCANCODE_SHIFT_RIGHT 0x36
#define SCANCODE_CAPS_LOCK   0x3A

// Extended scancodes for arrow keys
#define EXT_SCANCODE_UP        0x48
#define EXT_SCANCODE_DOWN      0x50
#define EXT_SCANCODE_LEFT      0x4B
#define EXT_SCANCODE_RIGHT     0x4D

// Special key values (for keyboard_state.special_key)
#define KEY_NONE      0x00
#define KEY_UP        0x01
#define KEY_DOWN      0x02
#define KEY_LEFT      0x03
#define KEY_RIGHT     0x04
#define KEY_HOME      0x05
#define KEY_END       0x06
#define KEY_PGUP      0x07
#define KEY_PGDN      0x08
#define KEY_DELETE    0x09

#define KEYBOARD_DATA_PORT     0x60
#define EXTENDED_SCANCODE_BYTE 0xE0

/**
 * keyboard_scancode_1_to_ascii_map[256], Convert scancode values that correspond to ASCII printables
 * How to use this array: ascii_char = k[scancode]
 * 
 * By default, QEMU using scancode set 1 (from empirical testing)
 */
extern const char keyboard_scancode_1_to_ascii_map[256];

/**
 * keyboard_scancode_1_to_ascii_uppercase_map[256], Convert scancode values to uppercase ASCII
 * Used when shift is pressed or caps lock is active
 */
extern const char keyboard_scancode_1_to_ascii_uppercase_map[256];

/**
 * KeyboardDriverState - Contain all driver states
 * 
 * @param read_extended_mode Optional, can be used for signaling next read is extended scancode (ex. arrow keys)
 * @param keyboard_input_on  Indicate whether keyboard ISR is activated or not
 * @param keyboard_buffer    Storing keyboard input values in ASCII
 * @param shift_pressed      Indicates if either shift key is currently pressed
 * @param caps_lock_on       Indicates if caps lock is toggled on
 * @param special_key        Stores special key value (arrow keys, etc.)
 */
struct KeyboardDriverState {
    bool read_extended_mode;
    bool keyboard_input_on;
    char keyboard_buffer;
    bool shift_pressed;
    bool caps_lock_on;
    uint8_t special_key;
} __attribute((packed));

/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void);

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void);

// Get keyboard buffer value and flush the buffer - @param buf Pointer to char buffer
void get_keyboard_buffer(char *buf);

// Get special key value (arrow keys, etc.) and clear it
uint8_t get_special_key(void);

// Check if shift is pressed
bool is_shift_pressed(void);

// Check if caps lock is on
bool is_caps_lock_on(void);

/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 */
void keyboard_isr(void);

#endif