#include "heap.h"
#include "paging.h"
#include "pmm.h"

#define HEAP_START 0x10000000
#define HEAP_MAGIC 0xDEADBEEF

#define ALIGN_8(size) (((size) + 7) & ~7)

static heap_block_t* head = 0;
static uint32_t heap_end_vaddr = HEAP_START;

void heap_init(void)
{
    uint32_t physical_block = (uint32_t)pmm_alloc_block();
    map_page(HEAP_START, physical_block);

    head = (heap_block_t*)HEAP_START;
    head->magic = HEAP_MAGIC;
    head->size = 4096 - sizeof(heap_block_t);
    head->is_free = 1;
    head->next = 0;

    heap_end_vaddr = HEAP_START + 4096;
}

void* kmalloc(size_t size)
{
    if(size == 0) return 0;
    size = ALIGN_8(size);

    heap_block_t* ptr = head;
    heap_block_t* last_block = 0;
    while(ptr != 0)
    {
        if(ptr->is_free && ptr->size >= size)
        {
            if (ptr->size > size + sizeof(heap_block_t) + 8)
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
    size_t pages_needed = (total_size_needed + 4095) / 4096;
    
    uint32_t block_start_vaddr = heap_end_vaddr;

    for (size_t i = 0; i < pages_needed; i++) {
        uint32_t physical_new_page = (uint32_t)pmm_alloc_block();
        map_page(heap_end_vaddr, physical_new_page);
        heap_end_vaddr += 4096;
    }

    heap_block_t* new_block = (heap_block_t*)block_start_vaddr;
    new_block->magic = HEAP_MAGIC;
    new_block->size = (pages_needed * 4096) - sizeof(heap_block_t);
    new_block->is_free = 1;
    new_block->next = 0;

    if (last_block != 0) {
        last_block->next = new_block;
    }
    
    // Recursively call kmalloc now that space exists
    return kmalloc(size);
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
    if(ptr == 0) return;

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if(!(block->magic == HEAP_MAGIC)) return;

    block->is_free = 1;
    coalesce_heap();
}