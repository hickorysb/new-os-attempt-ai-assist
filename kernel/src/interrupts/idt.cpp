#include "idt.h"
#include "interrupts.h"
#include "gdt.h"
#include "../lib/memory/memory.h"
#include "../devices/pic.h"
#include "../devices/fb.h"
#include "../lib/sys_utils.h"

#define IDT_ENTRIES 256

static IdtEntry idt_entries[IDT_ENTRIES];
static IdtPtr   idt_ptr;
static isr_t    interrupt_handlers[IDT_ENTRIES];

// These symbols are defined in our assembly file.
extern "C" {
    extern void* isr_stub_table[];
    extern void idt_load(uint64_t);
}

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].isr_low    = (uint16_t)(base & 0xFFFF);
    idt_entries[num].isr_mid    = (uint16_t)((base >> 16) & 0xFFFF);
    idt_entries[num].isr_high   = (uint32_t)((base >> 32) & 0xFFFFFFFF);
    idt_entries[num].kernel_cs  = sel;
    idt_entries[num].ist        = 0;
    idt_entries[num].attributes = flags;
    idt_entries[num].reserved   = 0;
}

void interrupts_init() {
    gdt_init();
    fb_print_string("GDT Initialized.\n", 0x00FFFF00);

    idt_ptr.limit = (sizeof(IdtEntry) * IDT_ENTRIES) - 1;
    idt_ptr.base  = (uint64_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(IdtEntry) * IDT_ENTRIES);

    for (uint16_t i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint64_t)isr_stub_table[i], 0x08, 0x8E);
    }
    
    pic_init();
    fb_print_string("PIC Remapped.\n", 0x00FFFF00);

    idt_load((uint64_t)&idt_ptr);
    fb_print_string("IDT Initialized.\n", 0x00FFFF00);
}

// Corrected to accept a pointer to the registers struct.
extern "C" void interrupt_handler(registers_t* regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else if (regs->int_no < 32) {
        fb_print_string("Unhandled exception: ", 0xFF0000);
        char buffer[32];
        u64_to_str(regs->int_no, buffer);
        fb_print_string(buffer, 0xFF0000);
        fb_print_string("\n", 0xFF0000);
        for(;;); // Halt
    }

    if (regs->int_no >= 32 && regs->int_no < 48) {
        pic_send_eoi(regs->int_no - 32);
    }
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}
