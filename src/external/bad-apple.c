#include <stdint.h>
#include "bad-apple.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

#include <stdint.h>
#include "bad-apple.h"  // Generated via `xxd -i bad_apple.bin > bad_apple.h`

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
    int delay_us = 1000000 / FPS;

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
        syscall(8, 0, 0, 0);
        for (int by = 0; by < BLOCK_ROWS; by++) {
            for (int dy = 0; dy < 8; dy++) {
                for (int bx = 0; bx < BLOCK_COLS; bx++) {
                    uint8_t byte = curr[by * BLOCK_COLS + bx][dy];
                    for (int bit = 7; bit >= 0; bit--) {
                        syscall(5, (uint32_t) ((byte >> bit) & 1 ? '#' : ' '), 0xF, 0);
                    }
                }
                syscall(5, (uint32_t) '\n', 0xF, 0);
            }
        }

        // busy wait
        for(volatile int i = 0; i < delay_us * 10; i++) {
            // This is a busy wait loop to simulate the delay.
            // In a real application, you might want to use a more efficient sleep function.
        }
    }

    return 0;
}

