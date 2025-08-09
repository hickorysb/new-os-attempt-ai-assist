section .text
bits 64

; External C-level handler
extern interrupt_handler

; GDT flush function
global gdt_flush
gdt_flush:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    push 0x08
    lea rax, [rel .reload_cs]
    push rax
    retfq
.reload_cs:
    ret

; IDT load function
global idt_load
idt_load:
    lidt [rdi]
    ret

; Common stub for all interrupts
; This stub saves registers and passes a pointer to a struct to the C-level handler.
isr_common_stub:
    ; We have already pushed the interrupt number and a dummy error code (if needed)
    ; Now, we push all general-purpose registers to perfectly match the `registers_t` struct layout.
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    ; The stack now holds the `registers_t` struct. Pass a pointer to it to the C++ handler.
    ; rdi is used for the first function argument in x86_64 calling conventions.
    mov rdi, rsp
    
    ; The stack is now 8-byte aligned. Add 8 bytes to align it to 16 bytes for the C++ function call.
    sub rsp, 8
    
    call interrupt_handler
    
    ; Restore stack alignment.
    add rsp, 8
    
    ; Restore all general-purpose registers in reverse order.
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    
    ; Pop the interrupt number and error code (16 bytes total) that we pushed earlier.
    add rsp, 16
    
    ; Return from interrupt.
    iretq

; Macro to create an ISR stub with no error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0                  ; Push a dummy error code
    push %1                 ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; Macro to create an ISR stub with an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1                 ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; CPU Exceptions
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31

; Generate stubs for IRQs and the rest
%assign i 32
%rep 256-32
    ISR_NOERRCODE i
%assign i i+1
%endrep

; Table of pointers to the stubs
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dq isr%[i]
%assign i i+1
%endrep
