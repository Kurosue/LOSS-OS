#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "header/filesystem/ext2.h"
#include "header/drivers/disk.h"
#include "header/lib/string.h"

// Global variable
uint8_t *image_storage;
uint8_t *file_buffer;

void read_blocks(void *ptr, uint32_t logical_block_address, uint8_t block_count) {
    for (int i = 0; i < block_count; i++) {
        memcpy(
            (uint8_t*) ptr + BLOCK_SIZE*i, 
            image_storage + BLOCK_SIZE*(logical_block_address+i), 
            BLOCK_SIZE
        );
    }
}

void write_blocks(const void *ptr, uint32_t logical_block_address, uint8_t block_count) {
    for (int i = 0; i < block_count; i++) {
        memcpy(
            image_storage + BLOCK_SIZE*(logical_block_address+i), 
            (uint8_t*) ptr + BLOCK_SIZE*i, 
            BLOCK_SIZE
        );
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "inserter: ./inserter <file to insert> <parent cluster index> <storage>\n");
        exit(1);
    }

    // Read storage into memory, requiring 4 MB memory
    image_storage = malloc(4*1024*1024);
    file_buffer   = malloc(4*1024*1024);
    FILE *fptr    = fopen(argv[3], "r");
    fread(image_storage, 4*1024*1024, 1, fptr);
    fclose(fptr);

    // Read target file, assuming file is less than 4 MiB
    FILE *fptr_target = fopen(argv[1], "r");
    size_t filesize   = 0;
    if (fptr_target == NULL)
        filesize = 0;
    else {
        fread(file_buffer, 4*1024*1024, 1, fptr_target);
        fseek(fptr_target, 0, SEEK_END);
        filesize = ftell(fptr_target);
        fclose(fptr_target);
    }
    char *name = argv[1];
    unsigned int filename_length = strlen(name);

    printf("Filename : %s\n",  name);
    printf("Filename length: %d\n", filename_length);
    printf("Filesize : %ld bytes\n", filesize);

    // EXT2 operations
    initialize_filesystem_ext2();
    struct EXT2DriverRequest request;
    struct EXT2DriverRequest reqread;
    uint8_t *read_buffer = malloc(4*1024*1024);



    request.name = name;
    request.name_len = filename_length;
    if(memcmp(name, "bin", 3) == 0)
    {
        request.buf = NULL;
        request.is_directory = true;
        request.buffer_size = 0;
    }
    else
    {
        request.buf = file_buffer;
        request.is_directory = false;
        request.buffer_size = filesize;
    }
    request.is_directory = false;
    sscanf(argv[2], "%u", &request.parent_inode);
    sscanf(argv[1], "%s", request.name);

    reqread = request;
    reqread.buf = read_buffer;
    int retcode = read(reqread);
    if (retcode == 0)
    {
        bool same = true;
        for (uint32_t i = 0; i < filesize; i++)
        {
            if (read_buffer[i] != file_buffer[i])
            {
                printf("not same\n");
                same = false;
                break;
            }
        }
        if (same)
        {
            printf("same\n");
        }
    }

    // no accident happen
    bool is_replace = false;

    retcode = write(&request);
    if (retcode == 1 && is_replace)
    {
        retcode = delete (request);
        retcode = write(&request);
    }
    if (retcode == 0)
        puts("Write success");
    else if (retcode == 1)
        puts("Error: File/folder name already exist");
    else if (retcode == 2)
        puts("Error: Invalid parent node index");
    else
        puts("Error: Unknown error");

    // Write image in memory into original, overwrite them
    fptr = fopen(argv[3], "w");
    fwrite(image_storage, 4 * 1024 * 1024, 1, fptr);
    fclose(fptr);

    return 0;
}