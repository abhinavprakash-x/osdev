all:
	nasm -f bin src/bootloader.asm -o build/bootloader.bin
	nasm -f bin src/shell.asm -o build/shell.bin
	cat build/bootloader.bin build/shell.bin > build/boot.img
	qemu-system-i386 -drive format=raw,file=build/boot.img