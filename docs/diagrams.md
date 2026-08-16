## Architecture

Bare Minimum OS currently uses a monolithic kernel design. All drivers,
kernel threads, the scheduler and the shell execute in Ring 0.

```
    BIOS
      │
      ▼
Custom Bootloader
      │
      ├── Load Kernel
      ├── E820 Memory Map
      ├── Enable A20
      └── Enter Protected Mode
              │
              ▼
           Kernel
              │
     ┌────────┼─────────┐
     ▼        ▼         ▼
 Interrupts  Memory   Drivers
 IDT/ISR     PMM      VGA
 PIC/PIT     Paging   Keyboard
             Heap
     │        │         │
     └────────┼─────────┘
              ▼
          Scheduler
              │
        Kernel Threads
              │
              ▼
            Shell
```

