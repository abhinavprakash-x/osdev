#include "drivers/fs.h"
#include "drivers/ata.h"
#include "libc/string.h"
#include "libc/stdlib.h"
#include "libc/mem.h"
#include "mm/heap.h"

static fs_superblock_t superblock;
static bool fs_mounted = false;

bool fs_init(void)
{
    fs_mounted = false;
    if(!ata_read_sectors(FS_SUPERBLOCK_LBA, 1, (uint8_t*)&superblock)) return false;
    if(superblock.magic != FS_MAGIC) return false;
    if(superblock.file_count > FS_MAX_FILES) return false;

    for(uint32_t i = 0; i < superblock.file_count; ++i)
        superblock.entries[i].name[FS_MAX_FILENAME - 1u] = '\0';

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
    if(sectors == 0 || sectors > 255u) return false;

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

bool fs_delete_file(const char* name)
{
    if(!fs_mounted || name == 0 || name[0] == '\0') return false;
    bool found = false;

    for(uint32_t i = 0; i < superblock.file_count; ++i)
    {
        if(strncmp(superblock.entries[i].name, name, FS_MAX_FILENAME) == 0)
        {
            found = true;
            for(uint32_t j = i; j + 1 < superblock.file_count; ++j)
                superblock.entries[j] = superblock.entries[j + 1];

            break;
        }
    }

    if(found)
    {
        memset(&superblock.entries[superblock.file_count - 1], 0, sizeof(fs_entry_t));
        superblock.file_count--;
        return ata_write_sectors(FS_SUPERBLOCK_LBA, 1, &superblock);
    }

    return false;
}

static uint32_t fs_find_next_free_lba(void)
{
    uint32_t next_lba = FS_SUPERBLOCK_LBA + 1u;
    for(uint32_t i = 0; i < superblock.file_count; ++i)
    {
        uint32_t sectors = (superblock.entries[i].size + 511u) / 512u;
        uint32_t end_lba = superblock.entries[i].start_lba + sectors;
        if(end_lba > next_lba)
        {
            next_lba = end_lba;
        }
    }
    return next_lba;
}

bool fs_write_file(const char* name, const void* data, uint32_t size)
{
    if(!fs_mounted || name == 0 || name[0] == '\0') return false;
    size_t name_len = strlen(name);
    if(name_len >= FS_MAX_FILENAME || data == 0 || size == 0) return false;

    uint32_t sectors_needed = (size + 511u) / 512u;
    if(sectors_needed == 0 || sectors_needed > 255u) return false;

    int existing_idx = -1;
    for(uint32_t i = 0; i < superblock.file_count; ++i)
    {
        if(strncmp(superblock.entries[i].name, name, FS_MAX_FILENAME) == 0)
        {
            existing_idx = (int)i;
            break;
        }
    }

    uint32_t target_lba = 0;
    uint32_t slot = 0;

    if(existing_idx >= 0)
    {
        slot = (uint32_t)existing_idx;
        uint32_t old_sectors = (superblock.entries[slot].size + 511u) / 512u;
        if(sectors_needed <= old_sectors) target_lba = superblock.entries[slot].start_lba;
        else target_lba = fs_find_next_free_lba();
    }
    else
    {
        if(superblock.file_count >= FS_MAX_FILES) return false;
        slot = superblock.file_count;
        target_lba = fs_find_next_free_lba();
    }

    // Allocate sector-aligned bounce buffer and copy payload
    void* bounce = kmalloc(sectors_needed * 512u);
    if (bounce == 0) return false;

    memset(bounce, 0, sectors_needed * 512u);
    memcpy(bounce, data, size);

    bool write_ok = ata_write_sectors(target_lba, (uint8_t)sectors_needed, bounce);
    kfree(bounce);
    if (!write_ok) return false;

    // Update directory entry and flush superblock to disk
    memset(superblock.entries[slot].name, 0, FS_MAX_FILENAME);
    memcpy(superblock.entries[slot].name, name, name_len);

    superblock.entries[slot].start_lba = target_lba;
    superblock.entries[slot].size      = size;
    superblock.entries[slot].reserved  = 0;

    if(existing_idx < 0) superblock.file_count++;
    return ata_write_sectors(FS_SUPERBLOCK_LBA, 1, &superblock);
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
        printf("%s\t\t%d\t\t%d\n", entry->name, (int)entry->size, (int)entry->start_lba);
    }
}