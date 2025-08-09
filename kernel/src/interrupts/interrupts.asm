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
isr_common_stub:
    ; Save registers
    push rdi; push rsi; push rdx; push rcx; push rbx; push rax; push rbp
    push r8; push r9; push r10; push r11; push r12; push r13; push r14; push r15

    ; Pass pointer to registers to C handler
    mov rdi, rsp
    
    ; Align stack and call
    sub rsp, 8
    and rsp, -16
    call interrupt_handler
    add rsp, 8

    ; Restore registers
    pop r15; pop r14; pop r13; pop r12; pop r11; pop r10; pop r9; pop r8
    pop rbp; pop rax; pop rbx; pop rcx; pop rdx; pop rsi; pop rdi
    
    ; Pop error code and int number
    add rsp, 16
    iretq

; Macro to create an ISR stub with no error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0
    push %1
    jmp isr_common_stub
%endmacro

; Macro to create an ISR stub with an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1
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
