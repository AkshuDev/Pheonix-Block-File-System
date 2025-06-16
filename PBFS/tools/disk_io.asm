[BITS 16]

[GLOBAL read_lba_asm]
[GLOBAL write_lba_asm]

; int read_lba_asm(PBFS_DAP *dap, uint8_t drive)
read_lba_asm:
    push bp
    mov bp, sp
    push es

    mov si, [bp+4]      ; arg1: pointer to DAP
    mov dl, [bp+6]      ; arg2: drive number

    xor ax, ax
    mov es, ax
    mov ah, 0x42        ; Extended read sectors
    int 0x13
    jc .fail

    xor ax, ax          ; success
    jmp .done

.fail:
    mov ax, 1           ; failure

.done:
    pop es
    pop bp
    ret

; int write_lba_asm(PBFS_DAP *dap, uint8_t drive)
write_lba_asm:
    push bp
    mov bp, sp
    push es

    mov si, [bp+4]      ; arg1: pointer to DAP
    mov dl, [bp+6]      ; arg2: drive number

    xor ax, ax
    mov es, ax
    mov ah, 0x43        ; Extended write sectors
    int 0x13
    jc .fail_write

    xor ax, ax
    jmp .done_write

.fail_write:
    mov ax, 1

.done_write:
    pop es
    pop bp
    ret
