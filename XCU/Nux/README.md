# Nux Microkernel

> **Full feature requirements:** see [MACH_PARITY.md](MACH_PARITY.md). It tracks the complete Mach-derived feature target and distinguishes implemented foundation from interface-only declarations and planned work.

Nux now defines public interface headers for tasks, threads, ports/IPC, virtual memory, synchronization, processors, exceptions, clocks/timers, bootstrap, host/device management, debugging, and system calls. These headers are contracts, not claims of completed implementations. A cross-architecture header smoke test is part of the XCU build.

Nux is the original microkernel at the center of XCU. It is inspired by Mach-style microkernel concepts—tasks, threads, ports, message-based IPC, and explicit rights—but all implementation code and interfaces are designed for Lenix. No BSD, Mach, Linux, or XNU kernel source is incorporated.

## Kernel responsibilities

Nux is intended to retain mechanisms that require kernel privilege or global coordination:

- task/address-space ownership and lifecycle;
- thread lifecycle and execution state;
- cooperative kernel/user-thread scheduling, task-aware processor-set affinity, and scheduler-backed wait/wake (preemption remains planned);
- IPC ports, bounded message transport, rights transfer, and blocking receiver wakeup;
- capability validation and revocation;
- virtual-memory primitives, static ELF64 image loading, isolated user mappings, and initial user-stack construction;
- interrupt and exception dispatch, user-mode entry/return, and syscall routing;
- timers and synchronization primitives.

Filesystems, network stacks, device drivers, and higher-level system services are intended to run as isolated user-space servers where practical.

## Assembly-first hardware interface

Assembly is the primary language for hardware-facing execution paths. Each architecture port owns boot entry, privilege-state transitions, exception/interrupt entry and return, register save/restore, context switching, low-level MMU operations, and system-call entry/return. C owns portable algorithms and policy. Every assembly/C boundary must define register, stack, alignment, clobber, and return contracts.

## Core object model

- **Task:** resource and address-space container.
- **Thread:** schedulable execution context belonging to one task.
- **Port:** protected IPC endpoint.
- **Message:** bounded typed inline IPC payload; message-attached descriptors and out-of-line memory remain planned.
- **Capability:** opaque authority to perform defined operations on a kernel object.
- **Memory object:** kernel-managed backing object mapped into task address spaces.

A task is not a thread, and a port name is not itself authority. Access is mediated by rights held in a task's capability space.

## Implemented foundation

| Area | Current implementation | Not yet implemented |
|---|---|---|
| Boot | UEFI removable-media images and ISOs for both targets, Limine handoff, staged kernel startup, debugger-visible boot phases, x86-64 entry/IDT, ARM64 EL2-to-EL1 normalization and VBAR setup, separate ARM64 TTBR0 user root, ACPI RSDP/XSDT/RSDT/MADT discovery for APIC and GIC topology, and x86-64/AArch64 QEMU firmware boot smoke tests | Secure Boot signing, Device Tree, production console, interrupt controllers, SMP, init/bootstrap handoff |
| Physical memory | Usable-region discovery, HHDM translation, page ownership bitmap, duplicate/invalid-free rejection | Boot reservations, zones, NUMA, reclaim, zeroing policy |
| Runtime | Freestanding `memcpy`, `memmove`, `memset`, and `memcmp`, with host tests | Broader compiler runtime and memory object management |
| Synchronization | Assembly spinlocks, bounded counting semaphores, nonblocking lock sets, condition object lifecycle | Blocking waits, waiter queues, condition/mutex atomic wait, ownership, priority inheritance |
| Syscalls | Versioned dispatch registry, x86-64 DPL3 `int 0x80`, AArch64 EL0 `SVC`, six-word argument ABI, result restoration, and user-fault thread termination | Exception ports, richer syscall authorization, TLS, and full ABI compatibility tests |
| Tasks/threads | Separate registries, fixed kernel stacks, saved callee-saved contexts, ARM64/x86-64 context switch, cooperative kernel-thread scheduling, user-thread bootstrap, isolated user entry with sanitized registers and dedicated kernel stack, task suspension/resume, task-aware scheduling, one distinct VM address space per task, address-space activation on scheduler handoff, and task teardown/revocation | Preemption, per-CPU queues, TLS, FP/SIMD context, stack reclamation, task resource inheritance |
| Capabilities | Separate 255-entry generation-checked capability spaces per task, object types, owner authorization, quotas, constrained derivation that cannot duplicate unique receive/control rights, cross-task move, one-use consumption, object/task revocation | Dynamic quotas, audit/tracing, delegated-right policy |
| Processor topology | ACPI MADT discovery for x86 APIC/x2APIC and ARM GICC/GICD/GICR, hardware-to-logical CPU mapping, processor sets, CPU membership and policy metadata, scheduler-enforced thread affinity on the current CPU | AP startup, IPI, per-CPU scheduling, affinity enforcement, CPU feature/frequency enumeration |
| IPC | Task-scoped capability-checked ports, FIFO bounded inline queues, backpressure, scheduler-backed blocking receive/wakeup, nonblocking port sets, send-once, make/copy-send, atomic right transfer, receive-owner migration, destruction/revocation | Multi-waiter and blocking port sets, deadlines, out-of-line memory, RPC/reply, notifications |
| VM | Four-level ARM64/x86-64 page tables, address-space activation, anonymous/shared mappings, protection, translation, memory objects, task-owned address-space allocation, kernel VM read/write, static ELF64 PT_LOAD mapping with W^X permissions, rollback, aligned initial user stack/auxv, and QEMU user execution on both targets | Demand paging, page-fault resolution, COW, pager protocol, partial-range operations, guard pages, physical-hardware validation |
| Exceptions | x86-64 IDT with 256 normalized stubs; ARM64 EL1 VBAR vector table; user syscall frames and return paths; faulting user-thread termination | Recoverable exception ports, interrupt controllers, timer IRQs, debugger state/reply |

The physical allocator currently tracks up to 64 GiB of usable physical pages with a static ownership bitmap; larger usable maps fail initialization instead of being silently truncated. Bootloader-reserved ranges are excluded by the bootloader memory map; explicit kernel-owned reservations and reclaim policies remain pending. Page-table and anonymous mapping allocations are zeroed before use.

The scheduler now performs cooperative context switches between kernel threads on ARM64 and x86-64. Each task creation allocates a distinct address space and exposes its identifier through the task API; task-owned address spaces are covered by VM integration tests, and the cooperative scheduler activates the selected thread's task address space before switching contexts. Task destruction now refuses to proceed while owned threads remain live and releases the task's VM address space after they terminate; per-task capability namespaces now have independent 255-handle limits and task teardown revokes the entire space. Port objects remain in a fixed global kernel registry, while every access requires an owner-scoped capability. The scheduler is not preemptive and does not yet launch a persistent user-space bootstrap service. A transient ELF smoke task now enters ring 3/EL0, hands off through a kernel worker, exits through the syscall ABI, and returns to the kernel under QEMU on both architectures. Per-thread floating-point/SIMD state is not implemented; user SIMD is trapped, and kernel C is compiled without SIMD/FPU use. Kernel stacks are statically reserved per thread until a safe stack reaper exists. Address-space and page-table code is present for both targets, and scheduler handoff now selects the owning task's address space. Firmware boot and a user-mode execution/exit smoke test have been exercised under QEMU on both architectures. This validates the tested transition and syscall path, not production privilege isolation or arbitrary user workloads. Capabilities now use independent task-owned spaces, constrained derivation, generation-safe revocation, and cross-task move semantics. IPC supports bounded inline messages, per-task port rights, send-once, make/copy-send, send/receive right movement, and nonblocking receive; it is not yet a user-mode security boundary. Synchronization waits are nonblocking; condition waits, wakeups, and priority inheritance require a functioning scheduler. Exception entry is installed, but all exceptions are fatal until recovery and exception-port delivery exist. Both architecture images now pass UEFI firmware boot smoke tests in QEMU and reach Nux's READY phase; this does not establish physical-hardware compatibility or production readiness. ACPI discovery now parses RSDP, XSDT/RSDT, and MADT records for x86 APIC/x2APIC/IO-APIC and ARM GICC/GICD/GICR topology. Processor discovery maps the current hardware CPU ID to a logical CPU; processor sets enforce affinity against the current CPU, while AP startup and multi-CPU scheduling remain unimplemented. The ELF64 loader maps static executable PT_LOAD segments, enforces W^X, rolls back failed loads, and builds an aligned initial stack with argc/argv/envp terminators and an auxiliary vector. QEMU smoke programs now execute from those address spaces and return through the architecture syscall path; dynamic linking and a persistent init/bootstrap service remain planned. The boot path follows a macOS/XNU-shaped sequence (UEFI, boot manager, architecture entry, staged kernel initialization, then user-space bootstrap), but uses Limine rather than Apple's proprietary boot.efi. On ARM64, the kernel explicitly normalizes a Limine EL2/VHE handoff to EL1 and keeps kernel mappings in TTBR1 while TTBR0 is reserved for task address spaces.

## Next implementation order

1. Add a persistent user-space bootstrap task and service-manager handoff.
2. Add recoverable exception ports and machine-state reply/recovery.
3. Add interrupt controllers, hardware timer delivery, and preemptive scheduling.
4. Implement demand paging, page-fault resolution, COW, and external pager services.
5. Add message-attached port/memory descriptors, out-of-line memory, blocking waits, deadlines, and RPC.
6. Add per-thread TLS and FP/SIMD state management.
7. Add Device Tree support, boot-time memory reservations, allocator reclaim, and stress testing.
8. Add bootstrap service management, host/device interfaces, SMP, profiling/debugging, NIDL tooling, fuzzing, and physical-hardware qualification.

Kernel implementation code is restricted to C and assembly. Only AArch64 and x86-64 are supported targets.


## Building from macOS

The Makefile selects Clang/LLD ELF cross-targets automatically on macOS. Install the LLVM toolchain and GNU Make:

```sh
brew install llvm lld make qemu xorriso mtools
export PATH="$(brew --prefix llvm)/bin:$(brew --prefix make)/libexec/gnubin:$PATH"
make -C XCU clean all iso uefi-disk
```

This produces x86-64 and AArch64 ELF kernels, UEFI ISO images, and removable-media UEFI disk images. Both architecture images are boot-tested in QEMU in CI. The ISO targets use a pinned, checksum-verified Limine binary release. This does not produce a signed Apple boot image or boot on Apple Silicon through Apple's native boot chain; the current firmware boot contract is UEFI plus Limine.
