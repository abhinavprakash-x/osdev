# Multitasking

Bare Minimum OS implements preemptive multitasking using kernel threads.

All tasks currently execute in Ring 0 and share the kernel address
space. User-space processes and separate process address spaces have
not yet been implemented.

## Overview

The Programmable Interval Timer (PIT) generates periodic interrupts at
100 Hz, producing one timer tick every 10 ms.

The scheduler uses a time slice of 5 timer ticks. On every PIT interrupt,
the current time slice is incremented:

```c
time_slice++;

if (time_slice >= 5) {
    time_slice = 0;
    schedule();
}
```

Therefore, under normal preemption, the scheduler is invoked every
5 ticks, giving each running task a time slice of approximately 50 ms.

```
PIT @ 100 Hz
     |
     | IRQ every 10 ms
     v
Timer Handler
     |
     +-- tick 1
     +-- tick 2
     +-- tick 3
     +-- tick 4
     |
     +-- tick 5
           |
           v
       schedule()
           |
           v
 Select next READY task
           |
           v
    Context Switch
```

This makes scheduling preemptive: a task does not need to voluntarily
give up the CPU for another task to run.

## Tasks

Each task is represented internally by a task control structure.

A task stores the information required by the scheduler to manage and
resume its execution, including its execution context, stack,
identifier, name and scheduling state.

The kernel currently creates kernel threads rather than isolated
user-space processes.

## Task States

Tasks can move between several scheduling states.

The important transitions are:

```text
             +-------+
             | READY |
             +---+---+
                 |
             scheduled
                 |
                 v
            +---------+
            | RUNNING |
            +----+----+
                 |
        +--------+---------+
        |                  |
     sleep()              exit
        |                  |
        v                  v
   +---------+        TERMINATED
   | WAITING |
   +----+----+
        |
    wake time
     reached
        |
        v
      READY
```

`READY` tasks are eligible to execute.

`RUNNING` represents the task currently executing on the CPU.

`WAITING` tasks are temporarily excluded from scheduling until their
wake condition is reached.

Finished tasks are removed from scheduling and their resources are
cleaned up.

## Context Switching

When the scheduler switches tasks, the execution state of the current
task must be preserved so that it can later continue from the same
point.

Conceptually:

```text
Task A running
      |
      v
Save Task A context
      |
      v
Select Task B
      |
      v
Restore Task B context
      |
      v
Task B continues
```

The low-level context switch is implemented in assembly while the
scheduler and task management logic are implemented in C.

## Sleeping and Waking

A task can call `task_sleep()` when it does not need to execute for a
period of time.

Instead of busy-waiting, the task enters the `WAITING` state and the
scheduler can execute another task.

```text
Task A
  |
task_sleep()
  |
  v
WAITING --------------------+
                            |
                        PIT ticks
                            |
                      wake time reached
                            |
                            v
                          READY
```

This allows CPU time to be used by other runnable tasks while a task is
waiting.

## Task Termination

Tasks may finish execution and leave the scheduler.

When a task exits, it is no longer considered runnable and its allocated
resources can be reclaimed.

This prevents completed tasks from remaining indefinitely in the task
list.

## Shell Integration

The shell itself runs as a scheduled kernel task.

The `tasks` command can be used to inspect the current task list:

```text
OS> tasks

PID NAME    STATE
--------------------
1   shell   RUNNING
0   kmain   READY
```

This provides a simple way to inspect scheduler state while the OS is
running.

## Testing

The kernel test suite contains an integration test for the scheduler.

The test creates temporary tasks and verifies that they are successfully
scheduled and executed.

Example:

```text
[PASS] Create scheduler task A
[PASS] Create scheduler task B
[PASS] Scheduler executed task A
[PASS] Scheduler executed task B
```

Because these tests require actual task execution, they exercise task
creation, scheduling, context switching, sleeping/waking and task
termination together.

## Current Limitations

The multitasking system currently operates entirely in kernel mode.

All tasks:

- Execute in Ring 0
- Share the kernel address space
- Have full access to kernel memory and hardware

The next major step is introducing Ring 3 execution, a Task State
Segment and system calls. This will establish a privilege boundary
between applications and the kernel.