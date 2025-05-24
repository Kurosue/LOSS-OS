#include "commands/cp.h"
#include "commands/rm.h"
#include "filesystem/ext2.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

// Helper function to copy a file
int copyFile(uint32_t src_inode, const char *src_name, uint32_t dest_inode, const char *dest_name) {
    // Read source file
    struct BlockBuffer file_buffer[16]; // Support up to 16 blocks
    struct EXT2DriverRequest readReq = {
        .buf                   = &file_buffer,
        .name                  = (char *)src_name,
        .parent_inode          = src_inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = strlen(src_name),
    };
    
    int32_t readRetCode;
    syscall(0, (uint32_t) &readReq, (uint32_t) &readRetCode, 0);
    
    if (readRetCode != 0) {
        const char *msg = "cp: cannot read source file ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)src_name, strlen(src_name), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return -1;
    }
    
    // Write to destination
    struct EXT2DriverRequest writeReq = {
        .buf                   = &file_buffer,
        .name                  = (char *)dest_name,
        .parent_inode          = dest_inode,
        .buffer_size           = BLOCK_SIZE * 16, // Will be adjusted by filesystem
        .name_len              = strlen(dest_name),
        .is_directory          = false,
    };
    
    int32_t writeRetCode;
    syscall(2, (uint32_t) &writeReq, (uint32_t) &writeRetCode, 0);
    
    if (writeRetCode != 0) {
        const char *msg = "cp: cannot write to destination ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)dest_name, strlen(dest_name), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return -1;
    }
    
    return 0;
}

// Helper function to copy a directory recursively
int copyDirectoryRecursive(uint32_t src_inode, const char *src_name, uint32_t dest_inode, const char *dest_name) {
    // First create the destination directory
    struct EXT2DriverRequest mkdirReq = {
        .buf                   = NULL,
        .name                  = (char *)dest_name,
        .parent_inode          = dest_inode,
        .buffer_size           = 0,
        .name_len              = strlen(dest_name),
        .is_directory          = true,
    };
    
    int32_t mkdirRetCode;
    syscall(2, (uint32_t) &mkdirReq, (uint32_t) &mkdirRetCode, 0);
    
    if (mkdirRetCode != 0) {
        const char *msg = "cp: cannot create destination directory ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)dest_name, strlen(dest_name), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return -1;
    }
    
    // Get source directory inode
    uint32_t src_dir_inode = getInodeByName(src_inode, src_name);
    if (src_dir_inode == 0) {
        const char *msg = "cp: source directory not found ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)src_name, strlen(src_name), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return -1;
    }
    
    // Get destination directory inode
    uint32_t dest_dir_inode = getInodeByName(dest_inode, dest_name);
    if (dest_dir_inode == 0) {
        const char *msg = "cp: destination directory creation failed ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)dest_name, strlen(dest_name), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return -1;
    }
    
    // Read source directory contents
    char data_buffer[BLOCK_SIZE * 16] = {};
    struct EXT2DriverRequest reqDir = {
        .buf                   = &data_buffer,
        .name                  = ".",
        .parent_inode          = src_dir_inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = 1,
    };
    
    int32_t retCode;
    syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0);
    
    if (retCode != 0) {
        const char *msg = "cp: cannot read source directory ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)src_name, strlen(src_name), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return -1;
    }
    
    // Copy each entry
    uint32_t current_offset = 0;
    while (current_offset < reqDir.buffer_size) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(data_buffer + current_offset);
        
        if (entry->inode == 0) {
            if (entry->rec_len == 0) break;
            current_offset += entry->rec_len;
            continue;
        }
        
        if (current_offset + entry->rec_len > reqDir.buffer_size || 
            entry->rec_len < (sizeof(struct EXT2DirectoryEntry) + entry->name_len)) {
            break;
        }
        
        char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
        
        // Skip . and .. entries
        if (!(entry->name_len == 1 && entry_name[0] == '.') &&
            !(entry->name_len == 2 && entry_name[0] == '.' && entry_name[1] == '.')) {
            
            // Create null-terminated string
            char child_name[256];
            if (entry->name_len < 255) {
                memcpy(child_name, entry_name, entry->name_len);
                child_name[entry->name_len] = '\0';
                
                if (entry->file_type == EXT2_FT_DIR) {
                    // Recursively copy subdirectory
                    copyDirectoryRecursive(src_dir_inode, child_name, dest_dir_inode, child_name);
                } else if (entry->file_type == EXT2_FT_REG_FILE) {
                    // Copy file
                    copyFile(src_dir_inode, child_name, dest_dir_inode, child_name);
                }
            }
        }
        
        current_offset += entry->rec_len;
    }
    
    return 0;
}

void cp(uint32_t current_inode, int argc, char *argv[]) {
    bool recursive = false;
    int start_index = 1;
    
    if (argc < 3) {
        const char *msg = "Usage: cp [-r] <source> <destination>\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        return;
    }
    
    // Check for -r flag
    if (argc >= 2 && memcmp(argv[1], "-r", 2) == 0 && strlen(argv[1]) == 2) {
        recursive = true;
        start_index = 2;
        
        if (argc < 4) {
            const char *msg = "Usage: cp [-r] <source> <destination>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            return;
        }
    }
    
    char *source = argv[start_index];
    char *destination = argv[start_index + 1];
    
    // Check if source exists and get its type
    uint32_t src_inode = getInodeByName(current_inode, source);
    if (src_inode == 0) {
        const char *msg = "cp: source not found: ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)source, strlen(source), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return;
    }
    
    // Try to read as directory to determine type
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
    
    if (is_directory && !recursive) {
        const char *msg = "cp: ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)source, strlen(source), 0xF);
        const char *msg2 = " is a directory (use -r for recursive copy)\n";
        syscall(6, (uint32_t)msg2, strlen(msg2), 0x4);
        return;
    }
    
    if (is_directory) {
        // Copy directory recursively
        if (copyDirectoryRecursive(current_inode, source, current_inode, destination) == 0) {
            const char *msg = "Directory copied successfully\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }
    } else {
        // Copy file
        if (copyFile(current_inode, source, current_inode, destination) == 0) {
            const char *msg = "File copied successfully\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }
    }
}