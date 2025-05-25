#include "commands/kill.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void kill(int argc, char *argv[]){
    if (argc < 2) {
        syscall(6, (uint32_t)"Usage: kill <pid>\n", 18, 0xC);
        return;
    }
    int pid = atoi(argv[1]);
    if(pid == -1){
        syscall(6, (uint32_t)"Invalid PID\n", 13, 0xC);
        return;
    }
    bool retcode;
    syscall(10, (uint32_t) pid, (uint32_t) &retcode, 0);
    if(!retcode)
    {
        syscall(6, (uint32_t)"No such process\n", 18, 0xC);
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
    } else if (*str == '+') {
        str++;
    }
    
    while (*str != '\0') {
        if (*str >= '0' && *str <= '9') {
            result = result * 10 + (*str - '0');
            valid = 1;
        } else {
            return -1;
        }
        str++;
    }
    if (!valid) {
        return -1;
    }

    return sign * result;
}