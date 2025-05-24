#include <stdint.h>

/**
 * Move/rename files and directories command
 * @param current_inode Current directory inode
 * @param argc Argument count
 * @param argv Argument vector
 */
void mv(uint32_t current_inode, int argc, char *argv[]);