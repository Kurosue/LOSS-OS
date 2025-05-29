#include "commands/mv.h"
#include "commands/cp.h"
#include "commands/rm.h"
#include "filesystem/ext2.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void mv(uint32_t current_inode, int argc, char *argv[]) {
    
    if (argc < 3) {
        const char *msg = "Usage: mv <source> <destination>\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        return;
    }

    char *source = argv[1];
    char *destination = argv[2];

    // Source exists or not
    uint32_t src_inode = get_inode_by_name(current_inode, source);
    if (src_inode == 0) {
        const char *msg = "mv: source not found: ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)source, strlen(source), 0xF);
        const char *newline = "\n\n";
        syscall(6, (uint32_t)newline, 2, 0xF);
        return;
    }

    // Destination already exists or not
    uint32_t dest_inode = get_inode_by_name(current_inode, destination);
    if (dest_inode != 0) {
        const char *msg = "mv: destination already exists: ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)destination, strlen(destination), 0xF);
        const char *newline = "\n\n";
        syscall(6, (uint32_t)newline, 2, 0xF);
        return;
    }

    // Source is a directory or not
    char test_buffer[BLOCK_SIZE];
    struct EXT2DriverRequest testReq = {
        .buf = &test_buffer,
        .name = ".",
        .parent_inode = src_inode,
        .buffer_size = BLOCK_SIZE,
        .name_len = 1,
    };

    if(src_inode == 2) {
        const char *msg = "mv: cannot access /bin directory\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
        return;
    }
    
    int32_t testret_code;
    syscall(1, (uint32_t)&testReq, (uint32_t)&testret_code, 0);

    bool is_directory = (testret_code == 0);

    // Strategy: Copy then delete (move = copy + delete)
    int copy_result;

    if (is_directory) {
        // Move directory: copy recursively then remove recursively
        copy_result = copy_dir_recursive(current_inode, source, current_inode, destination);
    }
    else {
        // Move file: copy then remove
        copy_result = copy_file(current_inode, source, current_inode, destination);
    }

    if (copy_result == 0) {

        // Copy successful, now remove source
        if (is_directory) {
            remove_recursive(current_inode, source);
        }
        else {
            struct EXT2DriverRequest deleteReq = {
                .buf = NULL,
                .name = source,
                .parent_inode = current_inode,
                .buffer_size = 0,
                .name_len = strlen(source),
                .is_directory = false,
            };

            int32_t delete_ret_code;
            syscall(3, (uint32_t)&deleteReq, (uint32_t)&delete_ret_code, 0);

            if (delete_ret_code != 0) {
                const char *msg = "mv: failed to remove source after copy: ";
                syscall(6, (uint32_t)msg, strlen(msg), 0x4);
                syscall(6, (uint32_t)source, strlen(source), 0xF);
                const char *newline = "\n\n";
                syscall(6, (uint32_t)newline, 1, 0xF);
                return;
            }
        }

        // Move successful
        const char *msg = "Move completed successfully\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
    }
    else {
        const char *msg = "mv: failed to copy source to destination\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
    }
}