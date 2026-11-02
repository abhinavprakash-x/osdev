#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "task/task.h"

// ELF Identification Constants
#define ELFMAG0     0x7Fu
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'
#define ELFCLASS32  1u // 32-bit architecture
#define ELFDATA2LSB 1u // Little-endian

// ELF File & Machine Types
#define ET_EXEC     2u // Executable file
#define EM_386      3u // Intel 80386 (x86)

// Program Header Segment Types
#define PT_NULL     0u
#define PT_LOAD     1u

// Program Header Permission Flags
#define PF_X        0x1u // Execute
#define PF_W        0x2u // Write
#define PF_R        0x4u // Read

// 52 Bytes at Byte 0 of File
typedef struct {
    uint8_t  e_ident[16];   // Magic and ABI info
    uint16_t e_type;        // File Type
    uint16_t e_machine;     // CPU Arch
    uint32_t e_version;     // ELF Version
    uint32_t e_entry;       // Virtual EIP entry point
    uint32_t e_phoff;       // Program Header Offset
    uint32_t e_shoff;       // Section Header Offset
    uint32_t e_flags;       // Processor-specific Flags
    uint16_t e_ehsize;      // Size of ELF Header (52)
    uint16_t e_phentsize;   // Size of Program Header Entry (32)
    uint16_t e_phnum;       // Number of Program Header Entries
    uint16_t e_shentsize;   // Size of Section Header Entry
    uint16_t e_shnum;       // Number of Section Header Entries
    uint16_t e_shstrndx;    // Section Name String Table Index
} __attribute__((packed)) Elf32_Ehdr;

// 32 Bytes per Entry, starting at file + e_phoff
typedef struct {
    uint32_t p_type;        // Segment type (PT_LOAD = 1)
    uint32_t p_offset;      // Byte offset in file
    uint32_t p_vaddr;       // Virtual address in user space
    uint32_t p_paddr;       // Physical address (ignored)
    uint32_t p_filesz;      // Bytes to copy from file
    uint32_t p_memsz;       // Bytes occupied in memory (>= p_filesz)
    uint32_t p_flags;       // Permissions (PF_R, PF_W, PF_X)
    uint32_t p_align;       // Alignment
} __attribute__((packed)) Elf32_Phdr;

bool elf_validate(const void* image, uint32_t size);
task_t* create_user_task_elf(const char* name, const void* image, uint32_t size);