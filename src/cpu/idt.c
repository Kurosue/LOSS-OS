#include "cpu/idt.h"

struct InterruptDescriptorTable interrupt_descriptor_table __attribute__((aligned(8)));

struct IDTR _idt_idtr = {
    .size = sizeof(interrupt_descriptor_table) - 1,
    .address = &interrupt_descriptor_table
};

void initialize_idt(void) {
    /* 
     * Iterate all isr_stub_table,
     * Set all IDT entry with set_interrupt_gate()
     * with following values:
     * Vector: i
     * Handler Address: isr_stub_table[i]
     * Segment: GDT_KERNEL_CODE_SEGMENT_SELECTOR
     * Privilege: 0
     */
    for (uint8_t i = 0; i < ISR_STUB_TABLE_LIMIT; i++) {
        set_interrupt_gate(i, isr_stub_table[i], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 0);
    }
    set_interrupt_gate(0x30, isr_stub_table[0x30], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 3);
    __asm__ volatile("lidt %0" : : "m"(_idt_idtr));
    __asm__ volatile("sti");
}
void set_interrupt_gate(
    uint8_t  int_vector, 
    void     *handler_address, 
    uint16_t gdt_seg_selector, 
    uint8_t  privilege
) {
    struct IDTGate *idt_int_gate = &interrupt_descriptor_table.table[int_vector];

    uint32_t addr = (uint32_t)handler_address;

    // Target system 32-bit and flag this as valid interrupt gate
    idt_int_gate->_r_bit_1    = INTERRUPT_GATE_R_BIT_1;
    idt_int_gate->_r_bit_2    = INTERRUPT_GATE_R_BIT_2;
    idt_int_gate->_r_bit_3    = INTERRUPT_GATE_R_BIT_3;
    idt_int_gate->gate_32     = 1;
    idt_int_gate->seg_present = 1;
    idt_int_gate->segment = gdt_seg_selector;
    idt_int_gate->_reserved = 0;
    idt_int_gate->dpl = privilege & 0x3;
    idt_int_gate->offset_low  = addr & 0xFFFF;
    idt_int_gate->offset_high = (addr >> 16) & 0xFFFF;
}


