; Primary ATA I/O ports
;ATA_DATA       equ 0x1F0  ; Data port (read/write)
;ATA_ERROR      equ 0x1F1  ; Error register (read)
;ATA_FEATURES   equ 0x1F1  ; Features (write)
;ATA_SECTOR_CNT equ 0x1F2  ; Sector count
;ATA_LBA_LOW    equ 0x1F3  ; LBA low byte
;ATA_LBA_MID    equ 0x1F4  ; LBA mid byte
;ATA_LBA_HIGH   equ 0x1F5  ; LBA high byte
;ATA_DRIVE      equ 0x1F6  ; Drive/head
;ATA_STATUS     equ 0x1F7  ; Status (read)
;ATA_COMMAND    equ 0x1F7  ; Command (write)

; Status bits
;STATUS_BSY     equ 0x80
;STATUS_DRQ     equ 0x08

; Commands
;CMD_READ_SECTORS  equ 0x20
;CMD_WRITE_SECTORS equ 0x30

; int read_lba_asm(PBFS_DAP* dap)
global read_lba_asm
read_lba_asm:
    pusha
    mov esi, edi ; esi points to DAP
    ; Load DAP values
    movzx ecx, word [esi + 2] ; sector count
    movzx eax, word [esi + 4] ; offset
    movzx edi, word [esi + 6] ; segment
    mov ebx, dword [esi + 8] ; lower 32-bits of lba
    mov edx, dword [esi + 12] ; upper 32-bits of lba

    ; compute linear mem addr
    shl edi, 4 ; segment * 16
    add edi, eax ; add offset
    ; edi now points to mem addr
    ; Setup ATA primary port (assuming master drive)
    mov al, 0xE0 ; master drive + lba
    out 0x1F2, al

    ; set sector count
    mov al, cl
    out 0x1F2, al

    ; set lba bytes (32-bit)
    mov al, bl
    out 0x1F3, al
    mov al, bh
    out 0x1F4, al
    mov al, dl
    out 0x1F5, al
    mov al, 0
    out 0x1F6, al

    ; Send command
    mov al, 0x20
    out 0x1F7, al

read_wait:
    in al, 0x1F7
    test al, 0x80
    jnz read_wait ; wait for bsy=0

read_drq:
    in al, 0x1F7
    test al, 0x08
    jz read_drq ; wair for drq=1
    ; read sector data (512-bytes)
    mov cx, 256
read_loop:
    in ax, 0x1F0
    stosw
    loop read_loop

    popa
    ret

write_lba_asm:
    ret ; placeholder
