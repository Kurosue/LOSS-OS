#include "process/process.h"
#include "memory/paging.h"
#include "lib/string.h"
#include "cpu/gdt.h"

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX] = {0};

struct ProcessManagerState process_manager_state = {
    .is_process_active = {
        [0 ... PROCESS_COUNT_MAX - 1] = false},
    .active_process_count = 0,
    .current_running_pid = NO_RUNNING_PROCESS
};

int32_t process_list_get_inactive_index() {
    // Asumsi bahwa selalu ada index yang tidak aktif
    int32_t ret = 0;
    while (ret < PROCESS_COUNT_MAX && process_manager_state.is_process_active[ret])
    {
        ret++;
    }
    process_manager_state.is_process_active[ret] = true;
    process_manager_state.active_process_count++;
    return ret;
}

struct ProcessControlBlock *process_get_current_running_pcb_pointer(void) {
    
    for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++)
    {
        if (process_manager_state.is_process_active[i] && _process_list[i].metadata.process_state == RUNNING)
        {
            return &_process_list[i];
        }
    }
    return NULL;
}

int32_t process_create_user_process(struct EXT2DriverRequest request) {

    // 0 - Validasi & pengecekan beberapa kondisi kegagalan

    int32_t ret_code = PROCESS_CREATE_SUCCESS;
    if (process_manager_state.active_process_count >= PROCESS_COUNT_MAX)
    {
        ret_code = PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED;
        goto exit_cleanup;
    }

    // Pastikan entrypoint tidak berada di area kernel pada higher half
    if ((uint32_t)request.buf >= KERNEL_VIRTUAL_ADDRESS_BASE)
    {
        ret_code = PROCESS_CREATE_FAIL_INVALID_ENTRYPOINT;
        goto exit_cleanup;
    }

    // Cek apakah page frame yang dibutuhkan untuk executable + stack tidak melebihi batas maksimal
    uint32_t page_frame_count_needed = ceil_div(request.buffer_size + PAGE_FRAME_SIZE, PAGE_FRAME_SIZE);
    if (!paging_allocate_check(page_frame_count_needed) || page_frame_count_needed > PROCESS_PAGE_FRAME_COUNT_MAX)
    {
        ret_code = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }

    int32_t p_index = process_list_get_inactive_index();
    struct ProcessControlBlock *new_pcb = &(_process_list[p_index]);

    // Local copy dari filename di memory kernel
    char filename_copy[256];
    memset(filename_copy, 0, sizeof(filename_copy));
    memcpy(filename_copy, request.name, request.name_len);

    // Copy dari seluruh request
    struct EXT2DriverRequest local_request = request;
    local_request.name = filename_copy;

    // 1 - Pembuatan virtual address space baru dengan page directory

    struct PageDirectory *pd_old = paging_get_current_page_directory_addr();
    struct PageDirectory *pd_new = paging_create_new_page_directory();

    // 2 - Membaca dan melakukan load executable dari file system ke memory baru

    new_pcb->memory.virtual_addr_used[0] = KERNEL_VIRTUAL_ADDRESS_BASE + (void *)paging_allocate_user_page_frame(pd_new, request.buf);
    new_pcb->memory.virtual_addr_used[1] = KERNEL_VIRTUAL_ADDRESS_BASE + (void *)paging_allocate_user_page_frame(pd_new, (void *)0xBFFFFFFC);
    paging_use_page_directory(pd_new);
    if (read(local_request) != 0)
    {
        ret_code = PROCESS_CREATE_FAIL_FS_READ_FAILURE;
        goto exit_cleanup;
    }
    paging_use_page_directory(pd_old);

    // 3 - Menyiapkan state & context awal untuk program

    new_pcb->context.cpu.segment.ds = 0x3 | 0x20;
    new_pcb->context.cpu.segment.es = 0x3 | 0x20;
    new_pcb->context.cpu.segment.fs = 0x3 | 0x20;
    new_pcb->context.cpu.segment.gs = 0x3 | 0x20;
    new_pcb->context.cpu.stack.ebp = 0xBFFFFFFC;
    new_pcb->context.cpu.stack.esp = 0xBFFFFFFC;
    new_pcb->context.eip = (uint32_t)request.buf;

    new_pcb->context.page_directory_virtual_addr = pd_new;
    new_pcb->context.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;

    // 4 - Mencatat semua informasi penting process ke metadata PCB

    new_pcb->metadata.process_state = READY;
    new_pcb->metadata.pid = p_index;

    memset(new_pcb->metadata.name, 0, PROCESS_NAME_LENGTH_MAX);
    memcpy(new_pcb->metadata.name, request.name, 8);

    // 5 - Mengembalikan semua state register dan memory sama sebelum process creation

exit_cleanup:
    return ret_code;
}

bool process_destroy(uint32_t pid) {

    if (!process_manager_state.is_process_active[pid]) return false;
    else
    {
        // Bersihkan area clock di layar VGA kalau di-kill
        for (int i = 0; i < 8; i++)
            vga_draw_char(70 + i, 59, ' ', 0x0);

        process_manager_state.is_process_active[pid] = false;
        process_manager_state.active_process_count--;
        struct ProcessControlBlock *delete_process = &_process_list[pid];
        paging_free_page_directory(delete_process->context.page_directory_virtual_addr);

        // Mungkin free page frame yang digunakan, kurang yakin juga
        memset(delete_process, 0, sizeof(struct ProcessControlBlock));
        return true;
    }
}

uint32_t ceil_div(uint32_t numerator, uint32_t denominator) {
    return (numerator + denominator - 1) / denominator;
}