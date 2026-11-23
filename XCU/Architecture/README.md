# Architecture Ports

The architecture ports provide the narrow privileged interface used by portable Nux code.

Each supported architecture must implement equivalent contracts for:

- CPU initialization and per-CPU state;
- exception/interrupt vector installation and dispatch entry;
- interrupt masking and acknowledgement hooks;
- thread context initialization, save, and restore;
- address-space activation and TLB maintenance;
- monotonic counter access and timer programming;
- syscall entry/return.

Do not share assembly between AArch64 and x86-64. Share C-level contracts and semantics, not register layouts or instruction sequences. Every assembly-to-C boundary must document the ABI, stack alignment, preserved registers, clobbers, and interrupt state.
