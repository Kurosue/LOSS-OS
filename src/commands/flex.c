#include "commands/flex.h"
#include <stdint.h>

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void flex(int argc, char **argv) {

    if (argc >= 2) {
        
        char *arg = argv[1];

        if ((memcmp(arg, "-h", 2) == 0 && strlen(arg) == 2) || 
            (memcmp(arg, "--help", 6) == 0 && strlen(arg) == 6)) {

            const char *msg = "Find any file/folder with name specified.\n";
            const char *msg2 = "\n\nUsage: find <name>\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0xF);
            syscall(6, (uint32_t)msg2, strlen(msg), 0xB);
            return;
        } 
    } 

    char* text_list[8][4] = {
        {"'##:::::::", ":'#######:::'######::", ":'######::", "LOSS"},
        {" ##:::::::", "'##.... ##:'##... ##:", "'##... ##:", "----"},
        {" ##:::::::", " ##:::: ##: ##:::..::", " ##:::..::", "Created with love by:"},
        {" ##:::::::", " ##:::: ##:. ######::", ". ######::", " - Refki Alfarizi"},
        {" ##:::::::", " ##:::: ##::..... ##:", ":..... ##:", " - Razi Rachman Widyadhana"},
        {" ##:::::::", " ##:::: ##:'##::: ##:", "'##::: ##:", " - Muhammad Aditya Rahmadeni"},
        {" ########:", ". #######::. ######::", ". ######::", " - Muhammad Luqman Hakim"},
        {"........::", ":.......::::......:::", ":......:::", ""},
    };

    const char *newline = "\n";
    const char *margin = "    ";

    syscall(6, (uint32_t) newline, strlen(newline), 0x7);

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            char* word = text_list[i][j];
            for (int k = 0; k < (int)strlen(word); k++) {

                char chr[2];
                chr[0] = word[k];
                chr[1] = '\0';

                if (chr[0] == '.' || chr[0] == ':'){
                    syscall(6, (uint32_t)chr, strlen(chr), 0x7);
                    continue;
                }

                // heheheh
                uint32_t color = (j == 1 ? 0xB : 0xD);
                color = (j == 3 ? 0xF : color);
                syscall(6, (uint32_t)chr, strlen(chr), color);
            }

            if (j == 2) {
                syscall(6, (uint32_t)margin, strlen(margin), 0xF);
            }
        }
        syscall(6, (uint32_t) newline, strlen(newline), 0x7);
    }
    syscall(6, (uint32_t) newline, strlen(newline), 0x7);
    syscall(6, (uint32_t) newline, strlen(newline), 0x7);
}