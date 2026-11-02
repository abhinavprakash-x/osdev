#include "task/elf.h"
#include "task/task.h"
#include "task/scheduler.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "libc/mem.h"

extern void enter_usermode(uint32_t user_eip, uint32_t user_esp);

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

task_t* create_user_task_elf(const char* name, const void* image, uint32_t size)
{
    if(name == 0 || name[0] == '\0') return 0;
    if(!elf_validate(image, size)) return 0;

    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)image;
    const Elf32_Phdr* phdr = (const Elf32_Phdr*)((const uint8_t*)image + ehdr->e_phoff);

    uint32_t* page_directory = paging_create_address_space();
    if (page_directory == 0) return 0;

    // 1. Load each PT_LOAD segment into the user address space
    for (uint16_t i = 0; i < ehdr->e_phnum; ++i)
    {
        if (phdr[i].p_type != PT_LOAD) continue;

        uint32_t seg_vaddr    = phdr[i].p_vaddr;
        uint32_t seg_file_end = seg_vaddr + phdr[i].p_filesz;
        uint32_t first_page   = seg_vaddr & 0xFFFFF000u;
        uint32_t last_page    = (seg_vaddr + phdr[i].p_memsz - 1u) & 0xFFFFF000u;

        // Apply segment permissions: read-only unless PF_W is set
        uint32_t pte_flags = PTE_PRESENT | PTE_USER;
        if((phdr[i].p_flags & PF_W) != 0) pte_flags |= PTE_RW;

        for (uint32_t page = first_page; ; page += PAGE_SIZE)
        {
            uint32_t phys = (uint32_t)pmm_alloc_block();
            if (phys == 0)
            {
                paging_destroy_address_space(page_directory);
                return 0;
            }

            // Copy segment file bytes and zero-fill .bss / padding via temporary mapping
            uint32_t eflags;
            __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

            void* temp_page = paging_temp_map(phys);
            memset(temp_page, 0, PAGE_SIZE);

            uint32_t copy_start = (seg_vaddr > page) ? seg_vaddr : page;
            uint32_t copy_end   = (seg_file_end < (page + PAGE_SIZE)) ? seg_file_end : (page + PAGE_SIZE);

            if (copy_start < copy_end)
            {
                uint32_t page_offset   = copy_start - page;
                uint32_t file_offset   = phdr[i].p_offset + (copy_start - seg_vaddr);
                uint32_t bytes_to_copy = copy_end - copy_start;

                memcpy((uint8_t*)temp_page + page_offset,
                       (const uint8_t*)image + file_offset,
                       bytes_to_copy);
            }

            paging_temp_unmap();
            __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

            if (!paging_map_page(page_directory, page, phys, pte_flags))
            {
                pmm_free_block((void*)phys);
                paging_destroy_address_space(page_directory);
                return 0;
            }

            if (page == last_page) break;
        }
    }

    // 2. Allocate and map the user stack page
    if (!paging_alloc_pages(page_directory, USER_STACK_TOP_ADDR - PAGE_SIZE, 1, PTE_RW | PTE_USER))
    {
        paging_destroy_address_space(page_directory);
        return 0;
    }

    // 3. Allocate PCB and kernel stack
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    if (new_task == 0)
    {
        paging_destroy_address_space(page_directory);
        return 0;
    }

    void* stack_memory = kmalloc(PAGE_SIZE);
    if (stack_memory == 0)
    {
        kfree(new_task);
        paging_destroy_address_space(page_directory);
        return 0;
    }

    // 4. Forge the kernel stack for enter_usermode(ehdr->e_entry, USER_STACK_TOP_ADDR)
    uint32_t* stack = (uint32_t*)((uint8_t*)stack_memory + PAGE_SIZE);
    new_task->kernel_stack_top = (uint32_t)stack;

    *(--stack) = USER_STACK_TOP_ADDR;
    *(--stack) = ehdr->e_entry;
    *(--stack) = (uint32_t)task_exit;
    *(--stack) = (uint32_t)enter_usermode;
    *(--stack) = 0x202; // EFLAGS (IF = 1)

    // pushad zeroes
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    new_task->esp = (uint32_t)stack;
    new_task->next = 0;
    new_task->name = name;
    new_task->state = TASK_READY;
    new_task->exit_status = 0;
    new_task->stack_allocation = stack_memory;
    new_task->page_directory = page_directory;
    new_task->wake_time = 0;

    new_task->pid = allocate_pid();
    if (new_task->pid == 0)
    {
        destroy_task(new_task);
        return 0;
    }

    return new_task;
}