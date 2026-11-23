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

- [x] UEFI-to-Limine boot protocol handoff on x86-64 and AArch64 (Foundation; boot image packaging remains)
- [x] Explicit Limine request delimiters retained by architecture linker scripts
- [x] x86-64 entry stack alignment, interrupt masking, and IDT installation
- [x] AArch64 EL2/VHE normalization to EL1h before entering the common kernel
- [x] AArch64 kernel/user translation split: TTBR1 retains kernel mappings and TTBR0 starts from an empty root
- [x] Common staged initialization with debugger-visible phase and failure code
- [x] Bootable UEFI ISO and removable-media image generation for both architectures (Secure Boot signing remains pending)
- [ ] Firmware console and early serial/framebuffer diagnostics
- [ ] ACPI/Device Tree discovery and CPU feature enumeration
- [ ] Timer and interrupt-controller initialization before enabling interrupts
- [ ] Launch the first isolated user-space bootstrap task
- [x] QEMU UEFI firmware boot smoke tests reach Nux READY on x86-64 and AArch64
- [ ] Physical-machine boot qualification on both architectures


Legend: **Foundation** = some code exists; **Interface** = public contract declared; **Planned** = implementation remains.

### 1. Tasks and resource containers

- [x] Initial task registry and task state model (Foundation; not a complete task implementation)
- [ ] Task creation from executable image
- [x] Task destruction gated on live-thread check, address-space release, and owner-port/port-set cleanup (Foundation; task-scoped non-port capabilities remain)
- [ ] Task suspension, resume, termination, and restart
- [x] Distinct VM address space allocated and associated with each task (Foundation; task teardown and lifecycle reclamation remain)
- [x] Address-space teardown after all task threads terminate (Foundation; inheritance and per-task accounting remain)
- [ ] Per-task capability/port namespace
- [ ] Task inheritance and creation attributes
- [ ] Task information, accounting, and limits
- [ ] Task special ports and host/service access
- [ ] Task resource quotas and isolation boundaries

### 2. Threads and machine state

- [x] Separate thread registry, lifecycle metadata, kernel stacks, and saved architecture contexts (Foundation; user-mode state remains)
- [x] Multiple cooperative kernel threads per task (Foundation; user-mode threads remain)
- [x] Kernel-thread creation, termination, suspend/resume metadata, and cooperative yield
- [x] Callee-saved register contexts and architecture-specific cooperative switch assembly on both targets (Foundation; FPU/SIMD and user register frames remain)
- [ ] Thread state get/set for controlled debugging
- [x] Fixed kernel stacks for cooperative kernel threads (Foundation; user stacks and stack reclamation remain)
- [ ] Thread-local storage and per-CPU state
- [ ] Thread naming, information, and accounting
- [ ] Thread migration and processor affinity
- [ ] Thread exception state and special ports

### 3. Scheduling and execution

- [x] Cooperative round-robin dispatch and real context switching between kernel threads (Foundation; no preemption)
- [x] Activate the selected kernel thread's owning task address space on scheduler handoff (Foundation; user-mode transitions remain)
- [ ] Preemptive scheduling and timer-driven quanta
- [ ] Dynamic time-sharing priorities
- [ ] Fixed-priority scheduling
- [ ] Real-time time-constraint scheduling (period, computation budget, deadline)
- [ ] Priority inheritance and starvation controls
- [x] Scheduler handoff and voluntary yield between kernel threads
- [ ] Per-CPU run queues and load balancing
- [ ] CPU affinity and processor-set policies
- [ ] Scheduler statistics and profiling

### 4. Ports, capabilities, and IPC

- [x] Generation-checked task-owned capability handles and caller authorization (Foundation; globally backed table)
- [ ] Separate per-task capability tables and quotas
- [x] Port creation/destruction and bounded queue metadata with receive-owner authorization
- [x] Capability-checked send/receive/send-once rights with atomic one-use consumption
- [x] Move send, receive, and send-once rights between task owners (Foundation; copy/make-send derivation remains)
- [ ] Port death, no-senders, and lifecycle notifications
- [x] Port sets with nonblocking wait-on-any scan (Foundation; scheduler-backed blocking waits remain)
- [x] FIFO bounded inline-message queues with backpressure
- [ ] Message send/receive timeouts and interruption
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
- [x] Four-level x86-64 and AArch64 page-table mapping, unmapping, protection, translation, and address-space activation (Foundation; hardware runtime validation pending)
- [x] Anonymous page-backed mappings and exact-region unmap/protect operations (Foundation; partial-range operations and region query remain)
- [ ] Demand paging and page-fault resolution
- [x] Anonymous page-backed memory objects (Foundation; fixed maximum object size)
- [ ] Named memory entries and transferable mapping rights
- [x] Shared physical backing through memory-object mappings in multiple address spaces (Foundation; no copy-on-write or inheritance implementation)
- [ ] Copy-on-write cloning and IPC transfer optimizations
- [ ] External pager protocol and user-space memory managers
- [ ] Pager initialization, data request/return, synchronize, terminate
- [ ] Page locking, unlocking, wiring, and unwiring
- [ ] Memory object attributes, caching, and purgeability
- [ ] Memory pressure, reclaim, and per-task accounting
- [ ] Guard pages, W^X enforcement, and mapping validation

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
- [x] Versioned syscall dispatch registry and handler validation (Foundation; no user entry/return path yet)
- [x] Kernel-side syscall dispatch with six-word argument contract and registered handlers
- [ ] User-to-kernel and kernel-to-user transition assembly
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

- [ ] CPU feature discovery and topology
- [ ] Bootstrap and application processor startup
- [ ] Per-CPU data and scheduler state
- [ ] Inter-processor interrupts
- [ ] Processor online/offline and controlled stop
- [ ] Processor sets and processor-set ports
- [ ] Assign tasks/threads to processor sets
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
- [ ] First user task and initial address space
- [ ] Bootstrap/service manager
- [ ] Service registration, lookup, and restart policy
- [ ] Special ports for task, thread, host, and bootstrap services
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
- [x] Host-side regression tests for runtime primitives, IPC queues (create/send/receive/backpressure/buffer sizing/port sets/revocation), physical page ownership, semaphores, lock sets, x86 context switching, syscall dispatch, VM objects, and software timers
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

1. Stabilize boot handoff, add memory reservations and page zeroing, and harden allocator tests.
2. Implement architecture page tables, address spaces, faults, and memory objects.
3. Add task ownership, per-task capability namespaces, and user-mode task loading.
4. Complete IPC rights transfer, out-of-line memory, blocking waits, deadlines, and RPC.
5. Add interrupt controllers, architecture timers, preemptive scheduling, and time accounting.
6. Implement recoverable exception ports and architecture syscall entry/return.
7. Add bootstrap service management and user-space pager/driver servers.
8. Add SMP, host/device controls, profiling/debugging, NIDL tooling, fuzzing, and hardware qualification.

## Completion criteria

A subsystem is complete only when it has:
- C and/or assembly implementation, with assembly owning hardware entry/exit paths;
- tests for normal, boundary, invalid, concurrent, and failure cases;
- capability and ownership checks on every privileged operation;
- deterministic cleanup and resource accounting;
- ARM64 and x86-64 build coverage;
- boot/runtime validation where hardware behavior is involved;
- documentation of ABI, invariants, and limitations.
