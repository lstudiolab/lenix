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

Legend: **Foundation** = some code exists; **Interface** = public contract declared; **Planned** = implementation remains.

### 1. Tasks and resource containers

- [x] Initial task registry and task state model (Foundation; not a complete task implementation)
- [ ] Task creation from executable image
- [ ] Task destruction and resource reclamation
- [ ] Task suspension, resume, termination, and restart
- [ ] Per-task address space
- [ ] Per-task capability/port namespace
- [ ] Task inheritance and creation attributes
- [ ] Task information, accounting, and limits
- [ ] Task special ports and host/service access
- [ ] Task resource quotas and isolation boundaries

### 2. Threads and machine state

- [x] Separate thread registry, creation, termination, suspend/resume, and state metadata (Foundation; no saved execution contexts or context switching yet)
- [ ] Multiple threads per task
- [x] Thread creation, termination, suspend, and resume metadata (Foundation; yield intentionally returns NOT_SUPPORTED until context switching exists)
- [ ] Saved register contexts and architecture-specific state
- [ ] Thread state get/set for controlled debugging
- [ ] Kernel and user stacks
- [ ] Thread-local storage and per-CPU state
- [ ] Thread naming, information, and accounting
- [ ] Thread migration and processor affinity
- [ ] Thread exception state and special ports

### 3. Scheduling and execution

- [x] Initial round-robin selection scaffold (Foundation only; no dispatch/context switch)
- [ ] Preemptive scheduling and timer-driven quanta
- [ ] Dynamic time-sharing priorities
- [ ] Fixed-priority scheduling
- [ ] Real-time time-constraint scheduling (period, computation budget, deadline)
- [ ] Priority inheritance and starvation controls
- [ ] Scheduler handoff and voluntary yield
- [ ] Per-CPU run queues and load balancing
- [ ] CPU affinity and processor-set policies
- [ ] Scheduler statistics and profiling

### 4. Ports, capabilities, and IPC

- [x] Generation-checked capability handles and rights validation (Foundation; global table only)
- [ ] Per-task capability namespaces
- [ ] Port creation, destruction, ownership, and queue limits
- [ ] Send, receive, and send-once rights
- [ ] Right copy, move, make-send, and transfer
- [ ] Port death, no-senders, and lifecycle notifications
- [ ] Port sets and wait-on-any semantics
- [ ] Ordered reliable message queues with backpressure
- [ ] Message send/receive timeouts and interruption
- [ ] Inline and out-of-line message payloads
- [ ] Atomic transfer of port rights and memory descriptors
- [ ] Message validation, size limits, and resource accounting
- [ ] Synchronous RPC and asynchronous messaging
- [ ] Reply ports and transaction correlation
- [ ] IPC notification delivery
- [ ] Kernel-object ports and service-object ports
- [ ] IPC tracing and diagnostics

### 5. Virtual memory and memory objects

- [x] Initial physical page allocator (Foundation; reservation/reclaim work remains)
- [ ] Per-task sparse virtual address spaces
- [ ] Architecture page-table management and address-space switching
- [ ] Map, unmap, remap, protect, and region-query operations
- [ ] Demand paging and page-fault resolution
- [ ] Anonymous memory objects
- [ ] Named memory entries and transferable mapping rights
- [ ] Shared memory and memory inheritance
- [ ] Copy-on-write cloning and IPC transfer optimizations
- [ ] External pager protocol and user-space memory managers
- [ ] Pager initialization, data request/return, synchronize, terminate
- [ ] Page locking, unlocking, wiring, and unwiring
- [ ] Memory object attributes, caching, and purgeability
- [ ] Memory pressure, reclaim, and per-task accounting
- [ ] Guard pages, W^X enforcement, and mapping validation

### 6. Synchronization and wait primitives

- [ ] Counting semaphores with wait, post, and post-all
- [ ] Notification objects with state
- [ ] Lock sets and transaction-style locking
- [ ] Mutexes and condition variables
- [ ] Wait queues and wake-one/wake-all
- [ ] Timed waits and interruptible waits
- [ ] Priority inheritance and deadlock diagnostics
- [ ] Interrupt-safe spinlocks and per-CPU synchronization

### 7. Exceptions, traps, and system calls

- [ ] AArch64 exception vectors and x86-64 IDT/TSS setup
- [ ] Fault classification and safe kernel exception handling
- [ ] Per-task and per-thread exception ports
- [ ] Exception delivery with identity and machine state
- [ ] Exception reply/recovery and thread termination
- [ ] Versioned system-call ABI
- [ ] System-call dispatch and argument validation
- [ ] User-to-kernel and kernel-to-user transition assembly
- [ ] System-call redirection/emulation hooks for compatibility servers
- [ ] Debugger attach/detach authorization

### 8. Time and clocks

- [ ] Monotonic, realtime, boot, and CPU clocks
- [ ] Architecture timer drivers and clock source selection
- [ ] One-shot and periodic timers
- [ ] Thread sleep/wakeup and timed waits
- [ ] Timer cancellation and expiration delivery
- [ ] CPU-time accounting and profiling clocks
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

1. Stabilize boot handoff, physical memory reservations, locks, and build validation.
2. Implement CPU discovery, exception vectors, interrupt controllers, timers, and AP startup.
3. Implement thread objects, kernel/user stacks, saved contexts, and assembly context switching.
4. Implement per-task address spaces, page tables, faults, and memory objects.
5. Implement per-task capability spaces and complete port/IPC semantics.
6. Integrate scheduler policies, processor sets, synchronization, and time accounting.
7. Implement exception ports, system calls, and controlled machine-state interfaces.
8. Boot the first user task and bootstrap server.
9. Move pagers, device drivers, filesystems, and networking into isolated user-space servers.
10. Add profiling, debugger, NIDL tooling, optional distributed IPC, fuzzing, and hardware qualification.

## Completion criteria

A subsystem is complete only when it has:
- C and/or assembly implementation, with assembly owning hardware entry/exit paths;
- tests for normal, boundary, invalid, concurrent, and failure cases;
- capability and ownership checks on every privileged operation;
- deterministic cleanup and resource accounting;
- ARM64 and x86-64 build coverage;
- boot/runtime validation where hardware behavior is involved;
- documentation of ABI, invariants, and limitations.
