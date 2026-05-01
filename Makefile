all:
	mkdir -p build
# Bootloader
	nasm -f bin src/bootloader.asm -o build/bootloader.bin
# Kernel entry (object file)
	nasm -f elf32 src/kernel_entry.asm -o build/kernel_entry.o
# C kernel (NO PIC!)
	gcc -m32 -ffreestanding -fno-pic -c src/kernel.c -o build/kernel.o
# Link kernel
	ld -m elf_i386 -Ttext 0x8000 -e _start \
	build/kernel_entry.o build/kernel.o \
	-o build/kernel.bin --oformat binary
# Create image
	cat build/bootloader.bin build/kernel.bin > build/boot.img
# Run
	qemu-system-i386 -drive format=raw,file=build/boot.img

clean:
	rm -rf build