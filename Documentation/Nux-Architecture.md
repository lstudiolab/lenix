# Nux Architecture: Initial Design

## 1. Goals

Nux is a small, capability-oriented microkernel for Lenix. It targets AArch64 and x86-64. It aims for strong isolation, explicit IPC, predictable privileged code, and a narrow hardware abstraction boundary.

The design borrows concepts, not source code, from Mach-style microkernels. No Linux, BSD, Mach, or Apple XNU kernel implementation is included.

## 2. Layering

```text
Applications and system services (user mode)
        |
        | capability-checked IPC
        v
User-space servers: filesystem, network, device services, session services
        |
        | kernel calls / IPC
        v
+-----------------------------------------------------+
| Nux                                                |
| Tasks | Threads | Scheduler | IPC | Ports           |
| Capabilities | VM | Memory | Timers | Synchronization|
+-----------------------------------------------------+
        |
        | narrow architecture interface
        v
+----------------------+  +---------------------------+
| AArch64 assembly/C   |  | x86-64 assembly/C         |
| vectors, traps, MMU, |  | IDT, traps, CRx, TLB,      |
| context, timers      |  | context, timers           |
+----------------------+  +---------------------------+
        |                          |
        +------ CPU / platform hardware -----------+
```

## 3. Why assembly is primary only at the hardware boundary

Assembly is mandatory where the compiler cannot express the required privileged transition or exact register/stack contract. It is the first implementation layer for boot entry, exception vectors, interrupt stubs, context switching, syscall entry, and architecture-specific MMU/control-register operations.

The majority of kernel policy should remain in C. Scheduler policy, IPC queue management, object lifetime, capability validation, and VM metadata are complex stateful algorithms. Implementing all of them in assembly would increase defect risk, make invariants harder to review, and duplicate logic across two instruction sets without improving hardware control.

The intended rule is: **assembly owns machine state transitions; C owns portable kernel mechanisms and policy.**

## 4. Core object relationships

- A task owns an address space, a capability table, and a set of threads.
- A thread belongs to exactly one task and has one architecture context while runnable or suspended.
- A port is a kernel object with a receive queue and explicit send/receive rights.
- A capability is a task-local reference to an object plus a bounded rights set.
- An IPC message has a versioned header, bounded inline payload, optional out-of-line memory, and optional transferred capabilities.
- Memory objects provide backing storage that can be mapped into authorized task address spaces.

Object IDs exposed across trust boundaries are opaque. Kernel pointers are never accepted as user-controlled object identifiers.

## 5. IPC model

The initial model supports synchronous request/reply through correlation IDs and asynchronous message send/receive. Send and receive are separately authorized operations. Rights transfer is explicit and must be validated and committed atomically with message enqueue/dequeue semantics.

The initial inline payload limit is 256 bytes. Larger payloads use memory objects rather than unbounded kernel message buffers. Deadlines are absolute monotonic deadlines; timeout behavior must be consistent across both architecture ports.

## 6. Scheduling model

Nux will use preemptive, priority-aware scheduling. The initial implementation should begin with a simple per-CPU run queue and a clearly specified policy before adding affinity, real-time classes, or work stealing. Timer interrupts request rescheduling; they do not run scheduler policy in assembly.

## 7. Virtual memory model

Each task owns an isolated virtual address space. Architecture ports implement page-table operations and TLB maintenance; portable VM code owns mapping metadata, permissions, memory-object relationships, and fault policy. Page-table formats remain private to each architecture port.

## 8. Security invariants

- User mode cannot invoke privileged instructions or directly access kernel memory.
- Every object operation validates a capability and the requested right.
- Rights are never implicitly widened during transfer.
- User pointers are validated and copied through controlled VM helpers.
- Interrupt context does not block on IPC or acquire locks that may sleep.
- Kernel object lifetime is explicit and race-safe.
- Architecture-specific entry code establishes a known stack and register state before calling C.

## 9. Implementation sequence

1. Freeze the boot handoff and architecture entry contracts.
2. Implement assembly entry and minimal serial/panic output for both targets.
3. Establish per-CPU state, exception handling, and context switching.
4. Implement physical memory and page-table primitives.
5. Implement task/thread lifecycle and scheduler.
6. Implement capability spaces, ports, and IPC.
7. Add user-mode server interfaces and move drivers/services out of the kernel incrementally.
8. Add tests, fault injection, and SMP validation.

## 10. Open engineering decisions

These must be resolved before a bootable implementation is committed:
- boot protocol and loader;
- minimum CPU feature baselines for both targets;
- platform discovery format;
- interrupt-controller support on AArch64 and APIC support on x86-64;
- physical memory allocator strategy;
- kernel virtual address layout;
- syscall numbering and stable userspace ABI;
- SMP startup and per-CPU data layout.
