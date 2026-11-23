# Kernel Entry and Boot

The first Nux boot milestone uses the Limine boot protocol on both supported targets. Limine supports x86-64 and AArch64 and provides a documented 64-bit entry ABI. The kernel requests protocol revision 6 and obtains the memory map and higher-half direct-map offset.

The architecture-specific `_start` symbols are assembly. They mask interrupts, establish the C-call boundary, collect the initial boot handoff fields, and call the portable C initialization function.

This is deliberately only an early-entry milestone. It does not yet install exception tables, take ownership of page tables, initialize physical memory allocation, start secondary CPUs, or schedule user tasks. Those steps must happen before Nux can be considered a functioning microkernel.

Build with an appropriate freestanding cross-toolchain from `XCU/` using `make x86_64` or `make aarch64`. The default cross-compiler names can be overridden on the make command line.
