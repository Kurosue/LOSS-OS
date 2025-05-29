#include "commands/kill.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void kill(int argc, char *argv[]) {

    if (argc < 2) {
        const char *msg = "Usage: kill <pid>\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
        return;
    }

    int pid = atoi(argv[1]);

    if (pid == -1) {
        const char *msg = "Invalid PID\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
        return;
    }

    bool ret_code;
    syscall(11, (uint32_t)pid, (uint32_t)&ret_code, 0);

    if (!ret_code) {
        const char *msg = "No such process\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }
}

int atoi(char *str) {

    int result = 0;
    int sign = 1;
    int valid = 0;

    while (*str == ' ' || *str == '\t')
        str++;

    if (*str == '-') {
        sign = -1;
        str++;
    }
    else if (*str == '+') {
        str++;
    }

    while (*str != '\0') {
        if (*str >= '0' && *str <= '9') {
            result = result * 10 + (*str - '0');
            valid = 1;
        }
        else {
            return -1;
        }
        str++;
    }

    if (!valid) return -1;

    return sign * result;
}