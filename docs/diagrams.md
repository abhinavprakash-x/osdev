# System Diagrams

This file collects the main execution paths of the current Bare Minimum OS implementation. The diagrams use Mermaid so they can be rendered directly by GitHub and other Markdown viewers that support Mermaid.

## 1. Boot Flow

```mermaid
flowchart TD
    BIOS --> B[bootloader.asm]
    B --> D[BIOS disk read: 100 sectors]
    B --> E[BIOS E820 memory map]
    E --> M[Store map at 0x5000 / 0x5004]
    B --> A20[Enable A20 via INT 0x15 AX=0x2401]
    B --> G[Load bootloader GDT]
    G --> P[Enable protected mode]
    P --> J[Jump to 0x08:0x8000]
    J --> KE[kernel_entry.asm]
    KE --> Z[Zero .bss]
    Z --> K[kmain]
```

## 2. Kernel Initialization

```mermaid
flowchart TD
    K[kmain] --> GDT[gdt_init]
    GDT --> TSS[tss_init]
    TSS --> IDT[idt_init]
    IDT --> PIC[pic_init]
    PIC --> PIT[pit_init(100)]
    PIT --> PMM[pmm_init]
    PMM --> PAG[paging_init]
    PAG --> HEAP[heap_init]
    HEAP --> SCH[scheduler_init]
    SCH --> SHELL[create shell task]
    SHELL --> ST[sti]
    ST --> HLT[kmain hlt loop]
```

## 3. Memory Management Layers

```mermaid
flowchart BT
    RAM[Physical RAM]
    PMM[Physical Memory Manager]
    PAG[Paging]
    HEAP[Kernel Heap]
    K[Kernel / Tasks / Syscalls]

    RAM --> PMM
    PMM --> PAG
    PAG --> HEAP
    HEAP --> K
```

## 4. Physical Frame Allocation

```mermaid
flowchart TD
    S[pmm_alloc_block] --> X[Start from last_searched_idx]
    X --> W[Check bitmap word]
    W -->|Full| N[Next word]
    N --> W
    W -->|Free bit found| C[Set bit]
    C --> U[used_blocks++]
    U --> R[Return physical frame address]
```

## 5. Virtual Address Translation

```mermaid
flowchart LR
    VA[32-bit Virtual Address]
    VA --> PDE[Page Directory Index: bits 31-22]
    VA --> PTE[Page Table Index: bits 21-12]
    VA --> OFF[Offset: bits 11-0]
    PDE --> DIR[Page Directory Entry]
    PTE --> TAB[Page Table Entry]
    DIR --> TAB
    TAB --> FRAME[Physical Frame]
    FRAME --> PA[Physical Address + Offset]
    OFF --> PA
```

## 6. Recursive Paging

```mermaid
flowchart TD
    DIR[Current Page Directory]
    DIR --> T1[Page Table]
    DIR --> T2[Page Table]
    DIR --> TN[Other Page Tables]
    DIR --> R[Entry 1023]
    R --> DIR

    VPD[0xFFFFF000] --> DIR
    VPT[0xFFC00000 + PDE index * 0x1000] --> T2
```

## 7. Heap Allocation

```mermaid
flowchart TD
    A[kmalloc(size)] --> B[8-byte alignment]
    B --> C{Free block fits?}
    C -->|Yes| D[Optional split]
    D --> E[Mark block used]
    E --> R[Return payload]
    C -->|No| F[Allocate required PMM frames]
    F --> G[Map pages at heap_end_vaddr]
    G --> H{Allocation succeeded?}
    H -->|No| RB[Rollback pages / frames]
    H -->|Yes| I[Create new free block]
    I --> B
```

## 8. Scheduler

```mermaid
flowchart TD
    IRQ[PIT IRQ0] --> T[tick_count++]
    T --> TS[time_slice++]
    TS --> C{time_slice >= 5?}
    C -->|No| EOI[EOI]
    C -->|Yes| S[schedule]
    S --> W[Inspect circular task list]
    W --> WAIT{Task WAITING?}
    WAIT -->|wake_time reached| READY[Make READY]
    WAIT -->|still waiting| NEXT[Continue scan]
    READY --> PICK[Select runnable task]
    NEXT --> PICK
    PICK --> CR3[Switch task page directory]
    CR3 --> ESP[Switch saved ESP]
    ESP --> RUN[Selected task RUNNING]
    RUN --> EOI
```

## 9. Task Sleeping

```mermaid
stateDiagram-v2
    [*] --> RUNNING
    RUNNING --> WAITING: task_sleep(ms)
    WAITING --> READY: get_ticks() >= wake_time
    READY --> RUNNING: scheduler selects task
```

## 10. Kernel/User Transition

```mermaid
sequenceDiagram
    participant K as Kernel
    participant EU as enter_usermode
    participant U as Ring 3 User Task

    K->>EU: user_eip, user_esp
    EU->>EU: load selector 0x23
    EU->>EU: build iret frame
    EU->>U: iretd / Ring 3
    U->>K: int 0x80
    K->>K: isr128 -> isr_handler -> syscall_handler
    K->>U: iret
```

## 11. Syscall Dispatch

```mermaid
flowchart TD
    INT[int 0x80] --> ISR[ISR 128]
    ISR --> DISP[isr_handler]
    DISP --> SYS[syscall_handler]
    SYS --> TEST[SYS_TEST]
    SYS --> WRITE[SYS_WRITE]
    SYS --> EXIT[SYS_EXIT]
    SYS --> YIELD[SYS_YIELD]
    SYS --> SLEEP[SYS_SLEEP]
    SYS --> PID[SYS_GETPID]
```

## 12. User Address Validation

```mermaid
flowchart TD
    A[user_range_valid(addr, size)] --> Z{size == 0?}
    Z -->|Yes| OK[Valid]
    Z -->|No| END[Calculate end]
    END --> WRAP{32-bit wraparound?}
    WRAP -->|Yes| BAD[Invalid]
    WRAP -->|No| LIM{end <= USER_SPACE_END?}
    LIM -->|No| BAD
    LIM -->|Yes| P[Check every page]
    P --> PDE{PDE present + USER?}
    PDE -->|No| BAD
    PDE -->|Yes| PTE{PTE present + USER?}
    PTE -->|No| BAD
    PTE -->|Yes| MORE{More pages?}
    MORE -->|Yes| P
    MORE -->|No| OK
```
