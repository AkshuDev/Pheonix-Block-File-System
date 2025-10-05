BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; load stage 2
    mov si, disk_read_msg
    call print_string

    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_err

    jmp 0x0100:0x0000 ; Jump

disk_err:
    mov si, err_msg 
    call print_string
    hlt

print_string:
    lodsb
    or al, al 
    jz .done
    mov ah, 0x0E
    int 0x10
    mov dx, 0x3F8 ; COM!
    out dx, al
    jmp print_string
.done:
    ret

boot_drive: db 0x80
disk_read_msg db "Loading stage 2 ...", 0xa, 0xd, 0
err_msg db "Disk read error!", 0xa, 0xd, 0

dap:
    db 0x10 ; size
    db 0 ; Reserved
    dw 9 ; number of sectors
    dw 0x0000 ; dest offset
    dw 0x0100 ; dest segment
    dq 4 ; starting lba

times 510 - ($ - $$) db 0
dw 0xAA55
