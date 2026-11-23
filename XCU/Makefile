CC_X86 ?= x86_64-linux-gnu-gcc
AS_X86 ?= x86_64-linux-gnu-gcc
CC_ARM ?= aarch64-linux-gnu-gcc
AS_ARM ?= aarch64-linux-gnu-gcc
HOST_CC ?= cc

COMMON_CFLAGS := -std=gnu11 -ffreestanding -fno-stack-protector -fno-pic -fno-pie -fno-builtin -fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Wextra -Werror -O2 -I Nux/Include -I Kernel/Boot -I Kernel/Entry
X86_CFLAGS := $(COMMON_CFLAGS) -mno-sse -mno-mmx -msoft-float
LDFLAGS := -nostdlib -static -no-pie -Wl,--build-id=none
C_SOURCES := Kernel/Boot/limine.c Nux/Runtime/boot.c Kernel/Initialization/main.c Nux/Memory/physical.c Nux/Task/task.c Nux/Task/thread.c Nux/Capability/capability.c Nux/CPU/exception.c Nux/Runtime/memory.c Nux/IPC/port.c Nux/Sync/sync.c Nux/Syscall/syscall.c Nux/VM/vm.c Nux/VM/page_table.c Nux/Time/time.c Nux/Time/clock_arch.c

.PHONY: all headers x86_64 aarch64 test-runtime test-ipc test-memory test-sync test-thread test-syscall test-vm test-time clean
all: headers x86_64 aarch64 test-runtime test-ipc test-memory test-sync test-thread test-syscall test-vm test-time

headers:
	$(CC_X86) $(COMMON_CFLAGS) -fsyntax-only Nux/Tests/header_check.c
	$(CC_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -fsyntax-only Nux/Tests/header_check.c

x86_64:
	mkdir -p Build/x86_64
	@for source in $(C_SOURCES); do object=Build/x86_64/$$(echo $$source | tr '/.' '__').o; $(CC_X86) $(X86_CFLAGS) -mno-red-zone -mcmodel=kernel -c $$source -o $$object || exit 1; done
	$(AS_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/Boot/entry.S -o Build/x86_64/entry.o
	$(AS_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/CPU/early.S -o Build/x86_64/early.o
	$(AS_X86) $(X86_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/CPU/lock.S -o Build/x86_64/lock.o
	$(AS_X86) $(X86_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/CPU/exception.S -o Build/x86_64/exception.o
	$(AS_X86) $(X86_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/CPU/context.S -o Build/x86_64/context.o
	$(CC_X86) $(LDFLAGS) -Wl,-T,Kernel/Linker/x86_64.ld -o Build/x86_64/nux.elf Build/x86_64/*.o

aarch64:
	mkdir -p Build/aarch64
	@for source in $(C_SOURCES); do object=Build/aarch64/$$(echo $$source | tr '/.' '__').o; $(CC_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c $$source -o $$object || exit 1; done
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/Boot/entry.S -o Build/aarch64/entry.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/early.S -o Build/aarch64/early.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/lock.S -o Build/aarch64/lock.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/exception.S -o Build/aarch64/exception.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/context.S -o Build/aarch64/context.o
	$(CC_ARM) $(LDFLAGS) -Wl,-T,Kernel/Linker/aarch64.ld -o Build/aarch64/nux.elf Build/aarch64/*.o

test-runtime:
	mkdir -p Build
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -fno-builtin -I Nux/Include Nux/Tests/runtime_test.c Nux/Runtime/memory.c -o Build/runtime_test
	./Build/runtime_test

test-ipc:
	mkdir -p Build
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -fno-builtin -I Nux/Include Nux/Tests/ipc_test.c Nux/IPC/port.c Nux/Capability/capability.c Nux/Task/task.c Nux/Runtime/memory.c -o Build/ipc_test
	./Build/ipc_test

test-memory:
	mkdir -p Build
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -fno-builtin -I Nux/Include -I Kernel/Entry Nux/Tests/memory_test.c Nux/Memory/physical.c Nux/Runtime/memory.c -o Build/memory_test
	./Build/memory_test

test-sync:
	mkdir -p Build
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -fno-builtin -I Nux/Include Nux/Tests/sync_test.c Nux/Sync/sync.c -o Build/sync_test
	./Build/sync_test

test-thread:
	mkdir -p Build
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -fno-builtin -fno-stack-protector -mno-sse -mno-mmx -msoft-float -mno-red-zone -I Nux/Include Nux/Tests/thread_context_test.c Nux/Task/thread.c Nux/Task/task.c Nux/Runtime/memory.c Nux/Time/time.c Nux/Time/clock_arch.c Architecture/X86_64/CPU/context.S -o Build/thread_context_test
	./Build/thread_context_test

test-syscall:
	mkdir -p Build
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -fno-builtin -I Nux/Include Nux/Tests/syscall_test.c Nux/Syscall/syscall.c -o Build/syscall_test
	./Build/syscall_test

test-vm:
	mkdir -p Build
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -fno-builtin -I Nux/Include Nux/Tests/vm_test.c Nux/VM/vm.c Nux/Task/task.c Nux/Runtime/memory.c -o Build/vm_test
	./Build/vm_test

test-time:
	mkdir -p Build
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -fno-builtin -I Nux/Include Nux/Tests/time_test.c Nux/Time/time.c -o Build/time_test
	./Build/time_test

clean:
	rm -rf Build
