#include "../header/cpu/gdt.h"
#include "../header/cpu/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // Null Descriptor - All fields are 0
            .segment_low = 0x0000,
            .base_low = 0x0000,
            .base_mid = 0x00,
            .type_bit = 0x0,
            .non_system = 0x0,
            .dpl = 0x0,
            .seg_present = 0x0,
            .segment_high = 0x0,
            .avl = 0x0,
            .long_mode = 0x0,
            .default_op = 0x0,
            .granularity = 0x0,
            .base_high = 0x00
        },
        {
            // Kernel Code Segment Descriptor (0x08)
            .segment_low = 0xFFFF,    // Segment limit bits 0-15 (4GB limit)
            .base_low = 0x0000,       // Base address bits 0-15
            .base_mid = 0x00,         // Base address bits 16-23
            .type_bit = 0xA,          // Type: code, execute/read, not accessed
            .non_system = 0x1,        // 1 for code and data segments
            .dpl = 0x0,               // Ring 0 (highest privilege level)
            .seg_present = 0x1,       // Segment is present
            .segment_high = 0xF,      // Segment limit bits 16-19
            .avl = 0x0,               // Available for system use (unused)
            .long_mode = 0x0,         // Not a 64-bit code segment
            .default_op = 0x1,        // 32-bit operand size
            .granularity = 0x1,       // 4KB granularity (limit scaled by 4KB)
            .base_high = 0x00         // Base address bits 24-31
        },
        {
            // Kernel Data Segment Descriptor (0x10)
            .segment_low = 0xFFFF,    // Segment limit bits 0-15 (4GB limit)
            .base_low = 0x0000,       // Base address bits 0-15
            .base_mid = 0x00,         // Base address bits 16-23
            .type_bit = 0x2,          // Type: data, read/write, not accessed
            .non_system = 0x1,        // 1 for code and data segments
            .dpl = 0x0,               // Ring 0 (highest privilege level)
            .seg_present = 0x1,       // Segment is present
            .segment_high = 0xF,      // Segment limit bits 16-19
            .avl = 0x0,               // Available for system use (unused)
            .long_mode = 0x0,         // Not used for data segments
            .default_op = 0x1,        // 32-bit operand size
            .granularity = 0x1,       // 4KB granularity (limit scaled by 4KB)
            .base_high = 0x00         // Base address bits 24-31
        },
        {
            .segment_low = 0xFFFF,    // Segment limit bits 0-15 (4GB limit)
            .base_low = 0x0000,       // Base address bits 0-15
            .base_mid = 0x00,         // Base address bits 16-23
            .type_bit = 0xA,          // Type: code, execute/read, not accessed
            .non_system = 0x1,        // 1 for code and data segments
            .dpl = 0x3,               // Ring 3 (user privilege level)
            .seg_present = 0x1,       // Segment is present
            .segment_high = 0xF,      // Segment limit bits 16-19
            .avl = 0x0,               // Available for system use (unused)
            .long_mode = 0x0,         // Not a 64-bit code segment
            .default_op = 0x1,        // 32-bit operand size
            .granularity = 0x1,       // 4KB granularity (limit scaled by 4KB)
            .base_high = 0x00         // Base address bits 24-31
        },
        {
            .segment_low = 0xFFFF,    // Segment limit bits 0-15 (4GB limit)
            .base_low = 0x0000,       // Base address bits 0-15
            .base_mid = 0x00,         // Base address bits 16-23
            .type_bit = 0x2,          // Type: data, read/write, not accessed
            .non_system = 0x1,        // 1 for code and data segments
            .dpl = 0x3,               // Ring 3 (user privilege level)
            .seg_present = 0x1,       // Segment is present
            .segment_high = 0xF,      // Segment limit bits 16-19
            .avl = 0x0,               // Available for system use (unused)
            .long_mode = 0x0,         // Not used for data segments
            .default_op = 0x1,        // 32-bit operand size
            .granularity = 0x1,       // 4KB granularity (limit scaled by 4KB)
            .base_high = 0x00         // Base address bits 24-31
        },
        {
            .segment_high      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low       = sizeof(struct TSSEntry),
            .base_high         = 0,
            .base_mid          = 0,
            .base_low          = 0,
            .non_system        = 0,    // S bit
            .type_bit          = 0x9,
            .dpl               = 0,    // DPL
            .seg_present       = 1,    // P bit
            .default_op        = 1,    // D/B bit
            .long_mode         = 0,    // L bit
            .granularity       = 0,    // G bit
        },
        {0}
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    .size = sizeof(struct GlobalDescriptorTable) - 1,
    .address = &global_descriptor_table
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}
