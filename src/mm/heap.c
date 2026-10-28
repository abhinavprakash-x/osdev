#include "mm/heap.h"
#include "mm/paging.h"
#include "mm/pmm.h"

#define ALIGN_8(size) (((size) + 7u) & ~7u)

static heap_block_t* head = 0;
static uint32_t heap_end_vaddr = HEAP_START;

void heap_init(void)
{
    uint32_t physical_block = (uint32_t)pmm_alloc_block();
    if (physical_block == 0) {
        while(1) __asm__ volatile ("cli; hlt");
    }

    if (!map_page(HEAP_START, physical_block, PTE_PRESENT | PTE_RW))
    {
        pmm_free_block((void*)physical_block);
        while(1) __asm__ volatile ("cli; hlt");
    }

    head = (heap_block_t*)HEAP_START;
    head->magic = HEAP_MAGIC;
    head->size = PAGE_SIZE - sizeof(heap_block_t);
    head->is_free = 1;
    head->next = 0;

    heap_end_vaddr = HEAP_START + PAGE_SIZE;
}

static void* kmalloc_unlocked(size_t size)
{
    if (size == 0 || size > (HEAP_MAX_SIZE - sizeof(heap_block_t) - PAGE_SIZE))
        return 0;
    size = ALIGN_8(size);

    heap_block_t* ptr = head;
    heap_block_t* last_block = 0;
    while (ptr != 0)
    {
        if (ptr->is_free && ptr->size >= size)
        {
            if (ptr->size >= size + sizeof(heap_block_t) + 8u)
            {
                // Create a new block immediately after the requested size
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)ptr + sizeof(heap_block_t) + size);
                new_block->magic = HEAP_MAGIC;
                new_block->is_free = 1;
                new_block->size = ptr->size - size - sizeof(heap_block_t);
                new_block->next = ptr->next;

                // Shrink the current block and link it to the new split block
                ptr->size = size;
                ptr->next = new_block;
            }
            ptr->is_free = 0;
            return (void*)((uint8_t*)ptr + sizeof(heap_block_t));
        }
        last_block = ptr;
        ptr = ptr->next;
    }

    size_t total_size_needed = size + sizeof(heap_block_t);
    size_t pages_needed = (total_size_needed + (PAGE_SIZE - 1u)) / PAGE_SIZE;
    uint32_t bytes_needed = (uint32_t)(pages_needed * PAGE_SIZE);

    // BUG-031: Enforce virtual address ceiling for the kernel heap
    if (bytes_needed > (HEAP_MAX_ADDR - heap_end_vaddr))
        return 0;

    uint32_t block_start_vaddr = heap_end_vaddr;
    size_t pages_allocated = 0;

    for (size_t i = 0; i < pages_needed; i++) {
        uint32_t physical_new_page = (uint32_t)pmm_alloc_block();

        if (physical_new_page == 0) {
            for (size_t j = 0; j < pages_allocated; j++)
            {
                uint32_t rollback_vaddr = block_start_vaddr + (j * PAGE_SIZE);
                uint32_t rollback_phys = get_physical_addr(rollback_vaddr) & 0xFFFFF000u;
                unmap_page(rollback_vaddr);

                if (rollback_phys != 0)
                {
                    pmm_free_block((void*)rollback_phys);
                }
            }
            heap_end_vaddr = block_start_vaddr;
            return 0;
        }
        if (!map_page(heap_end_vaddr, physical_new_page, PTE_PRESENT | PTE_RW))
        {
            for (size_t j = 0; j < pages_allocated; j++)
            {
                uint32_t rollback_vaddr = block_start_vaddr + (j * PAGE_SIZE);
                uint32_t rollback_phys = get_physical_addr(rollback_vaddr) & 0xFFFFF000u;
                unmap_page(rollback_vaddr);
                if (rollback_phys != 0)
                {
                    pmm_free_block((void*)rollback_phys);
                }
            }
            pmm_free_block((void*)physical_new_page);
            heap_end_vaddr = block_start_vaddr;
            return 0;
        }
        heap_end_vaddr += PAGE_SIZE;
        pages_allocated++;
    }

    heap_block_t* new_block = (heap_block_t*)block_start_vaddr;
    new_block->magic = HEAP_MAGIC;
    new_block->size = (pages_needed * PAGE_SIZE) - sizeof(heap_block_t);
    new_block->is_free = 1;
    new_block->next = 0;

    if (last_block != 0) last_block->next = new_block;

    if (last_block != 0 && last_block->is_free)
    {
        last_block->size += new_block->size + sizeof(heap_block_t);
        last_block->next = new_block->next;
    }

    return kmalloc_unlocked(size);
}

void* kmalloc(size_t size)
{
    // BUG-047: Disable interrupts so timer preemption / reap_dead_tasks()
    // cannot concurrently mutate the heap list.
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    void* res = kmalloc_unlocked(size);

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
    return res;
}

static void coalesce_heap(void)
{
    heap_block_t* ptr = head;

    while (ptr != 0 && ptr->next != 0)
    {
        if (ptr->is_free && ptr->next->is_free)
        {
            ptr->size += ptr->next->size + sizeof(heap_block_t);
            ptr->next = ptr->next->next;
        }
        else
        {
            ptr = ptr->next;
        }
    }
}

void kfree(void* ptr)
{
    if (ptr == 0) return;
    uint32_t addr = (uint32_t)ptr;

    if ((addr % 8u) != 0) return;

    // BUG-047: Guard heap free & coalescing against timer ISR preemption
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    // BUG-030: Validate pointer range before touching metadata
    if (addr >= (HEAP_START + sizeof(heap_block_t)) && addr < heap_end_vaddr)
    {
        heap_block_t* target = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));

        heap_block_t* curr = head;
        while (curr != 0)
        {
            if (curr == target)
            {
                if (curr->magic == HEAP_MAGIC && !curr->is_free)
                {
                    curr->is_free = 1;
                    coalesce_heap();
                }
                break;
            }
            curr = curr->next;
        }
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

bool heap_is_payload_range(uint32_t addr, size_t size)
{
    if (size == 0) return false;

    uint32_t end = addr + (uint32_t)size - 1u;
    if (end < addr) return false;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    bool valid = false;
    if (addr >= (HEAP_START + sizeof(heap_block_t)) && end < heap_end_vaddr)
    {
        heap_block_t* curr = head;
        while (curr != 0)
        {
            if (curr->magic != HEAP_MAGIC)
                break;

            uint32_t payload_start = (uint32_t)((uint8_t*)curr + sizeof(heap_block_t));
            uint32_t payload_end   = payload_start + (uint32_t)curr->size - 1u;

            if (addr >= payload_start && end <= payload_end)
            {
                valid = (curr->is_free == 0);
                break;
            }
            curr = curr->next;
        }
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
    return valid;
}