#include "drivers/fs.h"
#include "drivers/ata.h"
#include "libc/string.h"
#include "libc/stdlib.h"
#include "mm/heap.h"

static fs_superblock_t superblock;
static bool fs_mounted = false;

bool fs_init(void)
{
    fs_mounted = false;
    if(!ata_read_sectors(FS_SUPERBLOCK_LBA, 1, (uint8_t*)&superblock)) return false;
    if(superblock.magic != FS_MAGIC) return false;
    fs_mounted = true;
    return true;
}

const fs_entry_t* fs_find(const char* name)
{
    if(!fs_mounted || name == 0 || name[0] == '\0') return 0;

    for(uint32_t i = 0; i < superblock.file_count; i++)
    {
        if(strncmp(superblock.entries[i].name, name, FS_MAX_FILENAME) == 0)
            return &superblock.entries[i];
    }
    return 0;
}

bool fs_read_file(const char* name, void** out_buffer, uint32_t* out_size)
{
    if(!fs_mounted || name == 0 || name[0] == '\0' || out_buffer == 0 || out_size == 0) return false;

    const fs_entry_t* entry = fs_find(name);
    if(entry == 0 || entry->size == 0 || entry->start_lba <= FS_SUPERBLOCK_LBA) return false;

    uint32_t sectors = (entry->size + 511u) / 512u;
    if (sectors == 0 || sectors > 255u) return false;

    void* buf = kmalloc(sectors * 512u);
    if(buf == 0) return false;

    if(!ata_read_sectors(entry->start_lba, (uint8_t)sectors, buf))
    {
        kfree(buf);
        return false;
    }

    *out_buffer = buf;
    *out_size = entry->size;
    return true;
}

void fs_list_files(void)
{
    if(!fs_mounted)
    {
        printf("Filesystem not mounted.\n");
        return;
    }

    printf("NAME\t\tSIZE (Bytes)\tSTART LBA\n");
    printf("----------------------------------------\n");

    for(uint32_t i = 0; i < superblock.file_count; i++)
    {
        const fs_entry_t* entry = &superblock.entries[i];
        printf("%s, %u bytes, %u\n", entry->name, entry->size, entry->start_lba);
    }
}