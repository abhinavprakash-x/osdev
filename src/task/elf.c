#include "task/elf.h"
#include "task/task.h"
#include "mm/paging.h"
#include "mm/heap.h"

bool elf_validate(const void* image, uint32_t size)
{
    if(image == 0) return false;
    if(size < sizeof(Elf32_Ehdr)) return false;

    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)image;

    if(ehdr->e_ident[0] != ELFMAG0) return false;
    if(ehdr->e_ident[1] != ELFMAG1) return false;
    if(ehdr->e_ident[2] != ELFMAG2) return false;
    if(ehdr->e_ident[3] != ELFMAG3) return false;

    if(ehdr->e_ident[4] != ELFCLASS32) return false;
    if(ehdr->e_ident[5] != ELFDATA2LSB) return false;
    if(ehdr->e_ident[6] != 1) return false;

    if(ehdr->e_type != ET_EXEC) return false;
    if(ehdr->e_machine != EM_386) return false;
    if(ehdr->e_version != 1) return false;

    if(ehdr->e_ehsize != sizeof(Elf32_Ehdr)) return false;
    if(ehdr->e_phentsize != sizeof(Elf32_Phdr)) return false;
    if(ehdr->e_phnum == 0) return false;

    if(ehdr->e_phoff > size) return false;
    uint32_t ph_table_bytes = (uint32_t)ehdr->e_phnum * (uint32_t)ehdr->e_phentsize;
    if(ph_table_bytes > (size - ehdr->e_phoff)) return false;

    const Elf32_Phdr* phdr = (const Elf32_Phdr*)((const uint8_t*)image + ehdr->e_phoff);
    bool has_load_segment = false;
    bool entry_valid = false;

    for(uint16_t i = 0; i < ehdr->e_phnum; ++i)
    {
        if(phdr[i].p_type != PT_LOAD) continue;

        if(phdr[i].p_memsz == 0) return false;
        if(phdr[i].p_filesz > phdr[i].p_memsz) return false;

        if(phdr[i].p_offset > size) return false;
        if(phdr[i].p_filesz > (size - phdr[i].p_offset)) return false;

        uint32_t vaddr_start = phdr[i].p_vaddr;
        uint32_t vaddr_end   = vaddr_start + phdr[i].p_memsz - 1u;
        if(vaddr_end < vaddr_start) return false;

        if(vaddr_start < USER_SPACE_START || vaddr_end > USER_SPACE_END) return false;
        if(vaddr_start < HEAP_MAX_ADDR && vaddr_end >= HEAP_START) return false;

        uint32_t stack_start = USER_STACK_TOP_ADDR - PAGE_SIZE;
        uint32_t stack_end   = USER_STACK_TOP_ADDR - 1u;
        if(vaddr_start <= stack_end && vaddr_end >= stack_start) return false;

        uint32_t page_start_i = vaddr_start & 0xFFFFF000u;
        uint32_t page_end_i   = vaddr_end & 0xFFFFF000u;
        for(uint16_t j = 0; j < i; ++j)
        {
            if(phdr[j].p_type != PT_LOAD) continue;
            uint32_t page_start_j = phdr[j].p_vaddr & 0xFFFFF000u;
            uint32_t page_end_j   = (phdr[j].p_vaddr + phdr[j].p_memsz - 1u) & 0xFFFFF000u;

            if(page_start_i <= page_end_j && page_end_i >= page_start_j) return false;
        }
        has_load_segment = true;

        if(ehdr->e_entry >= vaddr_start && ehdr->e_entry <= vaddr_end && (phdr[i].p_flags & PF_X) != 0)
            entry_valid = true;
    }

    return has_load_segment && entry_valid;
}