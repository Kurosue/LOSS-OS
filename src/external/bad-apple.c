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

#define ENC_HEIGHT 30
#define ENC_WIDTH 60
#define BLOCK_ROWS (ENC_HEIGHT / 8)
#define BLOCK_COLS (ENC_WIDTH / 8)
#define BLOCK_COUNT (BLOCK_ROWS * BLOCK_COLS)
#define FPS 2

int main(void) {
    // bad_apple_bin and bad_apple_bin_len come from bad_apple.h
    const uint8_t *data = bad_apple_bin;
    unsigned int file_size = bad_apple_bin_len;

    uint8_t last[BLOCK_COUNT][8] = {{0}};
    uint8_t curr[BLOCK_COUNT][8];
    unsigned int idx = 0;
    // int delay_us = 1000000 / FPS;

    syscall(8, 0, 0, 0);
    while (idx < file_size) {
        uint8_t count = data[idx++];

        if (count == 255) {
            // full frame
            for (int b = 0; b < BLOCK_COUNT; b++) {
                for (int i = 0; i < 8; i++) {
                    curr[b][i] = data[idx++];
                }
            }
        } else {
            // start from last
            for (int b = 0; b < BLOCK_COUNT; b++) {
                for (int i = 0; i < 8; i++) {
                    curr[b][i] = last[b][i];
                }
            }
            // apply deltas
            for (int d = 0; d < count; d++) {
                uint8_t bidx = data[idx++];
                for (int i = 0; i < 8; i++) {
                    curr[bidx][i] = data[idx++];
                }
            }
        }

        // copy curr to last
        for (int b = 0; b < BLOCK_COUNT; b++) {
            for (int i = 0; i < 8; i++) {
                last[b][i] = curr[b][i];
            }
        }

        // render
        for (int by = 0; by < BLOCK_ROWS; by++) {
            for (int dy = 0; dy < 8; dy++) {
                for (int bx = 0; bx < BLOCK_COLS; bx++) {
                    int block_idx = by * BLOCK_COLS + bx;
                    uint8_t byte = curr[block_idx][dy];

                    int base_x = bx * 8 * 8;
                    int base_y = by * 8 * 8 + dy * 8;

                    for (int bit = 0; bit < 8; bit++) {
                        int pixel_x = base_x + bit * 8;
                        int pixel_y = base_y;
                        if ((byte >> (7 - bit)) & 1) {
                            syscall(16, (uint32_t)pixel_x, pixel_y, 0xF);
                        } else {

                            syscall(16, (uint32_t)pixel_x, pixel_y, 0x0);
                        }
                    }
                }
            }
        }

        // busy wait
        // syscall(13, (uint32_t) 25, 0, 0);
        for (volatile int i = 0; i < 20000000; i++) {
            // Busy wait
        }

    }

    syscall(10, 0, 0, 0);

    return 0;
}

