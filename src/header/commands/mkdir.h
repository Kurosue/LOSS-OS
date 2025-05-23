#include <stdint.h>

/**
 * Make directory command
 * @param dirname Name of directory to create
 * @param current_inode Current directory inode
 */
void mkdir(uint32_t current_inode, int argc, char *argv[]);