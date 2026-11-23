# Nux Microkernel

> **Full feature requirements:** see [MACH_PARITY.md](MACH_PARITY.md). It tracks the complete Mach-derived feature target and distinguishes implemented foundation from interface-only declarations and planned work.

Nux now defines public interface headers for tasks, threads, ports/IPC, virtual memory, synchronization, processors, exceptions, clocks/timers, bootstrap, host/device management, debugging, and system calls. These headers are contracts, not claims of completed implementations. A cross-architecture header smoke test is part of the XCU build.

Nux is the original microkernel at the center of XCU. It is inspired by Mach-style microkernel concepts—tasks, threads, ports, message-based IPC, and explicit rights—but all implementation code and interfaces are designed for Lenix. No BSD, Mach, Linux, or XNU kernel source is incorporated.

## Kernel responsibilities

Nux is intended to retain mechanisms that require kernel privilege or global coordination:

- task/address-space ownership and lifecycle;
- thread lifecycle and execution state;
- preemptive scheduling and CPU dispatch;
- IPC ports, bounded message transport, and rights transfer;
- capability validation and revocation;
- virtual-memory primitives and page-fault coordination;
- interrupt and exception dispatch;
- timers and synchronization primitives.

Filesystems, network stacks, device drivers, and higher-level system services are intended to run as isolated user-space servers where practical.

## Assembly-first hardware interface

Assembly is the primary language for hardware-facing execution paths. Each architecture port owns boot entry, privilege-state transitions, exception/interrupt entry and return, register save/restore, context switching, low-level MMU operations, and system-call entry/return. C owns portable algorithms and policy. Every assembly/C boundary must define register, stack, alignment, clobber, and return contracts.

## Core object model

- **Task:** resource and address-space container.
- **Thread:** schedulable execution context belonging to one task.
- **Port:** protected IPC endpoint.
- **Message:** bounded, typed IPC payload with optional out-of-line memory and transferred rights.
- **Capability:** opaque authority to perform defined operations on a kernel object.
- **Memory object:** kernel-managed backing object mapped into task address spaces.

A task is not a thread, and a port name is not itself authority. Access is mediated by rights held in a task's capability space.

## Implemented foundation

| Area | Current implementation | Not yet implemented |
|---|---|---|
| Boot | Limine handoff on ARM64 and x86-64 | Full CPU feature discovery and SMP bring-up |
| Physical memory | Usable-region discovery, page allocation/free, HHDM translation | Reservation tracking, zones, NUMA, reclaim |
| Runtime | Freestanding `memcpy`, `memmove`, `memset`, and `memcmp`, with host tests | Allocator hardening and memory object management |
| Synchronization | Assembly spinlock primitives on both targets | Interrupt-safe lock classes, wait queues, priority inheritance |
| Tasks | Fixed-capacity task registry and round-robin selection policy | Separate task/thread objects, stacks, saved contexts, lifecycle cleanup |
| Capabilities | Generation-checked handles, object types, rights checks, revocation | Per-task capability spaces, derivation, transfer, audit |
| IPC | Stable port/message interface declarations | Port objects, queues, blocking, deadlines, rights transfer |
| VM | Design boundary only | Page tables, mapping APIs, faults, address-space switching |
| Exceptions | x86-64 IDT with 256 normalized stubs; ARM64 EL1 VBAR vector table; fatal exception capture | Recoverable exception ports, interrupt controllers, timer IRQs, syscall ABI |

The current scheduler code is a policy scaffold only. It does not perform a context switch, and the kernel does not yet launch user tasks. A successful cross-build is not equivalent to a bootable or production-ready kernel.

## Next implementation order

1. Correct and test boot memory reservations and allocator invariants.
2. Add architecture CPU descriptors, exception vectors, interrupt controllers, and timer sources.
3. Implement per-CPU saved contexts and assembly context-switch paths.
4. Separate tasks from threads and connect scheduler dispatch to real context switching.
5. Implement address spaces and page-table operations for both MMUs.
6. Implement port objects, queued IPC, blocking/wakeup, deadlines, and rights transfer.
7. Add syscall entry/return and move the first system servers to user mode.
8. Add SMP synchronization, fault isolation, stress tests, and hardware qualification.

Kernel implementation code is restricted to C and assembly. Only AArch64 and x86-64 are supported targets.
