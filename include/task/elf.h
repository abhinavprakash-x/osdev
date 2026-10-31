#pragma once
#include <stdint.h>
#include <stdbool.h>

// ELF Magic
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFCLASS32  1 // 32-Bit
#define ELFDATA2LSB 1 // Little Endian
#define ET_EXEC     2 // Executable
#define EM_386      3 // x86
#define PT_NULL     0
#define PT_LOAD     1
#define PF_X        0x1 // Execute
#define PF_W        0x2 // Write
#define PF_R        0x4 // Read

// 52 Bytes at Byte 0 of File
typedef struct {
    uint8_t e_ident[16];    // Magic and EBI info
    uint16_t e_type;        // File Type
    uint16_t e_machine;     // CPU Arch
    uint32_t e_version;     // ELF Version
    uint32_t e_entry;       // Virtual EIP
    uint32_t e_phoff;       // Program Header Offset
    uint32_t e_shoff;       // Section Header Offset
    uint32_t e_flags;       // Flags
    uint16_t e_ehsize;      // Size of ELF
    uint16_t e_phentsize;   // Size of Program Header Entry
    uint16_t e_phnum;       // No. of Entries
    uint16_t e_shentsize;   // Size of Section Header Entry
    uint16_t e_shnum;       // No. of Entires
    uint16_t e_shstrndx;    // Section Name String Table Index
} __attribute__((packed)) Elf32_Ehdr;

// 32 Bytes per Entry, start at file + e_phoff
typedef struct {
    uint32_t p_type;        // Segment type
    uint32_t p_offset;      // Byte Offset
    uint32_t p_vaddr;       // Virtual Address in UserSpace
    uint32_t p_paddr;       // Physical Address (ignored for UserMode)
    uint32_t p_filesz;      // No. of Bytes to Copy from ELF to Memory
    uint32_t p_memsz;       // No. of Bytes this segment occupies
    uint32_t p_flags;       // Permissions
    uint32_t p_align;       // Alignment
} __attribute__((packed)) Elf32_Phdr;

bool elf_validate(const void* image, uint32_t size);