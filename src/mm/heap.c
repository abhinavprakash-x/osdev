#include "heap.h"
#include "paging.h"
#include "pmm.h"

#define HEAP_START 0x10000000
#define HEAP_MAGIC 0xDEADBEEF

static heap_block_t* head = 0;
static uint32_t heap_end_vaddr = HEAP_START + 4096;

void heap_init(void)
{
    uint32_t physical_block = (uint32_t)pmm_alloc_block();
    map_page(HEAP_START, physical_block);

    head = (heap_block_t*)HEAP_START;
    head->magic = HEAP_MAGIC;
    head->size = 4096 - sizeof(heap_block_t);
    head->is_free = 1;
    head->next = 0;
}

void* kmalloc(size_t size)
{
    heap_block_t* ptr = head;
    heap_block_t* last_block = 0;
    while(ptr != 0)
    {
        if(ptr->is_free && ptr->size >= size)
        {
            ptr->is_free = 0;
            return (void*)((uint8_t*)ptr + sizeof(heap_block_t));
        }
        last_block = ptr;
        ptr = ptr->next;
    }
    // Heap full expand it
    uint32_t physical_new_page = (uint32_t)pmm_alloc_block();
    map_page(heap_end_vaddr, physical_new_page);
    heap_block_t* new_block = (heap_block_t*)heap_end_vaddr;

    new_block->magic = HEAP_MAGIC;
    new_block->size = 4096 - sizeof(heap_block_t);
    new_block->is_free = 1;
    new_block->next = 0;

    if (last_block != 0) {
        last_block->next = new_block;
    }
    heap_end_vaddr += 4096;
    return kmalloc(size);
}

void kfree(void* ptr)
{
    if(ptr == 0) return;

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if(!(block->magic == HEAP_MAGIC)) return;
    block->is_free = 1;
}