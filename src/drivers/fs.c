#include "drivers/fs.h"
#include "drivers/ata.h"

fs_superblock_t superblock;
bool fs_mounted = false;

bool fs_init(void)
{
    if(!ata_read_sectors(151, 1, (uint8_t*)&superblock)) return false;
    if(superblock.magic != FS_MAGIC) return false;
    fs_mounted = true;
    return true;
}