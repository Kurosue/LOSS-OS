#include <stdint.h>

struct cmos_reader {
    unsigned char second, minute, hour, day, month, century;
    unsigned int year;
};
struct draw_info {
    uint32_t x, y;
    uint8_t color, character;
};

struct draw_info num_info;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

int strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Function to print a number with leading zero if needed
void print_number(uint8_t num) {
    num_info.color = 0xF;
    
    if (num < 10) {
        num_info.character = '0';
    }
    
    // Print tens digit
    if (num >= 10) {
        num_info.character = '0' + (num / 10);
    }
    syscall(15, (uint32_t) &num_info, 0, 0);
    num_info.x += 1;
    
    // Print ones digit
    num_info.character = '0' + (num % 10);
    syscall(15, (uint32_t) &num_info, 0, 0);
    num_info.x += 1;
}

int main(void) {
    struct cmos_reader time_data;
    
    // Display initial message
    const char *msg = "\nClock running...\n\n";
    syscall(6, (uint32_t)msg, strlen(msg), 0xA);
    
    // Continuous clock display
    while (1) {
        num_info.x = 70;
        num_info.y = 59;
        
        // Get updated time from CMOS
        syscall(14, (uint32_t)&time_data, 0, 0);
        
        uint8_t hour = (time_data.hour + 7) % 24;
        uint8_t minute = time_data.minute;
        uint8_t second = time_data.second;
        
        // Position cursor at bottom right (row 24, col 70)
        
        // Display time in HH:MM:SS format
        print_number(hour);
        num_info.character = ':';
        syscall(15, (uint32_t) &num_info, 0, 0);
        num_info.x += 1;
        
        print_number(minute);
        num_info.character = ':';
        syscall(15, (uint32_t) &num_info, 0, 0);
        num_info.x += 1;
        
        print_number(second);
        
        // Simple delay - approximately 1 second
        // This is a crude busy wait, but works for demonstration
        for (volatile int i = 0; i < 10000000; i++) {
            // Busy wait
        }
    }
    
    return 0;
}
