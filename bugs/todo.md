After fixing the current bugs, proceed in this order:

1. **Stabilize and verify**
   - Add regression tests for every fixed bug.
   - Run the kernel under QEMU with assertions and logging.
   - Test allocation failures, invalid addresses, task exhaustion, and large inputs.

2. **Improve memory management**
   - Add proper physical-frame ownership/reference counting.
   - Implement page fault diagnostics.
   - Add virtual memory allocation helpers.
   - Separate kernel and user address-space policies.

3. **Build a reliable process model**
   - Define task creation and cleanup contracts.
   - Add an idle task and robust scheduler behavior.
   - Implement process termination and exit statuses.
   - Preserve complete CPU state during context switches.

4. **Create a real user-program loader**
   - Replace the raw `user_test` copy with an ELF loader.
   - Allocate multiple code/data/stack pages.
   - Validate ELF headers and segment permissions.
   - Map user memory through safe kernel mappings.

5. **Add core kernel services**
   - System call argument validation.
   - File system and block-device support.
   - Timer-based sleeping and synchronization primitives.
   - Better keyboard/input abstractions.

6. **Improve engineering quality**
   - Add automatic dependency tracking to the build.
   - Add build-time kernel-size checks.
   - Use consistent error handling and ownership rules.
   - Document invariants for PMM, paging, scheduler, and syscalls.

A good immediate milestone is:

> **Run multiple user processes safely, with reliable allocation, cleanup, scheduling, and page-fault handling.**
