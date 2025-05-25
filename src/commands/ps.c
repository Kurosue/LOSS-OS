#include "commands/ps.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void ps() {
    struct ProcessControlBlock *processList;
    syscall(11, (uint32_t)&processList, 0, 0);
    syscall(6, (uint32_t)"PID\tName\tState\n", strlen("PID\tName\tState\n"), 0xC);
    for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (process_manager_state.is_process_active[i]) {
            struct ProcessControlBlock *pcb = &processList[i];

            char state[10];
            switch (pcb->metadata.process_state) {
                case RUNNING:
                    strcpy(state, "RUNNING");
                    break;
                case READY:
                    strcpy(state, "READY");
                    break;
                case BLOCK:
                    strcpy(state, "BLOCKED");
                    break;
                default:
                    strcpy(state, "UNKNOWN");
            }

            char *pid_str = itoa(pcb->metadata.pid);
            syscall(6, (uint32_t)pid_str, strlen(pid_str), 0xC);
            syscall(6, (uint32_t)"\t", 1, 0xC);
            syscall(6, (uint32_t)pcb->metadata.name, strlen(pcb->metadata.name), 0xC);
            syscall(6, (uint32_t)"\t", 1, 0xC);
            syscall(6, (uint32_t)state, strlen(state), 0xC);
            syscall(6, (uint32_t)"\n", 1, 0xC);
        }
    }
}

char* itoa(int i) {
    static char buffer[12];
    char *ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    if (i == 0) {
        *--ptr = '0';
        return ptr;
    }

    int sign = i < 0 ? -1 : 1;
    if (sign < 0) {
        i = -i;
    }

    while (i > 0) {
        *--ptr = (i % 10) + '0';
        i /= 10;
    }

    if (sign < 0) {
        *--ptr = '-';
    }

    return ptr;
}
