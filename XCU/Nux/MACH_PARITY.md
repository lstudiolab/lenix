# Nux Mach Capability-Parity Plan

Nux is an original Lenix microkernel. This document defines the complete Mach-derived capability target. It is a functional requirements catalogue, not a claim that every subsystem is already implemented. No Mach, XNU, BSD, or Linux kernel source is used.

## Design rules

1. Nux owns only mechanisms requiring privilege, isolation, scheduling, or global resource coordination.
2. Drivers, filesystems, networking, graphics, audio, and policy-heavy services run as isolated user tasks whenever practical.
3. Assembly is the primary hardware-facing language. C implements portable kernel mechanisms and policy.
4. The only supported CPU architectures are AArch64 (ARM64) and x86-64.
5. Every kernel object is accessed through validated, generation-safe capabilities. Raw kernel pointers never cross a task boundary.
6. Task and thread are separate objects. A task owns resources and an address space; threads execute inside it.
7. IPC is the standard service boundary. A service may be local or remote without changing its message contract.
8. Every subsystem must define resource limits, ownership, teardown, error handling, concurrency rules, and ABI versioning.
9. No API is considered implemented merely because its header exists. Implementation requires source, tests, and successful cross-builds on both architectures.

## Feature inventory

### 0. Firmware boot and kernel startup

- [x] First-party architecture-specific UEFI Boot Manager (`BOOTX64.EFI` / `BOOTAA64.EFI`)
- [x] First-party Nux ELF loader (`NUX.EFI`) and architecture-matched `NUX.ELF` loading
- [x] ELF64 header/program-header validation, bounded PT_LOAD loading, kernel/stack reservation, and memory-map capture
- [x] UEFI `ExitBootServices` handoff with the final memory-map key
- [x] x86-64 identity map, 64 GiB high-half direct map, and high-half kernel entry mapping
- [x] AArch64 TTBR0 identity map and TTBR1 kernel/direct-map split, with EL1 entry normalization
- [x] First-party UEFI removable-media and ISO image generation for both architectures
- [x] QEMU firmware boot tests execute the real Nux ELF kernel on x86-64 and AArch64
- [x] User-mode smoke executable enters ring 3/EL0, performs a syscall, exits, and returns to Nux on both architectures
- [ ] Versioned, size-checked boot handoff ABI and explicit reservation records
- [ ] Signed boot manifest, kernel-image signature verification, and Secure Boot key enrollment
- [ ] Firmware console, serial/framebuffer diagnostics, and graphical boot selection
- [ ] Timer and interrupt-controller initialization before enabling interrupts
- [ ] Launch a persistent isolated user-space bootstrap/service-manager task
- [ ] Physical-machine boot qualification on both architectures

**Current limits:** the loader maps a 4 GiB identity window and a 64 GiB direct-map window; the kernel ELF span is capped at 64 MiB and the handoff memory map at 256 regions. These limits are explicit and must be revisited before production deployment.

Legend: **Foundation** = some code exists; **Interface** = public contract declared; **Planned** = implementation remains.

### 1. Tasks and resource containers

- [x] Initial task registry and task state model (Foundation; not a complete task implementation)
- [ ] Task creation from executable image
- [x] Task destruction gated on live-thread check, address-space release, owner-port/port-set cleanup, and task capability-space revocation (Foundation)
- [x] Task suspension and resume with scheduler exclusion (Foundation; restart and full termination policy remain)
- [x] Distinct VM address space allocated and associated with each task (Foundation; task teardown and lifecycle reclamation remain)
- [x] Address-space teardown after all task threads terminate (Foundation; inheritance and per-task accounting remain)
- [x] Independent per-task capability spaces with fixed 255-handle quotas (port objects remain in a global kernel registry)
- [ ] Task inheritance and creation attributes
- [ ] Task information, accounting, and limits
- [ ] Task special ports and host/service access
- [ ] Task resource quotas and isolation boundaries beyond fixed capability-space quotas

### 2. Threads and machine state

- [x] Separate thread registry, lifecycle metadata, kernel stacks, and saved architecture contexts (Foundation; user-mode state remains)
- [x] Multiple cooperative kernel threads per task (Foundation; user-mode threads remain)
- [x] Kernel-thread creation, termination, suspend/resume metadata, cooperative yield, and scheduler-backed wait/wake transitions
- [x] Callee-saved register contexts and architecture-specific cooperative switch assembly on both targets (Foundation; FPU/SIMD and user register frames remain)
- [ ] Thread state get/set for controlled debugging
- [x] Fixed kernel stacks for cooperative kernel threads (Foundation; user stacks and stack reclamation remain)
- [ ] Thread-local storage and per-CPU state
- [ ] Thread naming, information, and accounting
- [x] Thread-to-processor-set affinity metadata and CPU-set membership (Foundation; current-CPU eligibility is enforced, SMP migration remains)
- [x] User-thread bootstrap with isolated address space, dedicated kernel stack, sanitized general registers, and architecture privilege transition (Foundation; no TLS or saved FP/SIMD state)
- [ ] Thread exception state and special ports

### 3. Scheduling and execution

- [x] Cooperative round-robin dispatch, real context switching, and blocked-thread exclusion/wakeup between kernel threads (Foundation; no preemption)
- [x] Activate the selected thread's owning task address space on scheduler handoff (Foundation; cooperative only)
- [ ] Preemptive scheduling and timer-driven quanta
- [ ] Dynamic time-sharing priorities
- [ ] Fixed-priority scheduling
- [ ] Real-time time-constraint scheduling (period, computation budget, deadline)
- [ ] Priority inheritance and starvation controls
- [x] Scheduler handoff and voluntary yield between kernel threads
- [ ] Per-CPU run queues and load balancing
- [x] Processor-set creation/destruction, CPU membership, policy metadata, and thread affinity association (Foundation; per-CPU scheduling remains)
- [ ] Scheduler statistics and profiling

### 4. Ports, capabilities, and IPC

- [x] Independent per-task capability spaces, generation-checked handles, caller authorization, and per-task quotas
- [x] Separate per-task capability tables and fixed quotas
- [x] Port creation/destruction and bounded queue metadata with receive-owner authorization
- [x] Capability-checked send/receive/send-once rights with atomic one-use consumption
- [x] Move send, receive, and send-once rights between task owners; make-send, copy-send, and send-once derivation
- [ ] Port death, no-senders, and lifecycle notifications
- [x] Port sets with nonblocking wait-on-any scan (Foundation; scheduler-backed blocking port-set waits remain)
- [x] FIFO bounded inline-message queues with backpressure and scheduler-backed blocking receive/wakeup (single waiter per port; multi-waiter and port-set blocking remain)
- [ ] Message send/receive deadlines, timeouts, and interruption
- [ ] Inline and out-of-line message payloads
- [x] Atomic capability movement between task owners (Foundation; message-attached memory/port descriptors remain)
- [x] Inline message validation, size limits, and fixed-pool resource accounting
- [x] Asynchronous bounded messaging (synchronous RPC/reply protocol remains)
- [ ] Reply ports and transaction correlation
- [ ] IPC notification delivery
- [ ] Kernel-object ports and service-object ports
- [ ] IPC tracing and diagnostics

### 5. Virtual memory and memory objects

- [x] Physical page allocator with region tracking, allocation bitmap, and duplicate-free rejection (Foundation; boot reservations/reclaim work remains)
- [x] Fixed-capacity sparse address-space objects (Foundation; task creation allocates and records one address space)
- [x] Four-level x86-64 and AArch64 page-table mapping, unmapping, protection, translation, and address-space activation (validated by first-party QEMU user-mode boots on both targets)
- [x] Eager anonymous page-backed mappings and exact-region unmap/protect operations (partial-range operations and region query remain)
- [x] Zero-filled anonymous demand mappings with fault-time allocation and cross-page software read/write support (external pager-backed file faults remain)
- [x] Static ELF64 PT_LOAD executable loader for both architectures with segment validation, W^X permissions, rollback, and initial user stack/auxv construction (Foundation; demand paging and dynamic linking remain)
- [x] QEMU validation of isolated user executable mappings and initial user-stack execution on both architectures
- [x] Anonymous demand-zero page-fault resolution on x86-64 and AArch64 (external pager and file-backed demand paging remain)
- [x] Anonymous page-backed memory objects (Foundation; fixed maximum object size)
- [ ] Named memory entries and transferable mapping rights
- [x] Shared physical backing through memory-object mappings in multiple address spaces (fixed-size objects; named entries and transferable mapping rights remain)
- [x] Anonymous-region copy-on-write cloning with physical page reference counts, write-protection, fault-time split, and reclamation (full task-fork inheritance semantics and IPC transfer optimizations remain)
- [ ] External pager protocol and user-space memory managers
- [ ] Pager initialization, data request/return, synchronize, terminate
- [ ] Page locking, unlocking, wiring, and unwiring
- [ ] Memory object attributes, caching, and purgeability
- [ ] Memory pressure, reclaim, and per-task accounting
- [x] W^X validation for ELF segments and VM protection requests
- [ ] Guard pages and broader mapping validation

## 6. Synchronization and wait primitives

- [x] Bounded counting semaphores with nonblocking wait/post (Foundation; wait queues and post-all remain)
- [ ] Notification objects with state
- [x] Lock sets with nonblocking acquire/release (Foundation; owner tracking remains)
- [x] Condition object lifecycle and signal sequence (Foundation; condition wait remains unsupported)
- [ ] Mutexes and fully blocking condition variables
- [ ] Wait queues and wake-one/wake-all
- [ ] Timed waits and interruptible waits
- [ ] Priority inheritance and deadlock diagnostics
- [ ] Interrupt-safe spinlocks and per-CPU synchronization

### 7. Exceptions, traps, and system calls

- [x] AArch64 EL1 exception vector table and x86-64 IDT with 256 vector stubs (Foundation; fatal capture only, no recovery or TSS/IST setup)
- [x] Fatal exception capture records vector, architecture syndrome/error code, and fault address (Foundation; stops the CPU safely)
- [ ] Fault classification, recoverable kernel exception handling, and panic diagnostics
- [ ] Per-task and per-thread exception ports
- [ ] Exception delivery with identity and machine state
- [ ] Exception reply/recovery and thread termination
- [x] Versioned syscall dispatch registry and handler validation
- [x] Kernel-side syscall dispatch with six-word argument contract and registered handlers
- [x] x86-64 DPL3 int 0x80 and AArch64 EL0 SVC entry/return frames, including six-argument dispatch and result restoration
- [x] User fault containment by terminating the faulting thread (Foundation; exception ports and machine-state recovery remain)
- [ ] System-call redirection/emulation hooks for compatibility servers
- [ ] Debugger attach/detach authorization

### 8. Time and clocks

- [x] Monotonic and boot clocks backed by ARM generic timer or x86 TSC when frequency discovery is available (Foundation; realtime and per-thread clocks remain)
- [x] Architecture clock-source frequency discovery on both supported targets
- [x] Software one-shot and periodic timer metadata with polling and cancellation (Foundation; no interrupt-driven expiry delivery)
- [ ] Scheduler-integrated thread sleep/wakeup and timed waits
- [x] Timer cancellation and polling-based expiration accounting
- [x] Explicit per-thread runtime accounting API (Foundation; not yet wired into scheduler preemption)
- [ ] Time-constraint scheduler integration

### 9. Processors and multiprocessor operation

- [x] ACPI MADT CPU topology discovery and hardware-to-logical CPU mapping (Foundation; CPU feature enumeration and Device Tree remain)
- [ ] Bootstrap and application processor startup
- [ ] Per-CPU data and scheduler state
- [ ] Inter-processor interrupts
- [ ] Processor online/offline and controlled stop
- [x] Processor-set creation/destruction and CPU membership metadata (Foundation; processor-set ports remain)
- [x] Associate threads with processor sets and store scheduling policy/priority metadata (Foundation; per-CPU enforcement remains)
- [ ] Per-set scheduling policy and priority
- [ ] Load balancing, migration, and SMP-safe synchronization
- [ ] NUMA topology and memory locality where hardware supports it

### 10. Host, devices, and kernel management

- [ ] Host object and host-control capability
- [ ] Host information, clock, shutdown, and reboot operations
- [ ] Processor and processor-set object interfaces
- [ ] Device service interface: open, close, read, write, map, control
- [ ] Device ownership and DMA/IOMMU isolation
- [ ] Interrupt delivery to authorized driver tasks
- [ ] Device discovery and resource assignment
- [ ] Kernel debugger and crash/fault reporting
- [ ] Task/thread profiling and performance counters

### 11. Bootstrap and user-space servers

- [ ] Kernel boot environment and validated boot modules
- [x] ELF-backed initial task construction with failure rollback (Foundation; boot-module discovery and persistent launch remain)
- [ ] First user task and initial address space
- [ ] Bootstrap/service manager
- [x] Capability-validated service registration, lookup, owner-only unregister, and owner-task cleanup (Foundation; service startup policy and restart supervision remain)
- [ ] Service restart policy
- [x] Task-scoped task/thread/host/bootstrap special-port slots with capability validation and teardown cleanup (Foundation; kernel policy and automatic population remain)
- [ ] User-space default memory pager
- [ ] User-space device manager and driver servers
- [ ] User-space filesystem and network servers
- [ ] Service health, crash isolation, and dependency ordering

### 12. Interface tooling and distributed extensions

- [ ] Nux Interface Definition Language (NIDL) for typed message/RPC contracts
- [ ] C-based NIDL compiler and generated client/server stubs
- [ ] ABI versioning, schema validation, and compatibility tests
- [ ] Optional remote IPC transport with authenticated endpoints
- [ ] Remote object references, timeout, retry, and failure semantics
- [ ] Large-message streaming and memory-region transfer
- [ ] Distributed service discovery (optional system service)

Distributed IPC is an optional extension, not a prerequisite for a local microkernel.

## Runtime support

- [x] Freestanding byte-copy, overlap-safe move, fill, and compare primitives
- [x] Host-side regression tests for runtime primitives, per-task capabilities and rights derivation/transfer/quotas, IPC queues and port sets, ACPI topology parsing, processor sets, physical page ownership, semaphores, lock sets, x86 context switching, syscall dispatch, VM reads/writes, ELF loading/rollback/stack layout, and software timers
- [ ] Full compiler runtime support and sanitizer/fuzz coverage

## Public interface headers

The initial public contracts are under `Include/nux/`:

- `task.h`, `thread.h`: task/thread model and scheduling controls
- `port.h`, `capability.h`, `object.h`: object rights and IPC
- `vm.h`, `memory.h`: address spaces and memory objects
- `sync.h`: semaphores, locks, and conditions
- `processor.h`: CPUs, processor sets, affinity
- `exception.h`, `syscall.h`: exception and trap boundaries
- `time.h`: clocks and timers
- `bootstrap.h`: initial task and service registry
- `host.h`, `device.h`: host and device management
- `debug.h`: profiling and debugging

These headers are interface declarations only unless the implementation status above explicitly says Foundation.

## Implementation sequence

1. Version and harden the first-party boot handoff; add explicit reservations, firmware diagnostics, signed manifests, and Secure Boot key enrollment.
2. Add demand paging, page-fault resolution, copy-on-write, and external pagers.
3. Complete IPC with out-of-line memory, blocking port sets, deadlines, notifications, and RPC/reply.
4. Initialize interrupt controllers and hardware timers; add preemptive scheduling and timed waits.
5. Implement recoverable exception ports and user fault delivery.
6. Add AP startup, per-CPU state, IPIs, SMP-safe synchronization, and load balancing.
7. Implement the persistent bootstrap/service manager and user-space pager, device, filesystem, and network servers.
8. Add host/device controls, profiling/debugging, NIDL tooling, fuzzing, and physical-hardware qualification.

## Completion criteria

A subsystem is complete only when it has:
- C and/or assembly implementation, with assembly owning hardware entry/exit paths;
- tests for normal, boundary, invalid, concurrent, and failure cases;
- capability and ownership checks on every privileged operation;
- deterministic cleanup and resource accounting;
- ARM64 and x86-64 build coverage;
- boot/runtime validation where hardware behavior is involved;
- documentation of ABI, invariants, and limitations.
