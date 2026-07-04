[bits 32]

global load_page_directory
; void load_page_directory(uint32_t* dir);

load_page_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]   ; Grab the pointer to the page directory
    mov cr3, eax       ; Load it into the CPU's Page Directory Base Register (CR3)
    mov esp, ebp
    pop ebp
    ret

global enable_paging
; void enable_paging();

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000 ; Set the PG bit (Bit 31)
    mov cr0, eax       ; PAGING IS NOW ON!
    mov esp, ebp
    pop ebp
    ret