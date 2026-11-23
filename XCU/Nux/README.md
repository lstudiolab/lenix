# Nux Microkernel

Nux is the original microkernel at the center of XCU. It is inspired by Mach-style microkernel concepts—tasks, threads, ports, message-based IPC, and explicit rights—but all implementation code and interfaces are designed for Lenix.

## Kernel responsibilities

Nux retains the mechanisms that require kernel privilege or must be globally coordinated:

- task address-space ownership and lifecycle;
- thread lifecycle and execution state;
- preemptive scheduling and CPU dispatch;
- IPC endpoints, message transport, and rights transfer;
- capability validation;
- virtual-memory primitives and page-fault coordination;
- interrupt and exception dispatch;
- timers and synchronization primitives.

Filesystems, network stacks, device drivers, and higher-level system services are intended to run as isolated user-space servers where practical. A small set of essential platform drivers may be brought up in stages, but this does not change the microkernel boundary.

## Assembly-first hardware interface

Assembly is the primary language for Nux's hardware-facing execution paths. The architecture ports own:

- earliest boot and CPU entry;
- exception-level / privilege-state transitions;
- exception and interrupt vector entry and return;
- register save/restore and thread context switching;
- low-level MMU and control-register operations;
- system-call entry and return;
- atomic primitives only where the C compiler's supported atomics are insufficient or unsuitable.

C owns portable algorithms and policy. Assembly entry stubs must have documented register, stack, alignment, clobber, and return contracts before they are used by C.

## Core object model

- **Task:** resource and address-space container.
- **Thread:** schedulable execution context belonging to one task.
- **Port:** protected IPC endpoint.
- **Message:** bounded, typed IPC payload with optional out-of-line memory and transferred rights.
- **Capability:** unforgeable authority to perform a defined set of operations on a kernel object.
- **Memory object:** kernel-managed backing object that can be mapped into one or more task address spaces.

A task is not a thread, and a port name is not itself authority: access is mediated by rights held in a task's capability space.

## Initial subsystem boundaries

`Core`, `Tasks`, `Threads`, `Scheduler`, `IPC`, `Ports`, `Capabilities`, `Memory`, `VM`, `Interrupts`, `Exceptions`, `Timers`, and `Synchronization` each own their implementation and public internal headers. Cross-subsystem dependencies must be explicit.

## Current status

This directory currently defines the design boundary and initial ABI types. No scheduler, IPC transport, VM subsystem, or boot path is claimed to be implemented yet.
