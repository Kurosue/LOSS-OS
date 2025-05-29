#include "drivers/keyboard.h"


const char keyboard_scancode_1_to_ascii_map[256] = {
    0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
  'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
  'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
  'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
    0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

const char keyboard_scancode_1_to_ascii_uppercase_map[256] = {
      0, 0x1B, '!', '@', '#', '$', '%', '^',  '&', '*', '(',  ')',  '_', '+', '\b', '\t',
    'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',  'O', 'P', '{',  '}', '\n',   0,  'A',  'S',
    'D',  'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',   0,  '|',  'Z', 'X',  'C',  'V',
    'B',  'N', 'M', '<', '>', '?',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

static struct KeyboardDriverState keyboard_state = {
    .read_extended_mode = false,
    .keyboard_input_on = false,
    .keyboard_buffer = 0,
    .shift_pressed = false,
    .caps_lock_on = false,
    .special_key = 0
};

void keyboard_state_activate(void) {
    keyboard_state.keyboard_input_on = true;
    keyboard_state.read_extended_mode = false;
    keyboard_state.keyboard_buffer = 0;
    keyboard_state.shift_pressed = false;
    keyboard_state.caps_lock_on = false;
    keyboard_state.special_key = KEY_NONE;
}

void keyboard_state_deactivate(void) {
    keyboard_state.keyboard_input_on = false;
}

void get_keyboard_buffer(char *buf) {
    *buf = keyboard_state.keyboard_buffer;
    keyboard_state.keyboard_buffer = 0;
}

uint8_t get_special_key(void) {
    uint8_t key = keyboard_state.special_key;
    keyboard_state.special_key = KEY_NONE;
    return key;
}

bool is_shift_pressed(void) {
    return keyboard_state.shift_pressed;
}

bool is_caps_lock_on(void) {
    return keyboard_state.caps_lock_on;
}

void keyboard_isr(void) {
    // Read from the keyboard data port for any IRQ1
    uint8_t scancode = in(KEYBOARD_DATA_PORT);
    
    // Process the scancode only if keyboard input is enabled
    if (keyboard_state.keyboard_input_on) {

        // Check if this is an extended scancode byte
        if (scancode == EXTENDED_SCANCODE_BYTE) {
            keyboard_state.read_extended_mode = true;
        } 
        // Process regular scancode
        else {
            bool is_break_code = (scancode & 0x80) != 0;
            uint8_t clean_scancode = scancode & 0x7F;
            
            // Handle regular make/break codes
            if (!keyboard_state.read_extended_mode) {

                // Handle shift keys
                if (clean_scancode == SCANCODE_SHIFT_LEFT || clean_scancode == SCANCODE_SHIFT_RIGHT) {
                    keyboard_state.shift_pressed = !is_break_code; // Set when pressed, clear when released
                }

                // Handle caps lock (toggle on key press, not on release)
                else if (clean_scancode == SCANCODE_CAPS_LOCK && !is_break_code) {
                    keyboard_state.caps_lock_on = !keyboard_state.caps_lock_on; // Toggle
                }

                // Handle other keys on make code (key press)
                else if (!is_break_code) {

                    // Determine if we should use uppercase based on shift and caps lock
                    bool use_uppercase = keyboard_state.shift_pressed ^ keyboard_state.caps_lock_on;
                    
                    // Only uppercase letters if caps lock is on (not symbols)
                    if (keyboard_state.caps_lock_on && !keyboard_state.shift_pressed) {
                        char c = keyboard_scancode_1_to_ascii_map[scancode];
                        if (c >= 'a' && c <= 'z') {
                            keyboard_state.keyboard_buffer = c - 32; // Convert to uppercase
                        } 
                        else {
                            keyboard_state.keyboard_buffer = c;
                        }
                    } 
                    else {
                        // Use either normal or uppercase map based on shift state
                        char mapped = use_uppercase
                            ? keyboard_scancode_1_to_ascii_uppercase_map[scancode]
                            : keyboard_scancode_1_to_ascii_map[scancode];

                        if (mapped) {
                            keyboard_state.keyboard_buffer = mapped;
                        }
                    }
                }
            } 

            // Handle extended scancodes (like arrow keys)
            else if (!is_break_code) {
                switch (clean_scancode) {
                    case EXT_SCANCODE_UP:
                        keyboard_state.special_key = KEY_UP;
                        break;
                    case EXT_SCANCODE_DOWN:
                        keyboard_state.special_key = KEY_DOWN;
                        break;
                    case EXT_SCANCODE_LEFT:
                        keyboard_state.special_key = KEY_LEFT;
                        break;
                    case EXT_SCANCODE_RIGHT:
                        keyboard_state.special_key = KEY_RIGHT;
                        break;
                    default:
                        // Handle other extended keys if needed
                        break;
                }

                // Clear extended mode after processing
                keyboard_state.read_extended_mode = false;
            } 
            else {
                // Break code (key release) for extended key
                keyboard_state.read_extended_mode = false;
            }
        }
    }
    
    // Acknowledge the interrupt to the PIC (IRQ1)
    pic_ack(IRQ_KEYBOARD);
}