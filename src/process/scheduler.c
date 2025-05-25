#include "process/scheduler.h"

void activate_timer_interrupt(void) {
    __asm__ volatile("cli");
    // Setup how often PIT fire
    uint32_t pit_timer_counter_to_fire = PIT_TIMER_COUNTER;
    out(PIT_COMMAND_REGISTER_PIO, PIT_COMMAND_VALUE);
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) (pit_timer_counter_to_fire & 0xFF));
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) ((pit_timer_counter_to_fire >> 8) & 0xFF));

    // Activate the interrupt
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_TIMER));
}

void scheduler_init(void) {
    activate_timer_interrupt();
}

void scheduler_save_context_to_current_running_pcb(struct Context ctx) {
    struct ProcessControlBlock* pcb = process_get_current_running_pcb_pointer();

    if (pcb != NULL) {
        pcb->metadata.process_state = READY;
        struct Context* process_context = &(pcb->context);
        process_context->cpu = ctx.cpu;
        process_context->eip = ctx.eip;
        process_context->eflags = ctx.eflags;
    }
}

void scheduler_switch_to_next_process(void) {
    // simple round robin, no priority yet (tight tight tight deadline)

    int pid = process_manager_state.current_running_pid;

    if (pid == NO_RUNNING_PROCESS){
        pid = -1;
    }

    struct ProcessControlBlock next_process;
    do {
        pid = (pid + 1) % PROCESS_COUNT_MAX; // we loopin
        if (process_manager_state.is_process_active[pid] == true) {
            next_process = _process_list[pid];
            _process_list[pid].metadata.process_state = RUNNING;
            process_manager_state.current_running_pid = pid;
            paging_use_page_directory(next_process.context.page_directory_virtual_addr);
            process_context_switch(next_process.context);           
            break;
        }
    } while (true);

}
