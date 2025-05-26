#include <stdint.h>

struct cmos_reader {
    uint8_t century;
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

// Function to print a number with leading zero if needed
void print_number(uint8_t num) {
    if (num < 10) {
        char c = '0';
        syscall(5, (uint32_t)c, 0xF, 0);
    }
    
    // Print tens digit
    if (num >= 10) {
        char tens = '0' + (num / 10);
        syscall(5, (uint32_t)tens, 0xF, 0);
    }
    
    // Print ones digit
    char ones = '0' + (num % 10);
    syscall(5, (uint32_t)ones, 0xF, 0);
}

int main(void) {
    struct cmos_reader time_data;
    
    // Display initial message
    const char *msg = "Clock running...\n";
    for (int i = 0; msg[i] != '\0'; i++) {
        syscall(5, (uint32_t)msg[i], 0xA, 0);
    }
    
    // Continuous clock display
    while (1) {
        // Get updated time from CMOS
        syscall(8, (uint32_t)&time_data, 0, 0);
        
        // Since your CMOS driver sets binary mode (0x04), values are already in binary
        // No BCD conversion needed
        uint8_t hour = time_data.hour;
        uint8_t minute = time_data.minute;
        uint8_t second = time_data.second;
        
        // Position cursor at bottom right (row 24, col 70)
        // Using ANSI escape sequences
        const char *clear_line = "\033[24;70H\033[K"; // Move and clear to end of line
        for (int i = 0; clear_line[i] != '\0'; i++) {
            syscall(5, (uint32_t)clear_line[i], 0xF, 0);
        }
        
        // Display time in HH:MM:SS format
        print_number(hour);
        char colon = ':';
        syscall(5, (uint32_t)colon, 0xF, 0);
        
        print_number(minute);
        syscall(5, (uint32_t)colon, 0xF, 0);
        
        print_number(second);
        
        // Simple delay - approximately 1 second
        // This is a crude busy wait, but works for demonstration
        for (volatile int i = 0; i < 8000000; i++) {
            // Busy wait
        }
    }
    
    return 0;
}