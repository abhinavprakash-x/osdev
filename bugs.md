## High-severity bugs

1. **User tasks start with an unmapped address space**  
   `create_user_task` creates a page directory but maps neither the user code nor user stack. Unless the caller maps them first, the task immediately page-faults. The API contract is unsafe and incomplete.

2. **User-test allocation failures leak resources**  
   `launch_user_test` frees only the code page when stack allocation fails. The task PCB, kernel stack, and page directory remain allocated.

3. **User-test mapping failures are ignored**  
   `launch_user_test` ignores both `paging_map_page()` return values, potentially scheduling a task with unmapped code or stack.

4. **Scheduler can spin forever when all tasks are waiting**  
   `schedule` loops indefinitely if no task is runnable. When invoked from an interrupt, interrupts are disabled, so timer ticks cannot advance and sleeping tasks never wake.

5. **Null task insertion causes a kernel crash**  
   `task_add` does not validate `new_task`. An allocation failure passed from `kmain` will dereference null at line 43.

6. **E820 map processing can corrupt memory**  
   `pmm_init` does not clamp E820 ranges to the 4-GB bitmap limit. High-memory or overflowing entries can call `clear_bit()` out of bounds.

7. **Reserved physical frames can be released and reallocated**  
   `pmm_free_block` checks only whether a bit is set, not whether the frame belongs to a usable E820 region. Passing a reserved address above 1 MB makes hardware/kernel memory available for reuse.

## Medium-severity bugs

8. **Kernel image size is not checked against the bootloader read limit**  
   The bootloader always reads 100 sectors (`bootloader.asm`), while the image builder merely appends 100 sectors (`Makefile`). A kernel larger than 50 KiB is silently truncated.

9. **User code is copied into one page without a size check**  
   `launch_user_test` allocates one physical page, then copies `user_code_size` bytes. If the assembled test exceeds 4096 bytes, this overruns the allocated frame.

10. **Sleep duration arithmetic overflows**  
    `task_sleep` multiplies milliseconds by timer frequency in 32-bit arithmetic. Large durations wrap and produce incorrect wake times.

11. **Keyboard input silently overwrites unread input**  
    `keyboard_handler` advances `buffer_head` without checking whether the circular buffer is full, losing keystrokes and corrupting queue ordering.

12. **Physical-memory accounting is inconsistent with the E820 map**  
    `pmm_init` counts all usable frames in `total_blocks`, then blindly adds 256 reserved frames to `used_blocks`, even when those frames were not usable. `meminfo` can report incorrect or negative free memory.

13. **Shell memory commands can crash the kernel**  
    `peek/poke handling` dereferences arbitrary addresses without checking mapping, alignment, or permissions. Invalid user input causes a kernel page fault or memory corruption.






## Critical bugs

1. **`.bss` is never zero-initialized**  
   `kernel_entry.asm` jumps directly to `kmain`, while the flat binary linker configuration does not emit `.bss` contents. Globals such as `tick_count`, keyboard state, and buffers can start with garbage values.

2. **Scheduler hangs when every task is waiting**  
   `schedule` loops forever if no task is runnable. When called with interrupts disabled, timer ticks cannot advance, so sleeping tasks never wake.

3. **Dead-task cleanup can use freed memory**  
   `schedule` can eventually unlink and free `current_task`, then continue using it as `prev_iterator` when multiple dead tasks remain.

4. **`scheduler_init()` dereferences failed allocation**  
   `scheduler_init` does not check whether `kmalloc()` returned `NULL`.

5. **`task_add()` accepts `NULL` tasks**  
   `task_add` dereferences `new_task` without validation. The test suite asserts allocation success but continues execution even after failure.

6. **Existing user address spaces miss later kernel mappings**  
   `paging_create_address_space` copies kernel PDEs only once. Later heap growth or other kernel mappings are absent from older address spaces, causing faults while kernel code runs under those page directories.

7. **PMM can write beyond its bitmap**  
   `pmm_init` converts 64-bit E820 ranges to bitmap indexes without clipping them to the 4-GB bitmap limit.

8. **Reserved E820 memory above 1 MB can be freed**  
   `pmm_free_block` only protects the first 1 MB. Reserved or MMIO frames above 1 MB can be incorrectly returned to the allocator.

9. **User-code copy can overflow one physical page**  
   `launch_user_test` allocates one page but copies `user_code_size` bytes without checking that the code fits.

10. **User-test mapping failures are ignored**  
    `launch_user_test` ignores both `paging_map_page()` results and schedules the task even if code or stack mapping failed.

11. **User-test failure path leaks the created task**  
    `launch_user_test` frees only the code page when stack allocation fails. The PCB, kernel stack, and page directory leak.

12. **Physical user frames are accessed as virtual addresses**  
    `launch_user_test` passes a physical address directly to `memcpy()`. This only works while the frame is identity-mapped.

13. **Heap allocation arithmetic can overflow**  
    `kmalloc` can wrap during alignment and page-count calculations. Requests such as `alloc -1` can cause invalid sizes, recursive allocation, or heap exhaustion.

14. **Shell exposes the heap overflow directly**  
    `alloc command` passes unchecked signed `atoi()` results into `kmalloc()`.

15. **Arbitrary shell memory access can crash or corrupt the kernel**  
    `peek and poke` dereference user-provided addresses without validation.

## Significant correctness bugs

16. **`paging_map_page()` overwrites existing mappings and leaks the old frame**  
    `map_page` assigns a new PTE without checking whether one is already present.

17. **Page-table aliases are not invalidated after reclamation**  
    `unmap_page` invalidates only the unmapped virtual page, not the recursive page-table alias.

18. **Paging APIs do not enforce page alignment**  
    `map_page` accepts unaligned virtual and physical addresses.

19. **Paging APIs do not validate physical-frame ownership or validity**  
    `paging_map_page` accepts arbitrary physical addresses, including reserved or unallocated frames.

20. **Address-space destruction assumes every user mapping owns its frame**  
    `paging_destroy_address_space` frees every mapped physical page, which breaks if frames are shared or aliased.

21. **E820 ranges are rounded incorrectly**  
    `pmm_init` floors both start and length. Partial pages can be incorrectly treated as usable.

22. **E820 extended attributes and entry validity are ignored**  
    `mem_map_start` and `pmm_init` do not validate returned signatures, entry sizes, or disabled-region attributes.

23. **E820 buffer has no capacity limit**  
    `mem_map_start` keeps incrementing `DI` without preventing the map from overwriting other memory.

24. **PMM memory accounting can be incorrect or negative**  
    `pmm_init` counts usable frames but independently adds 256 reserved frames to `used_blocks`, even when those frames were not counted as usable.

25. **Scheduler sleep arithmetic and tick comparisons overflow**  
    `task_sleep` can overflow during millisecond conversion and wake-time addition. `schedule` does not handle tick-counter wraparound.

26. **User `FS`, `GS`, and other segment registers are corrupted on interrupts**  
    `isr_common_stub` saves only `DS` but restores all data segment registers from it.

27. **`kfree()` does not validate heap ownership or allocation state**  
    `kfree` accepts arbitrary pointers, stale pointers, and already-free blocks if the preceding memory happens to contain the magic value.

28. **Heap growth has no reserved virtual-address ceiling**  
    `kmalloc` can grow into regions intended for user mappings or other special mappings.

29. **Keyboard ring buffer overwrites unread input**  
    `keyboard_handler` advances the producer index without checking whether the buffer is full.

30. **Extended PS/2 scancodes are mishandled**  
    `keyboard_handler` does not handle `0xE0` prefixes, so arrow and other extended keys are misinterpreted.

31. **Calculator can trigger signed division overflow**  
    `calc command` checks division by zero but not `INT_MIN / -1`, which can raise `#DE`.

32. **`time` command hard-codes 100 Hz**  
    `time command` ignores the configurable PIT frequency.

33. **Bootloader silently truncates kernels larger than 100 sectors**  
    `bootloader.asm` always reads 100 sectors, while `Makefile` has no size assertion.

34. **A20 enable failure is ignored**  
    `bootloader.asm` does not check the BIOS call’s carry flag or provide a fallback.

35. **PMM bitmap shifts use signed `1`**  
    `set_bit` and related functions perform `1 << 31`, which is undefined behavior in C.

## Build and test defects

36. **Header changes do not reliably trigger recompilation**  
    `Makefile` does not generate dependency files.

37. **Scheduler test is timing-dependent**  
    `test_scheduler` assumes a fixed PIT rate and exact scheduling timing.

38. **Page-table reclamation test dereferences a reclaimed recursive mapping**  
    `test_page_table_reclamation` accesses `page_table` after the final page-table mapping may have been removed.

39. **Assembly boundary pointer subtraction is not a valid C object relationship**  
    `launch_user_test` subtracts independently declared assembly symbols as if they belonged to one C array.

40. **PID allocation has no wraparound policy**  
    `create_task` eventually wraps `next_pid` to zero, colliding with the kernel task PID.

## Findings from the attached list that are not independent current bugs

I did not include these as confirmed defects:

- VGA interrupt “reentrancy” on a uniprocessor: interrupt gates clear IF, so normal shell code cannot run concurrently during the keyboard ISR.
- Missing `volatile` in the custom memory routines: undesirable for portability/MMIO, but not a demonstrated failure on this x86 target.
- Identity mapping of the first 4 MiB: intentional bootstrap behavior, although coarse.
- Missing `-fno-stack-protector` and orphan-section handling: build hardening improvements, not confirmed runtime bugs.
- `paging_destroy_address_space()` temporarily switching back into the directory being destroyed: poor design, but not independently incorrect given the current mappings.
