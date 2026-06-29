# Kernel Architecture & Design

This document serves as a reference for the physical memory layout, hardware communication, and core design decisions of the operating system.

## 1. Boot Sequence
1. **BIOS Initialization:** The system powers on, and the BIOS runs POST.
2. **Bootloader (16-bit):** The BIOS loads our custom bootloader into memory at `0x7C00`.
3. **Kernel Loading:** The bootloader uses BIOS Interrupt `0x13` with a Disk Address Packet (LBA) to load the kernel from the disk into memory at `0x8000`.
4. **Protected Mode:** The bootloader disables interrupts (`cli`), loads the GDT, flips the `cr0` register to enter 32-bit Protected Mode, and jumps to `0x8000`.
5. **Kernel Entry:** `kernel_entry.asm` hands execution over to our C code (`kmain`).

## 2. Physical Memory Map
Because we are working in a bare-metal environment, it is critical not to overwrite reserved memory.

| Address | Size/End | Description |
| :--- | :--- | :--- |
| `0x00000` | `0x003FF` | BIOS Interrupt Vector Table (IVT) |
| `0x00400` | `0x004FF` | BIOS Data Area (BDA) |
| `0x05000` | - | Memory Map data (populated via BIOS `E820`) |
| `0x07C00` | `0x07DFF` | Custom Bootloader (512 bytes) |
| `0x08000` | - | Kernel Code & Data |
| `0x90000` | - | Interrupt Descriptor Table (IDT) |
| `0xA0000` | - | Stack Base (Grows downwards) |
| `0xB8000` | `0xBFFFF` | VGA Text Mode Buffer |

## 3. Interrupts & IRQ Routing
To handle hardware devices, the 8259 Programmable Interrupt Controller (PIC) is remapped. By default, hardware IRQs collide with CPU exceptions (0-31). We remap them so hardware interrupts start at ISR 32.

* **CPU Exceptions:** ISR 0 - 31 (e.g., ISR 0 = Divide by Zero, ISR 14 = Page Fault)
* **Master PIC (IRQ 0-7):** Remapped to ISR 32 - 39
* **Slave PIC (IRQ 8-15):** Remapped to ISR 40 - 47

**Active Handlers:**
* `IRQ 0` (`ISR 32`): PIT (Timer running at 100Hz)
* `IRQ 1` (`ISR 33`): PS/2 Keyboard

## 4. Hardware I/O Ports
Communication with hardware devices occurs via `inb` and `outb` on the following ports:
* `0x20`, `0x21`: Master PIC Command/Data
* `0xA0`, `0xA1`: Slave PIC Command/Data
* `0x40`, `0x43`: PIT Data Channel 0 / Command Register
* `0x60`: PS/2 Keyboard Data Port
* `0x3D4`, `0x3D5`: VGA CRT Control (Blinking Cursor)

## 5. Physical Memory Manager
Note: This OS utilizes a **Bitmap Allocator** for managing physical RAM frames
Using a bitmap is highly efficient for early kernel development. Every 4KB frame of memory is represented by a single bit. If a bit is `1`, the frame is in use; if `0`, it is free. The memory bounds are dynamically detected using BIOS interrupt `0x15, eax=0xE820` during the boot phase.