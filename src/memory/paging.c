#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "memory/paging.h"

__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        [0] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
        [0x300] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
    }
};

static struct PageManagerState page_manager_state = {
    .page_frame_map = {
        [0]                            = true,
        [1 ... PAGE_FRAME_MAX_COUNT-1] = false
    },
    .free_page_frame_count = PAGE_FRAME_MAX_COUNT,
    .physical_addr_pointr = (uint8_t *)0 + PAGE_FRAME_SIZE
};

void update_page_directory_entry(
    struct PageDirectory *page_dir,
    void *physical_addr, 
    void *virtual_addr, 
    struct PageDirectoryEntryFlag flag
) {
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;
    page_dir->table[page_index].flag          = flag;
    page_dir->table[page_index].lower_address = ((uint32_t) physical_addr >> 22) & 0x3FF;
    flush_single_tlb(virtual_addr);
}

void flush_single_tlb(void *virtual_addr) {
    asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr): "memory");
}



/* --- Memory Management --- */
bool paging_allocate_check(uint32_t amount) {
    // Check whether requested amount is available

    uint32_t frameAmount = (amount + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;

    return page_manager_state.free_page_frame_count <= frameAmount;
}


bool paging_allocate_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    /*
     * TODO: Find free physical frame and map virtual frame into it
     * - Find free physical frame in page_manager_state.page_frame_map[] using any strategies
     * - Mark page_manager_state.page_frame_map[]
     * - Update page directory with user flags:
     *     > present bit    true
     *     > write bit      true
     *     > user bit       true
     *     > pagesize 4 mb  true
     */ 

    // Searching for the first unallocated page frame match

    bool found = false;
    uint32_t i = 0;
    while (i < PAGE_FRAME_MAX_COUNT && !found){
        if (page_manager_state.page_frame_map[i]) {
            // Already filled
            i++;
        } else {
            // Still available
            found = true;
        }
    }
    if (found){
        struct PageDirectoryEntryFlag newFlag = {
            .present_bit = true,
            .write_bit = true,
            .user_supervisor = true,
            .use_pagesize_4_mb = true,
        };
        
        uint32_t physical_addr = (uint32_t) page_manager_state.physical_addr_pointr + (i+1) * PAGE_FRAME_SIZE;
        update_page_directory_entry(
            page_dir, 
            (void *) physical_addr,
            virtual_addr,
            newFlag
        );

        // Set allocated memory to zero
        for (uint32_t i = 0; i < PAGE_FRAME_SIZE; i++) {
            *((uint8_t *)(virtual_addr + i)) = 0;
        }

        // Update page_manager_state
        page_manager_state.free_page_frame_count--;
        page_manager_state.page_frame_map[i] = true;
        return true;
    } else {
        return false;
    }
}

bool paging_free_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    /* 
     * TODO: Deallocate a physical frame from respective virtual address
     * - Use the page_dir.table values to check mapped physical frame
     * - Remove the entry by setting it into 0
     */
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;
    if (page_dir->table[page_index].flag.present_bit){
        uint32_t i = page_dir->table[page_index].lower_address >> 22;
        i--;
        page_manager_state.page_frame_map[i] = false;
        struct PageDirectoryEntryFlag nonFlag = {
            .present_bit = false,
            .write_bit = false,
            .user_supervisor = false,
            .use_pagesize_4_mb = false,
        };
        page_dir->table[page_index].flag = nonFlag;
        return true;
    } else {
        return false;
    }
}