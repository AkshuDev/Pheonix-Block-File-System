[BITS 16]

section .text
    global get_drive_params_real_asm

get_drive_params_real_asm:
    push es
    xor ax, ax
    mov ax, es

    mov ah, 0x48 ; Func
    ; Drive number already in dl
    ; Buffer already in si
    int 0x13
    jc .error

    xor ax, ax
    pop es
    ret
.error:
    mov ax, 1 ; Error code
    pop es
    ret