.code16
.global get_drive_params_real_asm
get_drive_params_real_asm:
    push %es
    xor %ax, %ax
    mov %ax, %es

    movb $0x48, %ah      # function
    movb %dl, %dl        # drive number already in DL
    movw %si, %si        # buffer already in SI
    int $0x13
    jc error

    xor %ax, %ax         # success
    pop %es
    ret

error:
    mov $1, %ax          # error code
    pop %es
    ret
