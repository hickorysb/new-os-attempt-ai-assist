; This file contains the low-level interrupt service routine stubs for x86_64.
; Each of these stubs saves the processor state, calls our main C-level handler,
; and then restores the state before returning.

section .text

; Declare the C-level interrupt handler that we will call.
extern interrupt_handler

; Macro to define an ISR stub.
; %1: The interrupt number.
; %2: A flag indicating if the CPU pushes an error code for this interrupt (1) or not (0).
%macro isr_stub 2
global isr%1
isr%1:
    ; The CPU pushes RIP, CS, RFLAGS, and optionally SS, RSP.
    ; Our stub needs to handle the rest.
    
    %if %2 == 0
        push 0  ; If the CPU doesn't push an error code, we push a dummy one for stack consistency.
    %endif
    
    ; The CPU pushes an error code for some exceptions. We now push the interrupt number.
    ; This, combined with the registers below, forms our 'registers_t' struct.
    push %1
    
    ; Save all general-purpose registers.
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; The first (and only) argument to our C handler is a pointer to the registers structure.
    ; The stack pointer (RSP) now points to the beginning of this structure.
    mov rdi, rsp 
    
    ; Align the stack to a 16-byte boundary before calling the C handler.
    sub rsp, 8

    ; Call the main C-level handler.
    call interrupt_handler

    ; De-align the stack.
    add rsp, 8

    ; Restore all the registers we saved.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; Pop the interrupt number and error code.
    add rsp, 16
    
    ; Return from the interrupt.
    iretq
%endmacro

; Create ISR stubs for CPU exceptions (0-31)
isr_stub 0, 0
isr_stub 1, 0
isr_stub 2, 0
isr_stub 3, 0
isr_stub 4, 0
isr_stub 5, 0
isr_stub 6, 0
isr_stub 7, 0
isr_stub 8, 1  ; Error code
isr_stub 9, 0
isr_stub 10, 1 ; Error code
isr_stub 11, 1 ; Error code
isr_stub 12, 1 ; Error code
isr_stub 13, 1 ; Error code
isr_stub 14, 1 ; Error code
isr_stub 15, 0
isr_stub 16, 0
isr_stub 17, 1 ; Error code
isr_stub 18, 0
isr_stub 19, 0
isr_stub 20, 0
isr_stub 21, 1 ; Error code
isr_stub 22, 0
isr_stub 23, 0
isr_stub 24, 0
isr_stub 25, 0
isr_stub 26, 0
isr_stub 27, 0
isr_stub 28, 0
isr_stub 29, 1 ; Error code
isr_stub 30, 1 ; Error code
isr_stub 31, 0

; Create ISR stubs for hardware interrupts (IRQs 32-47)
isr_stub 32, 0 ; IRQ0: Programmable Interrupt Timer
isr_stub 33, 0 ; IRQ1: Keyboard
isr_stub 34, 0 ; IRQ2: Cascade for 8259A PICs
isr_stub 35, 0 ; IRQ3: COM2
isr_stub 36, 0 ; IRQ4: COM1
isr_stub 37, 0 ; IRQ5: LPT2
isr_stub 38, 0 ; IRQ6: Floppy Disk
isr_stub 39, 0 ; IRQ7: LPT1
isr_stub 40, 0 ; IRQ8: CMOS real-time clock
isr_stub 41, 0 ; IRQ9: Free for peripherals
isr_stub 42, 0 ; IRQ10: Free for peripherals
isr_stub 43, 0 ; IRQ11: Free for peripherals
isr_stub 44, 0 ; IRQ12: PS/2 Mouse
isr_stub 45, 0 ; IRQ13: FPU
isr_stub 46, 0 ; IRQ14: Primary ATA Hard Disk
isr_stub 47, 0 ; IRQ15: Secondary ATA Hard Disk

; Table of ISR stubs, so our C code can easily access their addresses.
global isr_stub_table
isr_stub_table:
    dq isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    dq isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    dq isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    dq isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    dq isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39
    dq isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
