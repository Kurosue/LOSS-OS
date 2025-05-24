#include "commands/mv.h"
#include "commands/cp.h"
#include "commands/rm.h"
#include "filesystem/ext2.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void mv(uint32_t current_inode, int argc, char *argv[]) {
    if (argc < 3) {
        const char *msg = "Usage: mv <source> <destination>\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        return;
    }
    
    char *source = argv[1];
    char *destination = argv[2];
    
    // Check if source exists
    uint32_t src_inode = getInodeByName(current_inode, source);
    if (src_inode == 0) {
        const char *msg = "mv: source not found: ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)source, strlen(source), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return;
    }
    
    // Check if destination already exists
    uint32_t dest_inode = getInodeByName(current_inode, destination);
    if (dest_inode != 0) {
        const char *msg = "mv: destination already exists: ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)destination, strlen(destination), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return;
    }
    
    // Determine if source is a directory
    char test_buffer[BLOCK_SIZE];
    struct EXT2DriverRequest testReq = {
        .buf                   = &test_buffer,
        .name                  = ".",
        .parent_inode          = src_inode,
        .buffer_size           = BLOCK_SIZE,
        .name_len              = 1,
    };
    
    int32_t testRetCode;
    syscall(1, (uint32_t) &testReq, (uint32_t) &testRetCode, 0);
    
    bool is_directory = (testRetCode == 0);
    
    // Strategy: Copy then delete (move = copy + delete)
    int copy_result;
    
    if (is_directory) {
        // Move directory: copy recursively then remove recursively
        copy_result = copyDirectoryRecursive(current_inode, source, current_inode, destination);
    } else {
        // Move file: copy then remove
        copy_result = copyFile(current_inode, source, current_inode, destination);
    }
    
    if (copy_result == 0) {
        // Copy successful, now remove source
        if (is_directory) {
            removeRecursive(current_inode, source);
        } else {
            struct EXT2DriverRequest deleteReq = {
                .buf                   = NULL,
                .name                  = source,
                .parent_inode          = current_inode,
                .buffer_size           = 0,
                .name_len              = strlen(source),
                .is_directory          = false,
            };
            
            int32_t deleteRetCode;
            syscall(3, (uint32_t) &deleteReq, (uint32_t) &deleteRetCode, 0);
            
            if (deleteRetCode != 0) {
                const char *msg = "mv: failed to remove source after copy: ";
                syscall(6, (uint32_t)msg, strlen(msg), 0x4);
                syscall(6, (uint32_t)source, strlen(source), 0xF);
                const char *newline = "\n";
                syscall(6, (uint32_t)newline, 1, 0xF);
                return;
            }
        }
        
        // Move successful
        const char *msg = "Move completed successfully\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
    } else {
        const char *msg = "mv: failed to copy source to destination\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
    }
}