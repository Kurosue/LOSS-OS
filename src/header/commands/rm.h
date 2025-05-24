#include <stdint.h>
#include "filesystem/ext2.h"

/**
 * Make directory command
 * @param argc Argument count
 * @param argv Argument vector
 * @param current_inode Current directory inode
 */
void rm(uint32_t current_inode, int argc,char *argv[]);