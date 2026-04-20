all:
	nasm -f bin src/bootloader.asm -o build/bootloader.bin
	qemu-system-i386 -drive format=raw,file=build/bootloader.bin