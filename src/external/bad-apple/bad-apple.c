#include <stdint.h>
#include "bad-apple.h"  // Generated via `xxd -i bad_apple.bin > bad_apple.h`

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

#define ENC_HEIGHT 45
#define ENC_WIDTH 60
#define BLOCK_ROWS (ENC_HEIGHT / 8)
#define BLOCK_COLS (ENC_WIDTH / 8)
#define BLOCK_COUNT (BLOCK_ROWS * BLOCK_COLS)
#define FPS 15

int main(void) {
    // bad_apple_bin and bad_apple_bin_len come from bad_apple.h
    const uint8_t *data = bad_apple_bin;
    unsigned int file_size = bad_apple_bin_len;

    uint8_t last[BLOCK_COUNT][16] = {{0}};
    uint8_t curr[BLOCK_COUNT][16];
    unsigned int idx = 0;
    // int delay_us = 1000000 / FPS;

    syscall(8, 0, 0, 0);
     while (idx < file_size) {
        uint8_t count = data[idx++];
        
        if (count == 255) {
            // full frame
            for (int b = 0; b < BLOCK_COUNT; b++) {
                for (int i = 0; i < 16; i++) {  // 16 bytes per block
                    curr[b][i] = data[idx++];
                }
            }
        } else {
            // start from last
            for (int b = 0; b < BLOCK_COUNT; b++) {
                for (int i = 0; i < 16; i++) {
                    curr[b][i] = last[b][i];
                }
            }
            // apply deltas
            for (int d = 0; d < count; d++) {
                uint8_t bidx = data[idx++];
                for (int i = 0; i < 16; i++) {  // 16 bytes per block
                    curr[bidx][i] = data[idx++];
                }
            }
        }
        
        // copy curr to last
        for (int b = 0; b < BLOCK_COUNT; b++) {
            for (int i = 0; i < 16; i++) {
                last[b][i] = curr[b][i];
            }
        }
        
        // render
        for (int by = 0; by < BLOCK_ROWS; by++) {
            for (int dy = 0; dy < 8; dy++) {
                for (int bx = 0; bx < BLOCK_COLS; bx++) {
                    int block_idx = by * BLOCK_COLS + bx;
                    uint8_t byte1 = curr[block_idx][dy * 2];      // first 4 pixels
                    uint8_t byte2 = curr[block_idx][dy * 2 + 1];  // last 4 pixels
                    
                    int base_x = bx * 8 * 8;
                    int base_y = by * 8 * 8 + dy * 8;
                    
                    for (int px = 0; px < 4; px++) {
                        int pixel_x = base_x + px * 8;
                        int pixel_y = base_y;
                        
                        // 2 bits per pixel
                        uint8_t pixel_val = (byte1 >> (6 - px * 2)) & 3;
                        
                        uint32_t color;
                        switch (pixel_val) {
                            case 0: color = 0x0; break;  // Black
                            case 1: color = 0x8; break;  // Dark grey
                            case 2: color = 0x7; break;  // Light grey
                            case 3: color = 0xF; break;  // White
                        }
                        
                        syscall(16, (uint32_t)pixel_x, pixel_y, color);
                    }
                    
                    for (int px = 0; px < 4; px++) {
                        int pixel_x = base_x + (px + 4) * 8;
                        int pixel_y = base_y;
                        
                        uint8_t pixel_val = (byte2 >> (6 - px * 2)) & 3;
                        
                        uint32_t color;
                        switch (pixel_val) {
                            case 0: color = 0x0; break;  // Black
                            case 1: color = 0x8; break;  // Dark grey
                            case 2: color = 0x7; break;  // Light grey
                            case 3: color = 0xF; break;  // White
                        }
                        
                        syscall(16, (uint32_t)pixel_x, pixel_y, color);
                    }
                }
            }
        }
        
        // busy wait
        // syscall(13, (uint32_t) 25, 0, 0);
        // for (volatile int i = 0; i < 20000000; i++) {
        //     // Busy wait
        // }
    }
    syscall(10, 0, 0, 0);

    return 0;
}

