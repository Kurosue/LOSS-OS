#include "filesystem/ext2.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '5', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

static struct EXT2Superblock superBlock;
static struct EXT2BlockGroupDescriptorTable bgdtCache;

char *get_entry_name(void *entry) {
    // Name setelah entry( implisit)
    return (char *)entry + sizeof(struct EXT2DirectoryEntry);
}

struct EXT2DirectoryEntry *get_directory_entry(void *ptr, uint32_t offset) {
    return (struct EXT2DirectoryEntry *)((uint8_t *)ptr + offset);
}

struct EXT2DirectoryEntry *get_next_directory_entry(struct EXT2DirectoryEntry *entry) {
    // Reclen ~ offset next entry
    if (!entry) return NULL;
    return (struct EXT2DirectoryEntry *)((uint8_t *)entry + entry->rec_len);
}

uint16_t get_entry_record_len(uint8_t name_len) {

    // Reclen = sizeof(EXT2DirectoryEntry) + name_len
    uint16_t size = sizeof(struct EXT2DirectoryEntry) + name_len;
    uint16_t rec_len = (size + 3) & ~3;
    if (rec_len == 0) {
        rec_len = BLOCK_SIZE;
    }
    return rec_len;
}

uint32_t get_dir_first_child_offset(void *ptr) {

    // Dapatin offset dari entry pertama ke entry selanjutnya
    struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)ptr;
    if (!entry) return 0;
    uint32_t offset = entry->rec_len;
    entry = get_next_directory_entry(entry);
    if (!entry) return offset;
    offset += entry->rec_len;
    return offset;
}

// Asumsi file disimpan kaya matriks, jadi row == bgdt dan col == inode table. 
// 1-based inode num

uint32_t inode_to_bgd(uint32_t inode) {

    // Indeks bgdt (inode_num - 1 // INODES_PER_GROUP)
    if (inode == 0) return 0;
    return (inode - 1) / INODES_PER_GROUP;
}

uint32_t inode_to_local(uint32_t inode) {

    // Indeks bgdt (inode_num - 1 // INODES_PER_GROUP)
    if (inode == 0) return 0;
    return (inode - 1) % INODES_PER_GROUP;
}

static void sync_metadata() {

    // Update superblock dan bgdt misal ada perubahan pada inode dan block serta group
    struct BlockBuffer buf;
    memcpy(buf.buf, &superBlock, sizeof(superBlock));
    write_blocks(&buf, 1, 1);
    memcpy(buf.buf, &bgdtCache, sizeof(bgdtCache));
    write_blocks(&buf, 2, 1);
}

void init_directory_table(struct EXT2Inode *node, uint32_t inode, uint32_t parent_inode) {
    uint32_t bgd = inode_to_bgd(inode);
    uint32_t dir_block = 0;
    bool found = false;
    
    if (bgdtCache.table[0].bg_block_bitmap != 0) {
        for (uint32_t bg = bgd; bg < GROUPS_COUNT && !found; bg++) {
            if (bgdtCache.table[bg].bg_free_blocks_count > 0) {
                struct BlockBuffer bmp;
                read_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                for (uint32_t byte = 0; byte < BLOCK_SIZE; byte++) {
                    if (bmp.buf[byte] != 0xFF) {
                        for (uint8_t bit = 0; bit < 8; bit++) {
                            uint32_t bit_idx = byte * 8 + bit;
                            if (bit_idx >= BLOCKS_PER_GROUP) break;
                            if ((bmp.buf[byte] & (1 << bit)) == 0) {
                                bmp.buf[byte] |= (1 << bit);
                                write_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                                bgdtCache.table[bg].bg_free_blocks_count--;
                                superBlock.s_free_blocks_count--;
                                dir_block = bg * BLOCKS_PER_GROUP + bit_idx;
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found) break;
                }
            }
        }
    }
    
    if (!found) {
        uint32_t base_data;
        if (bgd == 0) {
            base_data = 5;
        } 
        else {
            base_data = bgd * BLOCKS_PER_GROUP + 2;
        }
        dir_block = base_data + INODES_TABLE_BLOCK_COUNT;
    }
    
    node->i_mode = EXT2_S_IFDIR;
    node->i_size = BLOCK_SIZE;
    node->i_blocks = 1;
    node->i_block[0] = dir_block;

    struct BlockBuffer buf;
    memset(&buf, 0, BLOCK_SIZE);
    
    struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)buf.buf;
    entry->inode = inode;
    entry->name_len = 1;
    entry->file_type = EXT2_FT_DIR;
    entry->rec_len = get_entry_record_len(1);
    char *name = get_entry_name(entry);
    name[0] = '.';
    
    struct EXT2DirectoryEntry *entry2 = (struct EXT2DirectoryEntry *)((uint8_t *)buf.buf + entry->rec_len);
    entry2->inode = parent_inode;
    entry2->name_len = 2;
    entry2->file_type = EXT2_FT_DIR;
    entry2->rec_len = BLOCK_SIZE - entry->rec_len;
    char *name2 = get_entry_name(entry2);
    name2[0] = '.';
    name2[1] = '.';
    
    write_blocks(&buf, dir_block, 1);
    
    if (bgdtCache.table[0].bg_block_bitmap != 0) {
        sync_metadata();
    }
}

bool is_empty_storage(void) {
    struct BlockBuffer buf;
    read_blocks(&buf, BOOT_SECTOR, 1);
    uint16_t sig = *(uint16_t *)buf.buf;
    // Cek sig sama dengan fs_signature atau tidak
    if (sig == *(uint16_t *)fs_signature) {
        return false;
    }
    return true;
}

void create_ext2(void) {
    struct BlockBuffer buf;
    memset(&buf, 0, BLOCK_SIZE);
    memcpy(buf.buf, fs_signature, BLOCK_SIZE);
    write_blocks(&buf, BOOT_SECTOR, 1);

    struct EXT2Superblock sb;
    memset(&sb, 0, sizeof(sb));
    sb.s_inodes_count = GROUPS_COUNT * INODES_PER_GROUP;
    sb.s_blocks_count = DISK_SPACE / BLOCK_SIZE;
    sb.s_r_blocks_count = 0;

    uint32_t total_blocks = sb.s_blocks_count;
    uint32_t used_blocks = 148;

    sb.s_free_blocks_count = total_blocks > used_blocks ? total_blocks - used_blocks : 0;
    sb.s_free_inodes_count = sb.s_inodes_count - 1;
    sb.s_first_data_block = 1;
    sb.s_first_ino = 1;
    sb.s_blocks_per_group = BLOCKS_PER_GROUP;
    sb.s_frags_per_group = BLOCKS_PER_GROUP;
    sb.s_inodes_per_group = INODES_PER_GROUP;
    sb.s_magic = EXT2_SUPER_MAGIC;
    sb.s_prealloc_blocks = 0;
    sb.s_prealloc_dir_blocks = 0;
    memset(&buf, 0, BLOCK_SIZE);
    memcpy(buf.buf, &sb, sizeof(sb));
    write_blocks(&buf, 1, 1);

    struct EXT2BlockGroupDescriptorTable *bgdt = (struct EXT2BlockGroupDescriptorTable *)buf.buf;
    memset(bgdt, 0, sizeof(*bgdt));
    for (uint32_t i = 0; i < GROUPS_COUNT; i++) {
        struct EXT2BlockGroupDescriptor *desc = &bgdt->table[i];
        if (i == 0) {
            desc->bg_block_bitmap = 3;
            desc->bg_inode_bitmap = 4;
            desc->bg_inode_table  = 5;
            desc->bg_free_blocks_count = BLOCKS_PER_GROUP - 22;
            desc->bg_free_inodes_count = INODES_PER_GROUP - 1;
            desc->bg_used_dirs_count = 1;
        } 
        else {
            uint32_t base = i * BLOCKS_PER_GROUP;
            desc->bg_block_bitmap = base + 0;
            desc->bg_inode_bitmap = base + 1;
            desc->bg_inode_table  = base + 2;
            desc->bg_free_blocks_count = BLOCKS_PER_GROUP - 18;
            desc->bg_free_inodes_count = INODES_PER_GROUP;
            desc->bg_used_dirs_count = 0;
        }
    }
    write_blocks(&buf, 2, 1);

    memset(&buf, 0, BLOCK_SIZE);
    struct EXT2Inode *inodes = (struct EXT2Inode *)buf.buf;
    init_directory_table(&inodes[0], 1, 1);
    write_blocks(&buf, 5, 1);
    memset(&buf, 0, BLOCK_SIZE);

    for (uint32_t blk = 6; blk < 5 + INODES_TABLE_BLOCK_COUNT; blk++) {
        write_blocks(&buf, blk, 1);
    }

    memset(&buf, 0, BLOCK_SIZE);
    for (int b = 0; b < 22; b++) {
        ((uint8_t *)buf.buf)[b / 8] |= (1 << (b % 8));
    }

    write_blocks(&buf, 3, 1);
    memset(&buf, 0, BLOCK_SIZE);
    ((uint8_t *)buf.buf)[0] |= 0x01;
    write_blocks(&buf, 4, 1);

    for (uint32_t i = 1; i < GROUPS_COUNT; i++) {
        uint32_t base = i * BLOCKS_PER_GROUP;
        memset(&buf, 0, BLOCK_SIZE);
        for (int b = 0; b < 18; b++) {
            ((uint8_t *)buf.buf)[b / 8] |= (1 << (b % 8));
        }
        write_blocks(&buf, base + 0, 1);
        memset(&buf, 0, BLOCK_SIZE);
        write_blocks(&buf, base + 1, 1);
    }
}

void initialize_filesystem_ext2(void) {
    if (is_empty_storage()) {
        create_ext2();
    }

    struct BlockBuffer buf;
    read_blocks(&buf, 1, 1);
    memcpy(&superBlock, buf.buf, sizeof(superBlock));

    read_blocks(&buf, 2, 1);
    memcpy(&bgdtCache, buf.buf, sizeof(bgdtCache));
}

bool is_directory_empty(uint32_t inode) {
    uint32_t bgd = inode_to_bgd(inode);
    uint32_t local = inode_to_local(inode);
   
    uint32_t inode_table_block = bgdtCache.table[bgd].bg_inode_table;
    uint32_t block_index = inode_table_block + (local / INODES_PER_TABLE);
    struct BlockBuffer buf;
    read_blocks(&buf, block_index, 1);
    struct EXT2Inode *node = &((struct EXT2Inode *)buf.buf)[local % INODES_PER_TABLE];

    if ((node->i_mode & EXT2_S_IFDIR) == 0) {
        return false; // Bukan dir
    }

    uint32_t dir_block = node->i_block[0];
    read_blocks(&buf, dir_block, 1);
    struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)buf.buf;

    entry = get_next_directory_entry(entry);
    if (!entry) return true;
    entry = get_next_directory_entry(entry);
    if (!entry) return true;
    return (entry->inode == 0);
}

int8_t read_directory(struct EXT2DriverRequest *prequest) {
    
    if (prequest == NULL) return -1;
    uint32_t parent_inode = prequest->parent_inode;

    if (parent_inode == 0) {
        return -1;  // Parent inode ga valid alias 0
    }

    uint32_t parent_bgd = inode_to_bgd(parent_inode);
    uint32_t parent_local = inode_to_local(parent_inode);
    struct BlockBuffer buf;
    uint32_t parent_itb = bgdtCache.table[parent_bgd].bg_inode_table;
    uint32_t parent_block_index = parent_itb + (parent_local / INODES_PER_TABLE);
    read_blocks(&buf, parent_block_index, 1);
    struct EXT2Inode *parent_node = &((struct EXT2Inode *)buf.buf)[parent_local % INODES_PER_TABLE];

    if ((parent_node->i_mode & EXT2_S_IFDIR) == 0) {
        return -1;  // Parent nya bukan folder (jaga jaga aja)
    }

    bool found = false;
    uint32_t entry_inode = 0;
    uint32_t entry_file_type = 0;
    uint32_t blocks = parent_node->i_blocks;

    for (uint32_t bi = 0; bi < blocks && !found; bi++) {

        uint32_t block_id = parent_node->i_block[bi];
        if (block_id == 0) break;

        struct BlockBuffer dirBuf;
        read_blocks(&dirBuf, block_id, 1);
        uint32_t offset = 0;

        while (offset < BLOCK_SIZE) {
            struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)((uint8_t *)dirBuf.buf + offset);
            if (entry->inode != 0) {
                uint16_t name_len = entry->name_len;
                if (name_len == prequest->name_len &&
                    memcmp(get_entry_name(entry), prequest->name, name_len) == 0) {
                    entry_inode = entry->inode;
                    entry_file_type = entry->file_type;
                    found = true;
                    break;
                }
            }
            if (entry->rec_len == 0) break;
            offset += entry->rec_len;
        }
    }

    if (!found) {
        return 2;  // Unchanged: 2 for "Folder tidak ditemukan pada parent folder"
    }
    if (entry_file_type != EXT2_FT_DIR) {
        return 1;  // Unchanged: 1 for "Bukan sebuah folder"
    }

    uint32_t target_inode = entry_inode;
    uint32_t target_bgd = inode_to_bgd(target_inode);
    uint32_t target_local = inode_to_local(target_inode);
    uint32_t target_itb = bgdtCache.table[target_bgd].bg_inode_table;
    uint32_t target_block_index = target_itb + (target_local / INODES_PER_TABLE);
    read_blocks(&buf, target_block_index, 1);
    struct EXT2Inode *target_node = &((struct EXT2Inode *)buf.buf)[target_local % INODES_PER_TABLE];

    if (target_node->i_blocks == 0) {
        return -1;  // Kalau block pada nodenya 0 alias gaada block
    }

    uint32_t dir_block = target_node->i_block[0];
    if (dir_block == 0) {
        return -1;  // Dir block null
    }

    struct BlockBuffer dataBuf;
    read_blocks(&dataBuf, dir_block, 1);

    if (prequest->buffer_size < target_node->i_size) {
        return -1;  // Kekecilan
    }

    memcpy(prequest->buf, dataBuf.buf, target_node->i_size);
    return 0;  
}

int8_t read(struct EXT2DriverRequest request) {

    uint32_t parent_inode = request.parent_inode;
    if (parent_inode == 0) {
        return -1; // Parent inode nya ga valid karena ga mungkin 0
    }
    
    uint32_t parent_bgd = inode_to_bgd(parent_inode);
    uint32_t parent_local = inode_to_local(parent_inode);
    struct BlockBuffer buf;
    uint32_t parent_itb = bgdtCache.table[parent_bgd].bg_inode_table;
    uint32_t parent_block_index = parent_itb + (parent_local / INODES_PER_TABLE);
    read_blocks(&buf, parent_block_index, 1);
    struct EXT2Inode *parent_node = &((struct EXT2Inode *)buf.buf)[parent_local % INODES_PER_TABLE];
    
    if ((parent_node->i_mode & EXT2_S_IFDIR) == 0) {
        return -1; // Parentnya bukan folder
    }
    
    bool found = false;
    uint32_t entry_inode = 0;
    uint32_t entry_file_type = 0;
    uint32_t blocks = parent_node->i_blocks;
    
    for (uint32_t bi = 0; bi < blocks && !found; bi++) {

        uint32_t block_id = parent_node->i_block[bi];
        if (block_id == 0) break;

        struct BlockBuffer dirBuf;
        read_blocks(&dirBuf, block_id, 1);
        uint32_t offset = 0;

        while (offset < BLOCK_SIZE) {
            struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)((uint8_t *)dirBuf.buf + offset);
            if (entry->inode != 0) {
                uint16_t name_len = entry->name_len;
                if (name_len == request.name_len &&
                    memcmp(get_entry_name(entry), request.name, name_len) == 0) {
                    entry_inode = entry->inode;
                    entry_file_type = entry->file_type;
                    found = true;
                    break;
                }
            }
            if (entry->rec_len == 0) break;
            offset += entry->rec_len;
        }
    }
    
    if (!found) {
        return 2; // File nya ga nemu di folder parent 
    }
    
    if (entry_file_type != EXT2_FT_REG_FILE) {
        return 1; // Bukan File rek
    }
    
    uint32_t file_inode = entry_inode;
    uint32_t file_bgd = inode_to_bgd(file_inode);
    uint32_t file_local = inode_to_local(file_inode);
    uint32_t file_itb = bgdtCache.table[file_bgd].bg_inode_table;
    uint32_t file_block_index = file_itb + (file_local / INODES_PER_TABLE);
    
    read_blocks(&buf, file_block_index, 1);
    struct EXT2Inode *file_node = &((struct EXT2Inode *)buf.buf)[file_local % INODES_PER_TABLE];
    uint32_t file_size = file_node->i_size;
    
    if (request.buffer_size < file_size) {
        return -1; // Buffer size too small for file content
    }
    
    uint32_t to_read = file_size;
    uint32_t copied = 0;
    
    // Copy direct blocks
    for (uint32_t i = 0; i < 12 && to_read > 0; i++) {
        uint32_t block_id = file_node->i_block[i];
        if (block_id == 0) break;
        struct BlockBuffer dataBuf;
        read_blocks(&dataBuf, block_id, 1);
        uint32_t copy_len = (to_read > BLOCK_SIZE) ? BLOCK_SIZE : to_read;
        memcpy((uint8_t *)request.buf + copied, dataBuf.buf, copy_len);
        copied += copy_len;
        to_read -= copy_len;
    }
    
    // Handle single indirect blocks
    if (to_read > 0 && file_node->i_block[12] != 0) {
        struct BlockBuffer indBuf;
        read_blocks(&indBuf, file_node->i_block[12], 1);
        uint32_t *ind_ptrs = (uint32_t *)indBuf.buf;
        for (uint32_t j = 0; j < BLOCK_SIZE/4 && to_read > 0; j++) {
            uint32_t block_id = ind_ptrs[j];
            if (block_id == 0) break;
            struct BlockBuffer dataBuf;
            read_blocks(&dataBuf, block_id, 1);
            uint32_t copy_len = (to_read > BLOCK_SIZE) ? BLOCK_SIZE : to_read;
            memcpy((uint8_t *)request.buf + copied, dataBuf.buf, copy_len);
            copied += copy_len;
            to_read -= copy_len;
        }
    }
    
    // Handle double indirect blocks
    if (to_read > 0 && file_node->i_block[13] != 0) {

        struct BlockBuffer dblBuf;
        read_blocks(&dblBuf, file_node->i_block[13], 1);
        uint32_t *dbl_ptrs = (uint32_t *)dblBuf.buf;

        for (uint32_t k = 0; k < BLOCK_SIZE/4 && to_read > 0; k++) {

            uint32_t ind_block = dbl_ptrs[k];
            if (ind_block == 0) break;

            struct BlockBuffer indBuf;
            read_blocks(&indBuf, ind_block, 1);
            uint32_t *ind_ptrs = (uint32_t *)indBuf.buf;

            for (uint32_t l = 0; l < BLOCK_SIZE/4 && to_read > 0; l++) {

                uint32_t block_id = ind_ptrs[l];
                if (block_id == 0) break;

                struct BlockBuffer dataBuf;
                read_blocks(&dataBuf, block_id, 1);
                uint32_t copy_len = (to_read > BLOCK_SIZE) ? BLOCK_SIZE : to_read;
                memcpy((uint8_t *)request.buf + copied, dataBuf.buf, copy_len);
                copied += copy_len;
                to_read -= copy_len;
            }
        }
    }
    
    return 0; // Operation successful
}

int8_t write(struct EXT2DriverRequest *request) {

    if (!request) return -1;
    uint32_t parent_inode = request->parent_inode;
    if (parent_inode == 0) return 2;

    // Load parent inode
    uint32_t parent_bgd = inode_to_bgd(parent_inode);
    uint32_t parent_local = inode_to_local(parent_inode);
    uint32_t itb = bgdtCache.table[parent_bgd].bg_inode_table;
    uint32_t block_idx = itb + (parent_local / INODES_PER_TABLE);

    struct BlockBuffer buf;
    read_blocks(&buf, block_idx, 1);
    struct EXT2Inode *parent = &((struct EXT2Inode *)buf.buf)[parent_local % INODES_PER_TABLE];
    if ((parent->i_mode & EXT2_S_IFDIR) == 0) return 2;

    uint16_t need_rec = get_entry_record_len(request->name_len);
    bool is_dir = (request->buffer_size == 0 && request->is_directory);

    // Search existing directory blocks for space
    for (uint32_t bi = 0; bi < parent->i_blocks; bi++) {

        uint32_t blk = parent->i_block[bi];
        if (!blk) continue;

        struct BlockBuffer dbuf;
        read_blocks(&dbuf, blk, 1);
        uint32_t off = 0;

        while (off < BLOCK_SIZE) {
            struct EXT2DirectoryEntry *e = (struct EXT2DirectoryEntry *)(dbuf.buf + off);
            uint16_t actual = get_entry_record_len(e->name_len);

            if (e->inode != 0) {
                // Duplicates
                if (e->name_len == request->name_len &&
                    memcmp(get_entry_name(e), request->name, e->name_len) == 0) {
                    return 1;
                }
            }

            // Space after this entry?
            if (e->inode != 0 && e->rec_len > actual) {

                uint16_t hole = e->rec_len - actual;
                if (hole >= need_rec) {

                    // Allocate new inode
                    uint32_t new_ino = allocate_node();
                    if (!new_ino) return -1;

                    // Init new inode
                    uint32_t new_bgd = inode_to_bgd(new_ino);
                    uint32_t new_loc = inode_to_local(new_ino);
                    uint32_t new_itb = bgdtCache.table[new_bgd].bg_inode_table;

                    struct BlockBuffer inbuf;
                    read_blocks(&inbuf, new_itb + (new_loc / INODES_PER_TABLE), 1);
                    struct EXT2Inode *ni = &((struct EXT2Inode *)inbuf.buf)[new_loc % INODES_PER_TABLE];

                    if (is_dir) {
                        init_directory_table(ni, new_ino, parent_inode);
                        bgdtCache.table[new_bgd].bg_used_dirs_count++;
                    } 
                    else {
                        ni->i_mode = EXT2_S_IFREG;
                        ni->i_size = request->buffer_size;
                        ni->i_blocks = (request->buffer_size + BLOCK_SIZE -1)/BLOCK_SIZE;
                        allocate_node_blocks(request->buf, ni, new_bgd);
                    }
                    sync_node(ni, new_ino);

                    // Insert entry
                    uint32_t new_off = off + actual;
                    struct EXT2DirectoryEntry *ne = (struct EXT2DirectoryEntry *)(dbuf.buf + new_off);
                    ne->inode = new_ino;
                    ne->name_len = request->name_len;
                    ne->file_type = is_dir ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
                    ne->rec_len = hole;
                    memcpy(get_entry_name(ne), request->name, request->name_len);

                    // Shrink existing entry rec_len
                    e->rec_len = actual;
                    write_blocks(&dbuf, blk, 1);
                    sync_metadata();
                    return 0;
                }
            }
            if (e->rec_len == 0) break;
            off += e->rec_len;
        }
    }

    // No space found: allocate new block, expand parent directory
    uint32_t newblock = 0;
    for (uint32_t bg = parent_bgd; bg < GROUPS_COUNT; bg++) {

        if (bgdtCache.table[bg].bg_free_blocks_count) {
            struct BlockBuffer bmp;
            read_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);

            for (uint32_t b=0; b<BLOCK_SIZE; b++) {
                if (bmp.buf[b] != 0xFF) {
                    for (uint8_t bit=0; bit<8; bit++) {
                        uint32_t idx = b*8 + bit;
                        if (idx>=BLOCKS_PER_GROUP) break;
                        if (!(bmp.buf[b]&(1<<bit))) {
                            bmp.buf[b] |= (1<<bit);
                            write_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                            bgdtCache.table[bg].bg_free_blocks_count--;
                            superBlock.s_free_blocks_count--;
                            newblock = bg * BLOCKS_PER_GROUP + idx;
                            break;
                        }
                    }
                }
                if (newblock) break;
            }
        }
        if (newblock) break;
    }
    if (!newblock) return -1;

    // Append to parent
    parent->i_block[parent->i_blocks++] = newblock;
    parent->i_size += BLOCK_SIZE;

    // Prepare block buffer
    struct BlockBuffer dbuf;
    memset(&dbuf, 0, BLOCK_SIZE);

    // Single entry covers whole block
    struct EXT2DirectoryEntry *e0 = (struct EXT2DirectoryEntry *)dbuf.buf;
    e0->inode = allocate_node();
    if (!e0->inode) return -1;

    uint32_t new_ino = e0->inode;
    uint32_t bgc = inode_to_bgd(new_ino);
    uint32_t loc = inode_to_local(new_ino);

    struct BlockBuffer inb;
    read_blocks(&inb, bgdtCache.table[bgc].bg_inode_table + (loc/INODES_PER_TABLE), 1);
    struct EXT2Inode *ni = &((struct EXT2Inode *)inb.buf)[loc%INODES_PER_TABLE];

    if (is_dir) {
        init_directory_table(ni, new_ino, parent_inode);
        bgdtCache.table[bgc].bg_used_dirs_count++;
    } 
    else {
        ni->i_mode = EXT2_S_IFREG;
        ni->i_size = request->buffer_size;
        ni->i_blocks = (request->buffer_size + BLOCK_SIZE -1)/BLOCK_SIZE;
        allocate_node_blocks(request->buf, ni, bgc);
    }
    sync_node(ni, new_ino);

    e0->name_len = request->name_len;
    e0->file_type = is_dir ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
    e0->rec_len = BLOCK_SIZE;
    memcpy(get_entry_name(e0), request->name, request->name_len);

    write_blocks(&dbuf, newblock, 1);

    // Write updated parent inode and metadata
    write_blocks(&buf, block_idx, 1);
    sync_metadata();
    return 0;
}


int8_t delete(struct EXT2DriverRequest request) {

    uint32_t parent_inode = request.parent_inode;
    if (parent_inode == 0) {
        return -1; // Parent inode 0 alias ga valid
    }

    uint32_t parent_bgd = inode_to_bgd(parent_inode);
    uint32_t parent_local = inode_to_local(parent_inode);
    uint32_t parent_itb = bgdtCache.table[parent_bgd].bg_inode_table;
    uint32_t parent_block_index = parent_itb + (parent_local / INODES_PER_TABLE);
    
    struct BlockBuffer buf;
    read_blocks(&buf, parent_block_index, 1);
    struct EXT2Inode *parent_node = &((struct EXT2Inode *)buf.buf)[parent_local % INODES_PER_TABLE];

    if ((parent_node->i_mode & EXT2_S_IFDIR) == 0) {
        return -1; // Parent nya bukan folder (jaga jaga) 
    }

    bool found = false;
    uint32_t entry_block = 0;
    uint32_t entry_offset = 0;
    uint32_t prev_offset = 0;
    struct EXT2DirectoryEntry *prev_entry = NULL;
    
    for (uint32_t bi = 0; bi < parent_node->i_blocks; bi++) {

        uint32_t block_id = parent_node->i_block[bi];
        if (block_id == 0) continue;

        struct BlockBuffer dirBuf;
        read_blocks(&dirBuf, block_id, 1);
        uint32_t offset = 0;
        prev_offset = 0;
        prev_entry = NULL;
        
        while (offset < BLOCK_SIZE) {
            struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)((uint8_t *)dirBuf.buf + offset);
            if (entry->rec_len == 0) break;
            
            if (entry->inode != 0) {
                uint16_t name_len = entry->name_len;
                if (name_len == request.name_len &&
                    memcmp(get_entry_name(entry), request.name, name_len) == 0) {
                    found = true;
                    entry_block = block_id;
                    entry_offset = offset;
                    break;
                }
            }
            
            prev_entry = entry;
            prev_offset = offset;
            offset += entry->rec_len;
        }
        if (found) break;
    }
    
    if (!found) {
        return 1; // Ga nemu foldernya di parent folder
    }
    
    struct BlockBuffer targetBuf;
    read_blocks(&targetBuf, entry_block, 1);
    struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)((uint8_t *)targetBuf.buf + entry_offset);
    
    if (entry->inode == 0) {
        return 1; // Entry not found (already deleted)
    }
    
    uint32_t target_inode = entry->inode;
    bool is_dir = request.is_directory;
    
    if (is_dir) {
        if (entry->file_type != EXT2_FT_DIR) {
            return 1; // Bukan folder, kalalu ada mis match entry dengan flag dir
        }
        if (!is_directory_empty(target_inode)) {
            return 2; // Folder not empty
        }
    } 
    else {
        if (entry->file_type != EXT2_FT_REG_FILE) {
            return 1; // Bukan file, kalau ada mis match entry dengan flag file
        }
    }
    
    uint32_t target_bgd = inode_to_bgd(target_inode);
    uint32_t target_local = inode_to_local(target_inode);
    uint32_t target_itb = bgdtCache.table[target_bgd].bg_inode_table;
    uint32_t target_block_index = target_itb + (target_local / INODES_PER_TABLE);
    
    struct BlockBuffer inodeBuf;
    read_blocks(&inodeBuf, target_block_index, 1);

    deallocate_node(target_inode);
    
    if (prev_entry) {
        struct EXT2DirectoryEntry *prev = (struct EXT2DirectoryEntry *)((uint8_t *)targetBuf.buf + prev_offset);
        // Hapus entry jadi nya kosong alias 0 semua
        prev->rec_len += entry->rec_len;
        entry->inode = 0;
        entry->rec_len = 0;
        for (uint8_t i = 0; i < entry->name_len; i++) {
            get_entry_name(entry)[i] = 0;
        }
        entry->file_type = 0;
        entry->name_len = 0;
    } 
    else {
        // Kalau entry pertama di block, jadiin entry kosong tapi simpan rec_len
        entry->inode = 0;
    }
    
    // Write changes back to disk
    write_blocks(&targetBuf, entry_block, 1);
    sync_metadata();
    
    return 0;
}

uint32_t allocate_node(void) {
    for (uint32_t i = 0; i < GROUPS_COUNT; i++) {

        if (bgdtCache.table[i].bg_free_inodes_count > 0) {
            struct BlockBuffer bmp;
            read_blocks(&bmp, bgdtCache.table[i].bg_inode_bitmap, 1);

            for (uint32_t byte = 0; byte < BLOCK_SIZE; byte++) {
                if (bmp.buf[byte] != 0xFF) {
                    for (uint8_t bit = 0; bit < 8; bit++) {
                        uint32_t idx = byte * 8 + bit;
                        if (idx >= INODES_PER_GROUP) break;
                        if ((bmp.buf[byte] & (1 << bit)) == 0) {
                            bmp.buf[byte] |= (1 << bit);
                            write_blocks(&bmp, bgdtCache.table[i].bg_inode_bitmap, 1);
                            bgdtCache.table[i].bg_free_inodes_count--;
                            superBlock.s_free_inodes_count--;
                            sync_metadata();
                            return i * INODES_PER_GROUP + idx + 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

void deallocate_node(uint32_t inode) {

    if (inode == 0) return;
    uint32_t bg = inode_to_bgd(inode);
    uint32_t local = inode_to_local(inode);
    uint32_t itb = bgdtCache.table[bg].bg_inode_table;
    uint32_t inode_block_index = itb + (local / INODES_PER_TABLE);

    struct BlockBuffer buf;
    read_blocks(&buf, inode_block_index, 1);

    struct EXT2Inode *node = &((struct EXT2Inode *)buf.buf)[local % INODES_PER_TABLE];
    if ((node->i_mode & EXT2_S_IFDIR) != 0) {
        if (bgdtCache.table[bg].bg_used_dirs_count > 0) {
            bgdtCache.table[bg].bg_used_dirs_count--;
        }
    }

    uint32_t remain = node->i_blocks;

    for (uint32_t i = 0; i < 12 && remain > 0; i++) {

        uint32_t block_id = node->i_block[i];

        if (block_id != 0) {
            uint32_t grp = block_id / BLOCKS_PER_GROUP;
            uint32_t off = block_id % BLOCKS_PER_GROUP;

            struct BlockBuffer bmp;
            read_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
            bmp.buf[off/8] &= ~(1 << (off % 8));
            write_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
            bgdtCache.table[grp].bg_free_blocks_count++;
            superBlock.s_free_blocks_count++;
        }
        remain--;
    }

    if (remain > 0 && node->i_block[12] != 0) {

        struct BlockBuffer indBuf;
        read_blocks(&indBuf, node->i_block[12], 1);
        uint32_t *ind_ptrs = (uint32_t *)indBuf.buf;
        uint32_t count = (remain < BLOCK_SIZE/4) ? remain : (BLOCK_SIZE/4);

        for (uint32_t j = 0; j < count; j++) {
            uint32_t block_id = ind_ptrs[j];
            if (block_id != 0) {
                uint32_t grp = block_id / BLOCKS_PER_GROUP;
                uint32_t off = block_id % BLOCKS_PER_GROUP;
                struct BlockBuffer bmp;
                read_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
                bmp.buf[off/8] &= ~(1 << (off % 8));
                write_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
                bgdtCache.table[grp].bg_free_blocks_count++;
                superBlock.s_free_blocks_count++;
            }
        }

        uint32_t block_id = node->i_block[12];
        uint32_t grp = block_id / BLOCKS_PER_GROUP;
        uint32_t off = block_id % BLOCKS_PER_GROUP;

        struct BlockBuffer bmp;
        read_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
        bmp.buf[off/8] &= ~(1 << (off % 8));
        write_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
        bgdtCache.table[grp].bg_free_blocks_count++;
        superBlock.s_free_blocks_count++;

        remain = (remain > count) ? (remain - count) : 0;
    }

    if (remain > 0 && node->i_block[13] != 0) {
        struct BlockBuffer dblBuf;
        read_blocks(&dblBuf, node->i_block[13], 1);
        uint32_t *dbl_ptrs = (uint32_t *)dblBuf.buf;
        uint32_t total = remain;

        for (uint32_t k = 0; k < BLOCK_SIZE/4 && total > 0; k++) {

            uint32_t ind_block = dbl_ptrs[k];
            if (ind_block != 0) {
                struct BlockBuffer indBuf;
                read_blocks(&indBuf, ind_block, 1);
                uint32_t *ind_ptrs = (uint32_t *)indBuf.buf;

                uint32_t cnt = (total < BLOCK_SIZE/4) ? total : (BLOCK_SIZE/4);
                for (uint32_t l = 0; l < cnt; l++) {
                    uint32_t block_id = ind_ptrs[l];
                    if (block_id != 0) {
                        uint32_t grp = block_id / BLOCKS_PER_GROUP;
                        uint32_t off = block_id % BLOCKS_PER_GROUP;

                        struct BlockBuffer bmp;
                        read_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
                        bmp.buf[off/8] &= ~(1 << (off % 8));
                        write_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
                        bgdtCache.table[grp].bg_free_blocks_count++;
                        superBlock.s_free_blocks_count++;
                    }
                }

                uint32_t grp = ind_block / BLOCKS_PER_GROUP;
                uint32_t off = ind_block % BLOCKS_PER_GROUP;
                struct BlockBuffer bmp;
                read_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
                bmp.buf[off/8] &= ~(1 << (off % 8));
                write_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
                bgdtCache.table[grp].bg_free_blocks_count++;
                superBlock.s_free_blocks_count++;

                total -= cnt;
            }
        }

        uint32_t block_id = node->i_block[13];
        uint32_t grp = block_id / BLOCKS_PER_GROUP;
        uint32_t off = block_id % BLOCKS_PER_GROUP;
        struct BlockBuffer bmp;
        read_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
        bmp.buf[off/8] &= ~(1 << (off % 8));
        write_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
        bgdtCache.table[grp].bg_free_blocks_count++;
        superBlock.s_free_blocks_count++;
    }

    memset(&((struct EXT2Inode *)buf.buf)[local % INODES_PER_TABLE], 0, sizeof(struct EXT2Inode));
    write_blocks(&buf, inode_block_index, 1);

    struct BlockBuffer bmp;
    read_blocks(&bmp, bgdtCache.table[bg].bg_inode_bitmap, 1);
    uint32_t idx = local;
    bmp.buf[idx/8] &= ~(1 << (idx % 8));
    write_blocks(&bmp, bgdtCache.table[bg].bg_inode_bitmap, 1);
    bgdtCache.table[bg].bg_free_inodes_count++;
    superBlock.s_free_inodes_count++;

    sync_metadata();
}

void deallocate_blocks(void *loc, uint32_t blocks) {
    uint32_t *addrs = (uint32_t *)loc;
    uint32_t to_free = blocks;
    for (uint32_t i = 0; i < 12 && to_free > 0; i++) {
        uint32_t block_id = addrs[i];
        if (block_id != 0) {
            uint32_t grp = block_id / BLOCKS_PER_GROUP;
            uint32_t off = block_id % BLOCKS_PER_GROUP;
            struct BlockBuffer bmp;
            read_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
            bmp.buf[off/8] &= ~(1 << (off % 8));
            write_blocks(&bmp, bgdtCache.table[grp].bg_block_bitmap, 1);
            bgdtCache.table[grp].bg_free_blocks_count++;
            superBlock.s_free_blocks_count++;
        }
        to_free--;
    }
    sync_metadata();
}

uint32_t deallocate_block(uint32_t *locations, 
                          uint32_t blocks, 
                          struct BlockBuffer *bitmap, 
                          uint32_t depth, 
                          uint32_t *last_bgd, 
                          bool bgd_loaded) {

    if (locations == NULL || blocks == 0) {
        return *last_bgd;
    }

    struct BlockBuffer buf;
    uint32_t ptr_per_block = BLOCK_SIZE / sizeof(uint32_t);
    
    if (depth == 0) {
        for (uint32_t i = 0; i < blocks; i++) {
            if (locations[i] == 0) continue;
            
            uint32_t block_id = locations[i];
            uint32_t grp = block_id / BLOCKS_PER_GROUP;
            uint32_t off = block_id % BLOCKS_PER_GROUP;
            
            if (!bitmap || grp != *last_bgd) {
                if (bitmap) {
                    write_blocks(bitmap, bgdtCache.table[*last_bgd].bg_block_bitmap, 1);
                }
                *last_bgd = grp;
                read_blocks(&buf, bgdtCache.table[grp].bg_block_bitmap, 1);
                bitmap = &buf;
            }
            
            bitmap->buf[off/8] &= ~(1 << (off % 8));
            
            bgdtCache.table[grp].bg_free_blocks_count++;
            superBlock.s_free_blocks_count++;
            
            locations[i] = 0;
        }
    } 
    else if (depth == 1) {
        for (uint32_t i = 0; i < blocks && locations[i] != 0; i++) {
            read_blocks(&buf, locations[i], 1);
            uint32_t *ind_ptrs = (uint32_t *)buf.buf;
            
            uint32_t count = (blocks > ptr_per_block) ? ptr_per_block : blocks;
            deallocate_block(ind_ptrs, count, bitmap, 0, last_bgd, bgd_loaded);
            
            uint32_t block_id = locations[i];
            uint32_t grp = block_id / BLOCKS_PER_GROUP;
            uint32_t off = block_id % BLOCKS_PER_GROUP;
            
            if (!bitmap || grp != *last_bgd) {
                if (bitmap) {
                    write_blocks(bitmap, bgdtCache.table[*last_bgd].bg_block_bitmap, 1);
                }
                *last_bgd = grp;
                read_blocks(&buf, bgdtCache.table[grp].bg_block_bitmap, 1);
                bitmap = &buf;
            }
            
            bitmap->buf[off/8] &= ~(1 << (off % 8));
            
            bgdtCache.table[grp].bg_free_blocks_count++;
            superBlock.s_free_blocks_count++;
            
            locations[i] = 0;
        }
    } 
    else if (depth == 2) {
        for (uint32_t i = 0; i < blocks && locations[i] != 0; i++) {
            read_blocks(&buf, locations[i], 1);
            uint32_t *dbl_ptrs = (uint32_t *)buf.buf;
            
            for (uint32_t j = 0; j < ptr_per_block && dbl_ptrs[j] != 0; j++) {
                uint32_t remain = (blocks > (j+1)*ptr_per_block) ? ptr_per_block : (blocks - j*ptr_per_block);
                if (remain == 0) break;
                
                deallocate_block(&dbl_ptrs[j], 1, bitmap, 1, last_bgd, bgd_loaded);
            }
            
            uint32_t block_id = locations[i];
            uint32_t grp = block_id / BLOCKS_PER_GROUP;
            uint32_t off = block_id % BLOCKS_PER_GROUP;
            
            if (!bitmap || grp != *last_bgd) {
                if (bitmap) {
                    write_blocks(bitmap, bgdtCache.table[*last_bgd].bg_block_bitmap, 1);
                }
                *last_bgd = grp;
                read_blocks(&buf, bgdtCache.table[grp].bg_block_bitmap, 1);
                bitmap = &buf;
            }
            
            bitmap->buf[off/8] &= ~(1 << (off % 8));
            
            bgdtCache.table[grp].bg_free_blocks_count++;
            superBlock.s_free_blocks_count++;
            
            locations[i] = 0;
        }
    }
    
    if (bitmap) {
        write_blocks(bitmap, bgdtCache.table[*last_bgd].bg_block_bitmap, 1);
    }
    
    return *last_bgd;
}

void allocate_node_blocks(void *ptr, struct EXT2Inode *node, uint32_t prefered_bgd) {

    uint8_t *data = (uint8_t *)ptr;
    uint32_t blocks_needed = node->i_blocks;
    uint32_t allocated = 0;

    for (uint32_t i = 0; i < 12 && allocated < blocks_needed; i++) {
        bool found = false;

        for (uint32_t bg = prefered_bgd; bg < GROUPS_COUNT && !found; bg++) {
            if (bgdtCache.table[bg].bg_free_blocks_count > 0) {
                struct BlockBuffer bmp;
                read_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);

                for (uint32_t byte = 0; byte < BLOCK_SIZE; byte++) {
                    if (bmp.buf[byte] != 0xFF) {

                        for (uint8_t bit = 0; bit < 8; bit++) {
                            uint32_t bit_idx = byte * 8 + bit;
                            if (bit_idx >= BLOCKS_PER_GROUP) break;

                            if ((bmp.buf[byte] & (1 << bit)) == 0) {

                                bmp.buf[byte] |= (1 << bit);
                                write_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                                bgdtCache.table[bg].bg_free_blocks_count--;
                                superBlock.s_free_blocks_count--;
                                uint32_t block_num = bg * BLOCKS_PER_GROUP + bit_idx;
                                node->i_block[i] = block_num;

                                struct BlockBuffer dataBuf;
                                memset(&dataBuf, 0, BLOCK_SIZE);
                                uint32_t to_copy = (allocated == blocks_needed - 1) ? (node->i_size % BLOCK_SIZE) : BLOCK_SIZE;
                                if (to_copy == 0) to_copy = BLOCK_SIZE;
                                memcpy(dataBuf.buf, data + allocated * BLOCK_SIZE, to_copy);
                                write_blocks(&dataBuf, block_num, 1);
                                allocated++;
                                found = true;

                                break;
                            }
                        }
                    }
                    if (found) break;
                }
            }
        }
        if (!found) break;
    }

    if (allocated < blocks_needed) {
        uint32_t pointer_block = 0;
        bool found = false;

        for (uint32_t bg = prefered_bgd; bg < GROUPS_COUNT && !found; bg++) {
            if (bgdtCache.table[bg].bg_free_blocks_count > 0) {
                struct BlockBuffer bmp;
                read_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);

                for (uint32_t byte = 0; byte < BLOCK_SIZE; byte++) {
                    if (bmp.buf[byte] != 0xFF) {

                        for (uint8_t bit = 0; bit < 8; bit++) {
                            uint32_t bit_idx = byte * 8 + bit;
                            if (bit_idx >= BLOCKS_PER_GROUP) break;

                            if ((bmp.buf[byte] & (1 << bit)) == 0) {
                                bmp.buf[byte] |= (1 << bit);
                                write_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                                bgdtCache.table[bg].bg_free_blocks_count--;
                                superBlock.s_free_blocks_count--;
                                pointer_block = bg * BLOCKS_PER_GROUP + bit_idx;
                                node->i_block[12] = pointer_block;
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found) break;
                }
            }
        }

        if (pointer_block != 0) {
            struct BlockBuffer ptrBuf;
            memset(&ptrBuf, 0, BLOCK_SIZE);
            uint32_t ptr_idx = 0;

            while (allocated < blocks_needed && ptr_idx < BLOCK_SIZE/4) {
                bool found2 = false;

                for (uint32_t bg = prefered_bgd; bg < GROUPS_COUNT && !found2; bg++) {
                    if (bgdtCache.table[bg].bg_free_blocks_count > 0) {
                        struct BlockBuffer bmp;
                        read_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);

                        for (uint32_t byte = 0; byte < BLOCK_SIZE; byte++) {
                            if (bmp.buf[byte] != 0xFF) {

                                for (uint8_t bit = 0; bit < 8; bit++) {
                                    uint32_t bit_idx = byte * 8 + bit;
                                    if (bit_idx >= BLOCKS_PER_GROUP) break;

                                    if ((bmp.buf[byte] & (1 << bit)) == 0) {
                                        bmp.buf[byte] |= (1 << bit);
                                        write_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                                        bgdtCache.table[bg].bg_free_blocks_count--;
                                        superBlock.s_free_blocks_count--;

                                        uint32_t block_num = bg * BLOCKS_PER_GROUP + bit_idx;
                                        ptrBuf.buf[ptr_idx*4] = (uint8_t)(block_num & 0xFF);
                                        ptrBuf.buf[ptr_idx*4 + 1] = (uint8_t)((block_num >> 8) & 0xFF);
                                        ptrBuf.buf[ptr_idx*4 + 2] = (uint8_t)((block_num >> 16) & 0xFF);
                                        ptrBuf.buf[ptr_idx*4 + 3] = (uint8_t)((block_num >> 24) & 0xFF);

                                        struct BlockBuffer dataBuf;
                                        memset(&dataBuf, 0, BLOCK_SIZE);
                                        uint32_t to_copy = (allocated == blocks_needed - 1) ? (node->i_size % BLOCK_SIZE) : BLOCK_SIZE;
                                        if (to_copy == 0) to_copy = BLOCK_SIZE;
                                        memcpy(dataBuf.buf, data + allocated * BLOCK_SIZE, to_copy);
                                        write_blocks(&dataBuf, block_num, 1);
                                        allocated++;
                                        ptr_idx++;
                                        found2 = true;

                                        break;
                                    }
                                }
                            }
                            if (found2) break;
                        }
                    }
                }
                if (!found2) break;
            }
            write_blocks(&ptrBuf, pointer_block, 1);
        }
    }

    if (allocated < blocks_needed) {
        uint32_t dbl_block = 0;
        bool found = false;
        for (uint32_t bg = prefered_bgd; bg < GROUPS_COUNT && !found; bg++) {
            if (bgdtCache.table[bg].bg_free_blocks_count > 0) {
                struct BlockBuffer bmp;
                read_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);

                for (uint32_t byte = 0; byte < BLOCK_SIZE; byte++) {
                    if (bmp.buf[byte] != 0xFF) {
                        for (uint8_t bit = 0; bit < 8; bit++) {
                            uint32_t bit_idx = byte * 8 + bit;
                            if (bit_idx >= BLOCKS_PER_GROUP) break;

                            if ((bmp.buf[byte] & (1 << bit)) == 0) {
                                bmp.buf[byte] |= (1 << bit);
                                write_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                                bgdtCache.table[bg].bg_free_blocks_count--;
                                superBlock.s_free_blocks_count--;
                                dbl_block = bg * BLOCKS_PER_GROUP + bit_idx;
                                node->i_block[13] = dbl_block;
                                found = true;

                                break;
                            }
                        }
                    }
                    if (found) break;
                }
            }
        }

        if (dbl_block != 0) {
            struct BlockBuffer dblBuf;
            memset(&dblBuf, 0, BLOCK_SIZE);
            uint32_t dbl_idx = 0;

            while (allocated < blocks_needed && dbl_idx < BLOCK_SIZE/4) {
                uint32_t ind_block = 0;
                bool found2 = false;

                for (uint32_t bg = prefered_bgd; bg < GROUPS_COUNT && !found2; bg++) {
                    if (bgdtCache.table[bg].bg_free_blocks_count > 0) {
                        struct BlockBuffer bmp;
                        read_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);

                        for (uint32_t byte = 0; byte < BLOCK_SIZE; byte++) {
                            if (bmp.buf[byte] != 0xFF) {

                                for (uint8_t bit = 0; bit < 8; bit++) {
                                    uint32_t bit_idx = byte * 8 + bit;
                                    if (bit_idx >= BLOCKS_PER_GROUP) break;

                                    if ((bmp.buf[byte] & (1 << bit)) == 0) {
                                        bmp.buf[byte] |= (1 << bit);
                                        write_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                                        bgdtCache.table[bg].bg_free_blocks_count--;
                                        superBlock.s_free_blocks_count--;

                                        ind_block = bg * BLOCKS_PER_GROUP + bit_idx;
                                        dblBuf.buf[dbl_idx*4] = (uint8_t)(ind_block & 0xFF);
                                        dblBuf.buf[dbl_idx*4 + 1] = (uint8_t)((ind_block >> 8) & 0xFF);
                                        dblBuf.buf[dbl_idx*4 + 2] = (uint8_t)((ind_block >> 16) & 0xFF);
                                        dblBuf.buf[dbl_idx*4 + 3] = (uint8_t)((ind_block >> 24) & 0xFF);
                                        found2 = true;

                                        break;
                                    }
                                }
                            }
                            if (found2) break;
                        }
                    }
                }

                if (!found2) break;
                struct BlockBuffer ptrBuf;
                memset(&ptrBuf, 0, BLOCK_SIZE);
                uint32_t ptr_idx = 0;

                while (allocated < blocks_needed && ptr_idx < BLOCK_SIZE/4) {
                    bool found3 = false;

                    for (uint32_t bg = prefered_bgd; bg < GROUPS_COUNT && !found3; bg++) {
                        if (bgdtCache.table[bg].bg_free_blocks_count > 0) {
                            struct BlockBuffer bmp;
                            read_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);

                            for (uint32_t byte = 0; byte < BLOCK_SIZE; byte++) {
                                if (bmp.buf[byte] != 0xFF) {

                                    for (uint8_t bit = 0; bit < 8; bit++) {
                                        uint32_t bit_idx = byte * 8 + bit;
                                        if (bit_idx >= BLOCKS_PER_GROUP) break;

                                        if ((bmp.buf[byte] & (1 << bit)) == 0) {
                                            bmp.buf[byte] |= (1 << bit);
                                            write_blocks(&bmp, bgdtCache.table[bg].bg_block_bitmap, 1);
                                            bgdtCache.table[bg].bg_free_blocks_count--;
                                            superBlock.s_free_blocks_count--;

                                            uint32_t block_num = bg * BLOCKS_PER_GROUP + bit_idx;
                                            ptrBuf.buf[ptr_idx*4] = (uint8_t)(block_num & 0xFF);
                                            ptrBuf.buf[ptr_idx*4 + 1] = (uint8_t)((block_num >> 8) & 0xFF);
                                            ptrBuf.buf[ptr_idx*4 + 2] = (uint8_t)((block_num >> 16) & 0xFF);
                                            ptrBuf.buf[ptr_idx*4 + 3] = (uint8_t)((block_num >> 24) & 0xFF);

                                            struct BlockBuffer dataBuf;
                                            memset(&dataBuf, 0, BLOCK_SIZE);
                                            uint32_t to_copy = (allocated == blocks_needed - 1) ? (node->i_size % BLOCK_SIZE) : BLOCK_SIZE;
                                            if (to_copy == 0) to_copy = BLOCK_SIZE;
                                            memcpy(dataBuf.buf, data + allocated * BLOCK_SIZE, to_copy);
                                            write_blocks(&dataBuf, block_num, 1);
                                            allocated++;
                                            ptr_idx++;
                                            found3 = true;
                                            
                                            break;
                                        }
                                    }
                                }
                                if (found3) break;
                            }
                        }
                    }
                    if (!found3) break;
                }
                write_blocks(&ptrBuf, ind_block, 1);
                dbl_idx++;
            }
            write_blocks(&dblBuf, dbl_block, 1);
        }
    }
    sync_metadata();
}

void sync_node(struct EXT2Inode *node, uint32_t inode) {
    if (inode == 0 || node == NULL) return;
    uint32_t bg = inode_to_bgd(inode);
    uint32_t local = inode_to_local(inode);
    uint32_t itb = bgdtCache.table[bg].bg_inode_table;
    uint32_t inode_block_index = itb + (local / INODES_PER_TABLE);
    struct BlockBuffer buf;
    read_blocks(&buf, inode_block_index, 1);
    ((struct EXT2Inode *)buf.buf)[local % INODES_PER_TABLE] = *node;
    write_blocks(&buf, inode_block_index, 1);
}

// Debug Purposes
uint32_t get_inode_for_path(const char *path) {
    if (!path || path[0] != '/') return 0;
    if (path[1] == '\0') return 1; // root inode is 1

    char temp[64];
    uint32_t current_inode = 1;
    const char *start = path + 1;
    const char *end = start;

    while (*start) {
        while (*end && *end != '/') end++;

        int len = end - start;
        if (len <= 0 || len > 64) return 0;
        memcpy(temp, start, len);
        temp[len] = 0;

        struct EXT2DriverRequest req = {
            .buf = NULL,
            .name = temp,
            .name_len = len,
            .parent_inode = current_inode,
            .buffer_size = 0,
            .is_directory = true
        };

        struct BlockBuffer buf;
        if (read_directory(&req) != 0) return 0;

        struct EXT2Inode *inode;
        uint32_t bgd = inode_to_bgd(current_inode);
        uint32_t local = inode_to_local(current_inode);
        uint32_t itb = bgdtCache.table[bgd].bg_inode_table;
        read_blocks(&buf, itb + (local / INODES_PER_TABLE), 1);
        inode = &((struct EXT2Inode *)buf.buf)[local % INODES_PER_TABLE];

        bool found = false;
        for (uint32_t i = 0; i < inode->i_blocks && !found; i++) {
            uint32_t block = inode->i_block[i];
            if (block == 0) continue;
            struct BlockBuffer dirBuf;
            read_blocks(&dirBuf, block, 1);
            uint32_t offset = 0;
            while (offset < BLOCK_SIZE) {
                struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(dirBuf.buf + offset);
                if (entry->inode != 0 &&
                    entry->name_len == len &&
                    memcmp(get_entry_name(entry), temp, len) == 0) {
                    current_inode = entry->inode;
                    found = true;
                    break;
                }
                if (entry->rec_len == 0) break;
                offset += entry->rec_len;
            }
        }

        if (!found) return 0;
        if (*end == '/') end++;
        start = end;
    }

    return current_inode;
}