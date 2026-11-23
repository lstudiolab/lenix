# Nux Microkernel

> **Full feature requirements:** see [MACH_PARITY.md](MACH_PARITY.md). It tracks the complete Mach-derived feature target and distinguishes implemented foundation from interface-only declarations and planned work.

Nux now defines public interface headers for tasks, threads, ports/IPC, virtual memory, synchronization, processors, exceptions, clocks/timers, bootstrap, host/device management, debugging, and system calls. These headers are contracts, not claims of completed implementations. A cross-architecture header smoke test is part of the XCU build.

Nux is the original microkernel at the center of XCU. It is inspired by Mach-style microkernel concepts—tasks, threads, ports, message-based IPC, and explicit rights—but all implementation code and interfaces are designed for Lenix. No BSD, Mach, Linux, or XNU kernel source is incorporated.

## Kernel responsibilities

Nux is intended to retain mechanisms that require kernel privilege or global coordination:

- task/address-space ownership and lifecycle;
- thread lifecycle and execution state;
- cooperative kernel-thread scheduling (preemption remains planned);
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
| Boot | Delimited Limine requests, common staged kernel startup, debugger-visible boot phases, x86-64 entry/IDT, ARM64 EL2-to-EL1 normalization and VBAR setup, separate ARM64 TTBR0 user root | Bootable image packaging, CPU feature discovery, console, interrupt controllers, SMP, init/bootstrap handoff |
| Physical memory | Usable-region discovery, HHDM translation, page ownership bitmap, duplicate/invalid-free rejection | Boot reservations, zones, NUMA, reclaim, zeroing policy |
| Runtime | Freestanding `memcpy`, `memmove`, `memset`, and `memcmp`, with host tests | Broader compiler runtime and memory object management |
| Synchronization | Assembly spinlocks, bounded counting semaphores, nonblocking lock sets, condition object lifecycle | Blocking waits, waiter queues, condition/mutex atomic wait, ownership, priority inheritance |
| Syscalls | Versioned kernel-side dispatch registry with six arguments and result validation | Architecture user entry/return, caller isolation, syscall authorization and ABI tests |
| Tasks/threads | Separate registries, fixed kernel stacks, saved callee-saved contexts, ARM64/x86-64 context switch, cooperative kernel-thread scheduling, one distinct VM address space allocated per task, address-space activation on scheduler handoff, task teardown after all owned threads terminate, revocation of task-owned port capabilities, and removal of owned port sets | User-mode contexts, task resource inheritance, preemption, per-CPU queues, stack reclamation |
| Capabilities | Generation-checked handles, object types, rights checks, revocation | Per-task capability spaces, derivation, transfer, audit |
| IPC | Capability-checked ports, FIFO bounded inline queues, port sets, backpressure, destruction/revocation | Blocking waits, deadlines, per-task namespaces, right transfer, out-of-line memory, RPC/notifications |
| VM | Four-level ARM64/x86-64 page tables, address-space activation, anonymous/shared mappings, protection, translation, memory objects, task-owned address-space allocation | Demand paging, page-fault resolution, COW, pager protocol, partial-range operations, task teardown, hardware runtime validation |
| Exceptions | x86-64 IDT with 256 normalized stubs; ARM64 EL1 VBAR vector table; fatal exception capture | Recoverable exception ports, interrupt controllers, timer IRQs, syscall ABI |

The physical allocator currently tracks up to 64 GiB of usable physical pages with a static ownership bitmap; larger usable maps fail initialization instead of being silently truncated. Bootloader-reserved ranges are excluded by the bootloader memory map; explicit kernel-owned reservations, reclaim policies, and allocator page zeroing remain pending. Page-table and anonymous mapping allocations are zeroed before use.

The scheduler now performs cooperative context switches between kernel threads on ARM64 and x86-64. Each task creation allocates a distinct address space and exposes its identifier through the task API; task-owned address spaces are covered by VM integration tests, and the cooperative scheduler activates the selected thread's task address space before switching contexts. Task destruction now refuses to proceed while owned threads remain live and releases the task's VM address space after they terminate; per-task IPC/capability namespaces and full task resource inheritance remain unimplemented. The scheduler is not preemptive, does not yet launch user tasks, and does not save floating-point state; x86 kernel C is therefore compiled without SIMD/FPU use. Kernel stacks are statically reserved per thread until a safe stack reaper exists. Address-space and page-table code is present for both targets, and scheduler handoff now selects the owning task's address space. Hardware page-table activation and privilege isolation have not been validated on hardware or under an emulator. IPC currently uses a global capability table, supports bounded inline messages only, and performs nonblocking receive; it is not yet a secure user-task boundary. Synchronization waits are nonblocking; condition waits, wakeups, and priority inheritance require a functioning scheduler. Exception entry is installed, but all exceptions are fatal until recovery and exception-port delivery exist. A successful cross-build is not equivalent to a bootable or production-ready kernel. The boot path now follows a macOS/XNU-shaped sequence (UEFI, boot manager, architecture entry, staged kernel initialization, then user-space bootstrap), but uses Limine rather than Apple's proprietary boot.efi. On ARM64, the kernel explicitly normalizes a Limine EL2/VHE handoff to EL1 and keeps kernel mappings in TTBR1 while TTBR0 is reserved for task address spaces.

## Next implementation order

1. Add task teardown/reclamation and per-task capability/port namespaces.
2. Implement executable loading, user stacks, privilege transitions, and syscall entry/return.
3. Complete IPC right transfer, out-of-line memory, blocking waits, deadlines, and RPC.
4. Add interrupt controllers, hardware timer delivery, and preemptive scheduling.
5. Implement demand paging, page-fault resolution, COW, and external pager services.
6. Implement recoverable exception ports and machine-state reply/recovery.
7. Add boot-time memory reservations, allocator reclaim, and stress testing.
8. Add bootstrap service management, host/device interfaces, SMP, profiling/debugging, NIDL tooling, fuzzing, and hardware qualification.

Kernel implementation code is restricted to C and assembly. Only AArch64 and x86-64 are supported targets.


## Building from macOS

The Makefile selects Clang/LLD ELF cross-targets automatically on macOS. Install the LLVM toolchain and GNU Make:

```sh
brew install llvm lld make qemu xorriso
export PATH="$(brew --prefix llvm)/bin:$(brew --prefix make)/libexec/gnubin:$PATH"
make -C XCU clean all iso
```

This produces the same x86-64 and AArch64 ELF kernels and UEFI ISO images as the Linux build. The ISO targets use a pinned, checksum-verified Limine binary release. This does not produce a signed Apple boot image or boot on Apple Silicon through Apple's native boot chain; the current firmware boot contract is UEFI plus Limine.
