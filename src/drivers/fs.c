#include "drivers/fs.h"
#include "drivers/ata.h"
#include "libc/string.h"
#include "libc/stdlib.h"
#include "libc/mem.h"
#include "mm/heap.h"

static fs_superblock_t superblock;
static bool fs_mounted = false;
static uint16_t current_dir = FS_ROOT_DIR; // Starts in '/' (0xFFFF)

static int fs_find_in_dir(uint16_t dir_idx, const char* name, uint32_t name_len)
{
    for (uint32_t i = 0; i < FS_MAX_FILES; ++i)
    {
        if(superblock.entries[i].type == FS_TYPE_FREE) continue;
        if(superblock.entries[i].parent != dir_idx) continue;
        if(strlen(superblock.entries[i].name) == name_len &&
            strncmp(superblock.entries[i].name, name, name_len) == 0) return (int)i;
    }
    return -1;
}

static int fs_find_free_slot(void)
{
    for(uint32_t i = 0; i < FS_MAX_FILES; ++i)
    {
        if(superblock.entries[i].type == FS_TYPE_FREE) return (int)i;
    }
    return -1;
}

/*
 * Resolves a relative or absolute path into:
 *   *out_parent : directory index (or FS_ROOT_DIR) containing the final component
 *   out_leaf    : null-terminated final component name ("" if path is "/")
 */
static bool fs_resolve_path(const char* path, uint16_t* out_parent, char* out_leaf)
{
    if(path == 0 || out_parent == 0 || out_leaf == 0) return false;

    while(*path == ' ') path++;
    if(*path == '\0') return false;

    uint16_t dir = (*path == '/') ? FS_ROOT_DIR : current_dir;
    while(*path == '/') path++;

    // Path consisted solely of '/' characters
    if(*path == '\0')
    {
        *out_parent = FS_ROOT_DIR;
        out_leaf[0] = '\0';
        return true;
    }

    const char* p = path;
    while(*p != '\0')
    {
        const char* seg_start = p;
        while(*p != '\0' && *p != '/') p++;
        size_t seg_len = (size_t)(p - seg_start);

        while(*p == '/') p++;

        if (*p != '\0')
        {
            // Intermediate directory component
            if(seg_len == 1 && seg_start[0] == '.') continue;
            if(seg_len == 2 && seg_start[0] == '.' && seg_start[1] == '.')
            {
                if(dir != FS_ROOT_DIR) dir = superblock.entries[dir].parent;
                continue;
            }
            if(seg_len == 0 || seg_len >= FS_MAX_FILENAME) return false;

            int idx = fs_find_in_dir(dir, seg_start, (uint32_t)seg_len);
            if(idx < 0 || superblock.entries[idx].type != FS_TYPE_DIR) return false;

            dir = (uint16_t)idx;
        }
        else
        {
            // Final leaf component
            if(seg_len == 0 || seg_len >= FS_MAX_FILENAME) return false;
            memcpy(out_leaf, seg_start, seg_len);

            out_leaf[seg_len] = '\0';
            *out_parent = dir;
            return true;
        }
    }

    return false;
}

/*
 * First-fit sector allocator: finds the lowest LBA range of length `sectors_needed`
 * starting at FS_SUPERBLOCK_LBA + 1 that does not overlap any active file.
 */
static uint32_t fs_alloc_lba(uint32_t sectors_needed, int ignore_slot)
{
    uint32_t candidate = FS_SUPERBLOCK_LBA + 1u;

    while(1)
    {
        bool conflict = false;
        uint32_t cand_end = candidate + sectors_needed;

        for(uint32_t i = 0; i < FS_MAX_FILES; ++i)
        {
            if((int)i == ignore_slot) continue;
            if(superblock.entries[i].type != FS_TYPE_FILE) continue;
            if(superblock.entries[i].size == 0) continue;

            uint32_t f_start   = superblock.entries[i].start_lba;
            uint32_t f_sectors = (superblock.entries[i].size + 511u) / 512u;
            uint32_t f_end     = f_start + f_sectors;

            if(candidate < f_end && f_start < cand_end)
            {
                candidate = f_end;
                conflict = true;
                break;
            }
        }

        if(!conflict) return candidate;
    }
}

bool fs_init(void)
{
    fs_mounted = false;
    current_dir = FS_ROOT_DIR;

    if(!ata_read_sectors(FS_SUPERBLOCK_LBA, 1, (uint8_t*)&superblock)) return false;
    if(superblock.magic != FS_MAGIC) return false;
    if(superblock.file_count > FS_MAX_FILES) return false;

    uint32_t active_count = 0;
    for(uint32_t i = 0; i < FS_MAX_FILES; ++i)
    {
        superblock.entries[i].name[FS_MAX_FILENAME - 1u] = '\0';

        // Backward-compatibility: upgrade v1 entries that had type == 0 but a valid name/LBA
        // if(superblock.entries[i].type == FS_TYPE_FREE &&
        //     superblock.entries[i].name[0] != '\0' &&
        //     superblock.entries[i].start_lba > FS_SUPERBLOCK_LBA)
        // {
        //     superblock.entries[i].type   = FS_TYPE_FILE;
        //     superblock.entries[i].parent = FS_ROOT_DIR;
        // }

        if(superblock.entries[i].type != FS_TYPE_FREE)
        {
            active_count++;
        }
    }

    superblock.file_count = active_count;
    fs_mounted = true;
    return true;
}

const fs_entry_t* fs_find(const char* path)
{
    if(!fs_mounted || path == 0 || path[0] == '\0') return 0;

    uint16_t parent_dir;
    char leaf[FS_MAX_FILENAME];
    if(!fs_resolve_path(path, &parent_dir, leaf) || leaf[0] == '\0')
        return 0;

    int idx = fs_find_in_dir(parent_dir, leaf, (uint32_t)strlen(leaf));
    if(idx < 0) return 0;

    return &superblock.entries[idx];
}

bool fs_read_file(const char* path, void** out_buffer, uint32_t* out_size)
{
    if(!fs_mounted || path == 0 || path[0] == '\0' || out_buffer == 0 || out_size == 0)
        return false;

    const fs_entry_t* entry = fs_find(path);
    if(entry == 0 || entry->type != FS_TYPE_FILE || entry->size == 0 || entry->start_lba <= FS_SUPERBLOCK_LBA)
        return false;

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

bool fs_delete_file(const char* path)
{
    if(!fs_mounted || path == 0 || path[0] == '\0') return false;

    uint16_t parent_dir;
    char leaf[FS_MAX_FILENAME];
    if(!fs_resolve_path(path, &parent_dir, leaf) || leaf[0] == '\0')
        return false;

    if(strcmp(leaf, ".") == 0 || strcmp(leaf, "..") == 0)
        return false;

    int idx = fs_find_in_dir(parent_dir, leaf, (uint32_t)strlen(leaf));
    if(idx < 0) return false;

    // If deleting a directory, refuse if it is current_dir or non-empty
    if(superblock.entries[idx].type == FS_TYPE_DIR)
    {
        if((uint16_t)idx == current_dir) return false;

        for(uint32_t i = 0; i < FS_MAX_FILES; ++i)
        {
            if(superblock.entries[i].type != FS_TYPE_FREE &&
                superblock.entries[i].parent == (uint16_t)idx)
            {
                return false; // Directory is not empty
            }
        }
    }

    memset(&superblock.entries[idx], 0, sizeof(fs_entry_t));
    if(superblock.file_count > 0) superblock.file_count--;

    return ata_write_sectors(FS_SUPERBLOCK_LBA, 1, &superblock);
}

bool fs_write_file(const char* path, const void* data, uint32_t size)
{
    if(!fs_mounted || path == 0 || path[0] == '\0' || data == 0 || size == 0)
        return false;

    uint16_t parent_dir;
    char leaf[FS_MAX_FILENAME];
    if(!fs_resolve_path(path, &parent_dir, leaf) || leaf[0] == '\0')
        return false;

    if(strcmp(leaf, ".") == 0 || strcmp(leaf, "..") == 0)
        return false;

    uint32_t sectors_needed = (size + 511u) / 512u;
    if(sectors_needed == 0 || sectors_needed > 255u) return false;

    size_t name_len = strlen(leaf);
    int existing_idx = fs_find_in_dir(parent_dir, leaf, (uint32_t)name_len);

    uint32_t target_lba = 0;
    uint32_t slot = 0;

    if(existing_idx >= 0)
    {
        // Cannot overwrite a directory with a file
        if(superblock.entries[existing_idx].type != FS_TYPE_FILE)
            return false;

        slot = (uint32_t)existing_idx;
        uint32_t old_sectors = (superblock.entries[slot].size + 511u) / 512u;
        if(sectors_needed <= old_sectors) target_lba = superblock.entries[slot].start_lba;
        else target_lba = fs_alloc_lba(sectors_needed, existing_idx);
    }
    else
    {
        int free_slot = fs_find_free_slot();
        if(free_slot < 0) return false;

        slot = (uint32_t)free_slot;
        target_lba = fs_alloc_lba(sectors_needed, -1);
    }

    void* bounce = kmalloc(sectors_needed * 512u);
    if(bounce == 0) return false;

    memset(bounce, 0, sectors_needed * 512u);
    memcpy(bounce, data, size);

    bool write_ok = ata_write_sectors(target_lba, (uint8_t)sectors_needed, bounce);
    kfree(bounce);
    if(!write_ok) return false;

    memset(&superblock.entries[slot], 0, sizeof(fs_entry_t));
    memcpy(superblock.entries[slot].name, leaf, name_len);
    superblock.entries[slot].start_lba = target_lba;
    superblock.entries[slot].size      = size;
    superblock.entries[slot].parent    = parent_dir;
    superblock.entries[slot].type      = FS_TYPE_FILE;

    if(existing_idx < 0) superblock.file_count++;

    return ata_write_sectors(FS_SUPERBLOCK_LBA, 1, &superblock);
}

bool fs_mkdir(const char* path)
{
    if(!fs_mounted || path == 0 || path[0] == '\0') return false;

    uint16_t parent_dir;
    char leaf[FS_MAX_FILENAME];

    if(!fs_resolve_path(path, &parent_dir, leaf) || leaf[0] == '\0') return false;
    if(strcmp(leaf, ".") == 0 || strcmp(leaf, "..") == 0) return false;

    size_t name_len = strlen(leaf);
    if(fs_find_in_dir(parent_dir, leaf, (uint32_t)name_len) >= 0)
        return false; // Already exists

    int free_slot = fs_find_free_slot();
    if(free_slot < 0) return false;

    fs_entry_t* new_entry = &superblock.entries[free_slot];
    memset(new_entry, 0, sizeof(fs_entry_t));
    memcpy(new_entry->name, leaf, name_len);
    new_entry->start_lba = 0;
    new_entry->size      = 0;
    new_entry->parent    = parent_dir;
    new_entry->type      = FS_TYPE_DIR;

    superblock.file_count++;
    return ata_write_sectors(FS_SUPERBLOCK_LBA, 1, &superblock);
}

bool fs_chdir(const char* path)
{
    if(!fs_mounted || path == 0 || path[0] == '\0') return false;

    uint16_t parent_dir;
    char leaf[FS_MAX_FILENAME];
    
    if(!fs_resolve_path(path, &parent_dir, leaf)) return false;

    // `cd /`
    if(leaf[0] == '\0')
    {
        current_dir = FS_ROOT_DIR;
        return true;
    }

    // `cd .`
    if(strcmp(leaf, ".") == 0)
    {
        current_dir = parent_dir;
        return true;
    }

    // `cd ..`
    if(strcmp(leaf, "..") == 0)
    {
        if(parent_dir != FS_ROOT_DIR) current_dir = superblock.entries[parent_dir].parent;
        else current_dir = FS_ROOT_DIR;
        return true;
    }

    int entry_idx = fs_find_in_dir(parent_dir, leaf, (uint32_t)strlen(leaf));
    if(entry_idx < 0 || superblock.entries[entry_idx].type != FS_TYPE_DIR)
        return false;

    current_dir = (uint16_t)entry_idx;
    return true;
}

void fs_get_pwd(char* buf, size_t max_len)
{
    if(buf == 0 || max_len == 0) return;
    if(!fs_mounted || max_len < 2)
    {
        buf[0] = '\0';
        return;
    }

    if(current_dir == FS_ROOT_DIR)
    {
        buf[0] = '/';
        buf[1] = '\0';
        return;
    }

    uint16_t chain[FS_MAX_FILES];
    uint32_t depth = 0;
    uint16_t dir = current_dir;

    while(dir != FS_ROOT_DIR && dir < FS_MAX_FILES && depth < FS_MAX_FILES)
    {
        if(superblock.entries[dir].type != FS_TYPE_DIR) break;
        chain[depth++] = dir;
        dir = superblock.entries[dir].parent;
    }

    size_t pos = 0;
    for(int i = (int)depth - 1; i >= 0; --i)
    {
        const char* name = superblock.entries[chain[i]].name;
        size_t len = strlen(name);
        if (pos + 1 + len >= max_len) break;

        buf[pos++] = '/';
        memcpy(buf + pos, name, len);
        pos += len;
    }

    if(pos == 0) buf[pos++] = '/';
    buf[pos] = '\0';
}

void fs_list_files(void)
{
    if(!fs_mounted)
    {
        printf("Filesystem not mounted.\n");
        return;
    }

    printf("TYPE\tNAME\t\tSIZE (Bytes)\tSTART LBA\n");
    printf("------------------------------------------------\n");

    for(uint32_t i = 0; i < FS_MAX_FILES; i++)
    {
        const fs_entry_t* entry = &superblock.entries[i];
        if(entry->type == FS_TYPE_FREE) continue;
        if(entry->parent != current_dir) continue;

        if(entry->type == FS_TYPE_DIR) printf("<DIR>\t%s/\t\t-\t\t-\n", entry->name);
        else printf("FILE\t%s\t\t%d\t\t%d\n", entry->name, (int)entry->size, (int)entry->start_lba);
    }
}