global load_page_directory
load_page_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax       ; Load the page directory address into CR3
    mov esp, ebp
    pop ebp
    ret

global enable_paging
enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000 ; Set the PG bit (Bit 31)
    mov cr0, eax       ; PAGING IS NOW ON!
    mov esp, ebp
    pop ebp
    ret