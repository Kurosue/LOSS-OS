#include "cpu/interrupt.h"


void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100); // ICW3: tell Master PIC, slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Disable all interrupts
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}

void main_interrupt_handler(struct InterruptFrame frame)
{
    switch (frame.int_number){
        case 0xd: // General Protection Fault (13)
            __asm__ volatile("hlt");
            break;
        case 0xe: // Page Fault (14)
            __asm__ volatile("hlt");
            break;
        case IRQ_KEYBOARD + PIC1_OFFSET: // Keyboard (33)
            keyboard_isr();
            break;

        case 0x30: // System call (48)
            syscall(frame);
            break;

        default:
            if (frame.int_number < 32)
            {
                // Any CPU exception
                __asm__ volatile("hlt");
            }
            else if (frame.int_number >= PIC1_OFFSET && frame.int_number < PIC1_OFFSET + 16)
            {
                uint8_t irq = frame.int_number - PIC1_OFFSET;

                // Acknowledge the interrupt
                pic_ack(irq);
            }
            break;
    }
}

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

void syscall(struct InterruptFrame frame) {
    switch (frame.cpu.general.eax) {
        case 0: // Read
            *((int8_t*) frame.cpu.general.ecx) = read(
                *(struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case 1: // Read Dir
            *((int8_t*) frame.cpu.general.ecx) = read_directory(
                (struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case 2: // Write
            *((int8_t*) frame.cpu.general.ecx) = write(
                (struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case 3: // Delete
            *((int8_t*) frame.cpu.general.ecx) = delete(
                *(struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break; 
        case 4: // GetChar dari keyboard
            get_keyboard_buffer((char*) frame.cpu.general.ebx);
            break;
        case 5: // Putchar ke output wak
            putchar(
                (char) frame.cpu.general.ebx, 
                frame.cpu.general.ecx
            );
            break;
        case 6: // Puts doang
            puts(
                (char *)frame.cpu.general.ebx, 
                frame.cpu.general.ecx, 
                frame.cpu.general.edx
            ); // Assuming puts() exist in kernel
            break;
        case 7: 
            keyboard_state_activate();
            break;
        case 8:
            clear_screen();
            break;
        case 9:
            // create process user buat exec
            *((int8_t*) frame.cpu.general.ecx) = process_create_user_process(
                *(struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            scheduler_switch_to_next_process();
            break;
        case 10:
            // Exit dari process
            struct ProcessControlBlock* current = process_get_current_running_pcb_pointer();
            if (current == NULL) {
                // Gagal: tidak ada proses aktif? fallback
                while (1) asm volatile("hlt");
            }

            uint32_t pid = current->metadata.pid;
            process_destroy(pid);
            scheduler_switch_to_next_process();

        case 11:
            // Kill atau terminati user based on PID
            *((int8_t*) frame.cpu.general.ecx) = process_destroy(
                frame.cpu.general.ebx
            );
            break;
        case 12: {
            // Langsung disini karena address yang direturn bisa menyebabkan page fault kalau sampe user mode
            puts("PID   Name                            State", strlen("PID  Name                            State"), 0xD);
            putchar('\n', 0xC);
            for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++){
                if (!process_manager_state.is_process_active[i]) continue;
                struct ProcessControlBlock *pcb = &_process_list[i];

                char *state_str;
                switch (pcb->metadata.process_state) {
                    case RUNNING: state_str = "RUNNING"; break;
                    case READY:   state_str = "READY";   break;
                    case BLOCK:   state_str = "BLOCKED"; break;
                    default:      state_str = "UNKNOWN"; break;
                }
                puts(itoa(pcb->metadata.pid), strlen(itoa(pcb->metadata.pid)), 0xE);
                puts("      ", strlen("      ") - strlen(itoa(pcb->metadata.pid)), 0xE);
                puts(pcb->metadata.name, strlen(pcb->metadata.name), 0xE);
                puts("                                ", strlen("                                ") - strlen(pcb->metadata.name), 0xC);
                puts(state_str, strlen(state_str), 0xE);
            }
            break;
        }
           
    }
}

char* itoa(int i) {
    static char buffer[12];
    char *ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    if (i == 0) {
        *--ptr = '0';
        return ptr;
    }

    int sign = i < 0 ? -1 : 1;
    if (sign < 0) {
        i = -i;
    }

    while (i > 0) {
        *--ptr = (i % 10) + '0';
        i /= 10;
    }

    if (sign < 0) {
        *--ptr = '-';
    }

    return ptr;
}
