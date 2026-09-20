# ============================================================
# Toolchain
# ============================================================

ASM   ?= nasm
CROSS ?= ./cross/bin/i686-elf-

CC      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
QEMU    ?= qemu-system-i386


# ============================================================
# Directories
# ============================================================

SRC_DIR     := src
INCLUDE_DIR := include
BUILD_DIR   := build

BOOTLOADER  := $(SRC_DIR)/bootloader.asm
LINKER      := linker.ld

BOOT_BIN    := $(BUILD_DIR)/bootloader.bin
KERNEL_BIN  := $(BUILD_DIR)/kernel.bin
BOOT_IMAGE  := $(BUILD_DIR)/boot.img


# ============================================================
# Compiler / Assembler / Linker flags
# ============================================================

# CFLAGS
#
# -m32                         Build 32-bit code
# -ffreestanding               Build for a freestanding environment
# -std=gnu11                   Use GNU C11
# -fno-pic                     No position-independent code
# -fno-pie                     No position-independent executable
# -fno-stack-protector         Kernel has no normal userspace SSP runtime
# -fno-builtin                 Use our own libc functions rather than
#                              compiler builtins where appropriate
#
# Warnings:
# -Wall                        Basic warnings
# -Wextra                      Extra useful warnings
# -Wshadow                     Warn about variable shadowing
# -Wundef                      Warn about undefined macros in #if
#
# Errors:
# -Werror=implicit-function-declaration
#                              Missing function declarations are errors
# -Werror=return-type          Missing return statements are errors
#
# Dependencies:
# -MMD -MP                    Generate .d dependency files
#
CFLAGS := \
	-m32 \
	-ffreestanding \
	-std=gnu11 \
	-fno-pic \
	-fno-pie \
	-fno-stack-protector \
	-fno-builtin \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wundef \
	-Werror=implicit-function-declaration \
	-Werror=return-type \
	-MMD \
	-MP \
	-I$(INCLUDE_DIR)

# NASM flags for normal kernel assembly
ASFLAGS := -f elf32

# NASM flags for the bootloader
BOOT_ASFLAGS := -f bin

# Linker flags
LDFLAGS := \
	-m elf_i386 \
	-T $(LINKER) \
	--oformat binary


# ============================================================
# Source discovery
# ============================================================

# Find all C files recursively under src/
C_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.c' | sort)

# Find all assembly files recursively under src/
ASM_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.asm' | sort)

# The bootloader is assembled directly into a raw binary.
# Everything else is assembled into ELF32 object files.
ASM_KERNEL := $(filter-out $(BOOTLOADER),$(ASM_SOURCES))


# ============================================================
# Object mapping
# ============================================================

# Example:
#
# src/mm/pmm.c
#      ↓
# build/mm/pmm.o
#
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# Example:
#
# src/task/switch.asm
#      ↓
# build/task/switch.o
#
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_KERNEL))

OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

# Automatically generated dependency files from GCC
DEP_FILES := $(C_OBJECTS:.o=.d)


# ============================================================
# Default target
# ============================================================

.PHONY: all
all: $(BOOT_IMAGE)


# ============================================================
# Bootloader
# ============================================================

$(BOOT_BIN): $(BOOTLOADER)
	@mkdir -p $(dir $@)
	$(ASM) $(BOOT_ASFLAGS) $< -o $@


# ============================================================
# C compilation
# ============================================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# ============================================================
# Kernel assembly
# ============================================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASFLAGS) $< -o $@


# ============================================================
# Kernel linking
# ============================================================

$(KERNEL_BIN): $(OBJECTS) $(LINKER)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)


# ============================================================
# Final boot image
# ============================================================

$(BOOT_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@mkdir -p $(dir $@)
	cat $^ > $@
	dd if=/dev/zero bs=512 count=100 >> $@


# ============================================================
# Run
# ============================================================

.PHONY: run
run: all
	$(QEMU) -drive format=raw,file=$(BOOT_IMAGE)


# ============================================================
# Debug
# ============================================================

.PHONY: debug
debug: all
	$(QEMU) \
		-drive format=raw,file=$(BOOT_IMAGE) \
		-d int \
		-D qemu-int.log


# ============================================================
# Build check
# ============================================================

# Compile everything without building the final image.
# Useful for quickly checking whether source/header changes
# compile correctly.

.PHONY: check
check: $(OBJECTS)
	@echo "Build check passed."


# ============================================================
# Print build information
# ============================================================

.PHONY: print
print:
	@echo "CC          = $(CC)"
	@echo "ASM         = $(ASM)"
	@echo "LD          = $(LD)"
	@echo "OBJCOPY     = $(OBJCOPY)"
	@echo "QEMU        = $(QEMU)"
	@echo
	@echo "SRC_DIR     = $(SRC_DIR)"
	@echo "INCLUDE_DIR = $(INCLUDE_DIR)"
	@echo "BUILD_DIR   = $(BUILD_DIR)"
	@echo
	@echo "C_SOURCES  ="
	@$(foreach f,$(C_SOURCES),echo "  $(f)";)
	@echo
	@echo "ASM_SOURCES ="
	@$(foreach f,$(ASM_SOURCES),echo "  $(f);)
	@echo
	@echo "OBJECTS    ="
	@$(foreach f,$(OBJECTS),echo "  $(f);)


# ============================================================
# Clean
# ============================================================

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)


# ============================================================
# Rebuild
# ============================================================

.PHONY: rebuild
rebuild: clean all


# ============================================================
# Help
# ============================================================

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make          Build boot image"
	@echo "  make all      Build boot image"
	@echo "  make check    Compile all source files"
	@echo "  make run      Build and run in QEMU"
	@echo "  make debug    Build and run QEMU with interrupt logging"
	@echo "  make print    Print discovered sources and objects"
	@echo "  make clean    Remove build directory"
	@echo "  make rebuild  Clean and rebuild everything"
	@echo "  make help     Show this help"


# ============================================================
# Include generated dependency files
# ============================================================

-include $(DEP_FILES)