#include <stdint.h>

/**
 * Helper function to copy a file
 * @param src_inode Source directory inode
 * @param src_name Source file name
 * @param dest_inode Destination directory inode
 * @param dest_name Destination file name
 * @return 0 on success, error code on failure
 */
int copyFile(uint32_t src_inode, const char *src_name, uint32_t dest_inode, const char *dest_name);

/**
 * Helper function to copy a directory recursively
 * @param src_inode Source directory inode
 * @param src_name Source directory name
 * @param dest_inode Destination directory inode
 * @param dest_name Destination directory name
 * @return 0 on success, error code on failure
 */
int copyDirectoryRecursive(uint32_t src_inode, const char *src_name, uint32_t dest_inode, const char *dest_name);

/**
 * Copy files and directories command
 * @param current_inode Current directory inode
 * @param argc Argument count
 * @param argv Argument vector
 */
void cp(uint32_t current_inode, int argc, char *argv[]);