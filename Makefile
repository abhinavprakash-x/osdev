BUILD_DIR = build
SRC_DIR = src

C_SOURCES = $(SRC_DIR)/kernel.c \
            $(SRC_DIR)/drivers/vga.c

C_OBJECTS = $(C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

ASM_OBJECTS = $(BUILD_DIR)/kernel_entry.o

all: $(BUILD_DIR)/boot.img

# ----------------------------
# Bootloader (flat binary)
# ----------------------------

$(BUILD_DIR)/bootloader.bin: $(SRC_DIR)/bootloader.asm
	mkdir -p $(BUILD_DIR)
	nasm -f bin $< -o $@

# ----------------------------
# Kernel entry (ASM -> object)
# ----------------------------

$(BUILD_DIR)/kernel_entry.o: $(SRC_DIR)/kernel_entry.asm
	mkdir -p $(BUILD_DIR)
	nasm -f elf32 $< -o $@

# ----------------------------
# C files -> object files
# ----------------------------

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	gcc -m32 -ffreestanding -fno-pic -c $< -o $@

# ----------------------------
# Link kernel
# ----------------------------

$(BUILD_DIR)/kernel.bin: $(ASM_OBJECTS) $(C_OBJECTS) linker.ld
	ld -m elf_i386 -T linker.ld -o $@ $(ASM_OBJECTS) $(C_OBJECTS) --oformat binary

# ----------------------------
# Final disk image
# ----------------------------

$(BUILD_DIR)/boot.img: $(BUILD_DIR)/bootloader.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $@

# ----------------------------
# Run in QEMU
# ----------------------------

run: all
	qemu-system-i386 -drive format=raw,file=$(BUILD_DIR)/boot.img

# ----------------------------
# Clean
# ----------------------------

clean:
	rm -rf $(BUILD_DIR)