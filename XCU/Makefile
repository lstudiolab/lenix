HOST_OS := $(shell uname -s)
ifeq ($(HOST_OS),Darwin)
CC_X86 ?= clang --target=x86_64-elf
AS_X86 ?= clang --target=x86_64-elf
LD_X86 ?= clang --target=x86_64-elf -fuse-ld=lld
CC_ARM ?= clang --target=aarch64-elf
AS_ARM ?= clang --target=aarch64-elf
LD_ARM ?= clang --target=aarch64-elf -fuse-ld=lld
HOST_CC ?= clang
else
CC_X86 ?= x86_64-linux-gnu-gcc
AS_X86 ?= x86_64-linux-gnu-gcc
LD_X86 ?= x86_64-linux-gnu-gcc
CC_ARM ?= aarch64-linux-gnu-gcc
AS_ARM ?= aarch64-linux-gnu-gcc
LD_ARM ?= aarch64-linux-gnu-gcc
HOST_CC ?= cc
endif

COMMON_CFLAGS := -DNUX_KERNEL -std=gnu11 -ffreestanding -fno-stack-protector -fno-pic -fno-pie -fno-builtin -fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Wextra -Werror -O2 -I Nux/Include -I Kernel/Boot -I Kernel/Entry
X86_CFLAGS := $(COMMON_CFLAGS) -march=x86-64 -mtune=generic -mno-bmi -mno-bmi2 -mno-sse -mno-mmx -msoft-float
LDFLAGS := -nostdlib -static -no-pie -Wl,--build-id=none
C_SOURCES := Kernel/Boot/limine.c Nux/Runtime/boot.c Kernel/Initialization/main.c Nux/Memory/physical.c Nux/Task/task.c Nux/Task/thread.c Nux/Capability/capability.c Nux/CPU/exception.c Nux/CPU/acpi.c Nux/CPU/processor.c Nux/Runtime/memory.c Nux/IPC/port.c Nux/Sync/sync.c Nux/Syscall/syscall.c Nux/VM/vm.c Nux/VM/page_table.c Nux/Time/time.c Nux/Time/clock_arch.c

.PHONY: all headers test-capability test-acpi test-processor x86_64 aarch64 aarch64-qemu iso iso-x86_64 iso-aarch64 uefi-disk uefi-disk-x86_64 uefi-disk-aarch64 uefi-disk-aarch64-qemu test-boot test-runtime test-ipc test-memory test-sync test-thread test-syscall test-vm test-time clean
all: headers test-capability test-acpi test-processor x86_64 aarch64 test-boot test-runtime test-ipc test-memory test-sync test-thread test-syscall test-vm test-time

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
	$(LD_X86) $(LDFLAGS) -Wl,-T,Kernel/Linker/x86_64.ld -o Build/x86_64/nux.elf Build/x86_64/*.o

aarch64:
	mkdir -p Build/aarch64
	@for source in $(C_SOURCES); do object=Build/aarch64/$$(echo $$source | tr '/.' '__').o; $(CC_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c $$source -o $$object || exit 1; done
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/Boot/entry.S -o Build/aarch64/entry.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/early.S -o Build/aarch64/early.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/lock.S -o Build/aarch64/lock.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/exception.S -o Build/aarch64/exception.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/context.S -o Build/aarch64/context.o
	$(LD_ARM) $(LDFLAGS) -Wl,-T,Kernel/Linker/aarch64.ld -o Build/aarch64/nux.elf Build/aarch64/*.o

test-processor:
	mkdir -p Build
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -DNUX_PROCESSOR_TEST -DNUX_PROCESSOR_TEST_X86_64 -I Nux/Include Nux/Tests/processor_test.c Nux/CPU/processor.c -o Build/processor_test
	./Build/processor_test
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -DNUX_PROCESSOR_TEST -DNUX_PROCESSOR_TEST_ARM64 -I Nux/Include Nux/Tests/processor_arm_test.c Nux/CPU/processor.c -o Build/processor_arm_test
	./Build/processor_arm_test

test-capability:
	mkdir -p Build
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -I Nux/Include Nux/Tests/capability_test.c Nux/Capability/capability.c -o Build/capability_test
	./Build/capability_test

test-acpi:
	mkdir -p Build
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -I Nux/Include Nux/Tests/acpi_test.c Nux/CPU/acpi.c -o Build/acpi_test
	./Build/acpi_test

test-boot:
	mkdir -p Build
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -I Nux/Include Nux/Tests/boot_test.c Nux/Runtime/boot.c -o Build/boot_test
	./Build/boot_test

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

ifeq ($(HOST_OS),Darwin)
test-thread:
	@echo "Native context-switch execution test is skipped on Darwin; both architecture context-switch objects are cross-built."
else
test-thread:
	mkdir -p Build
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -fno-builtin -fno-stack-protector -I Nux/Include Nux/Tests/thread_context_test.c Nux/Task/thread.c Nux/Task/task.c Nux/Runtime/memory.c Architecture/X86_64/CPU/context.S -o Build/thread_context_test
	./Build/thread_context_test
endif

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

LIMINE_VERSION := 12.11.1
LIMINE_SHA256 := 953aba25c23abc6d6b4e85e20ec8b9f3a8605450a26997103fa3fddf00e658e6
LIMINE_ARCHIVE := Build/limine-binary.tar.gz
LIMINE_DIR := Build/limine-binary

$(LIMINE_ARCHIVE):
	mkdir -p Build
	curl -fL -o $@ https://github.com/Limine-Bootloader/Limine/releases/download/v$(LIMINE_VERSION)/limine-binary.tar.gz
	printf '%s  %s\n' '$(LIMINE_SHA256)' '$@' | shasum -a 256 -c -

$(LIMINE_DIR)/limine: $(LIMINE_ARCHIVE)
	rm -rf $(LIMINE_DIR)
	tar -xzf $(LIMINE_ARCHIVE) -C Build
	$(MAKE) -C $(LIMINE_DIR) CC="$(HOST_CC)"

iso: iso-x86_64 iso-aarch64

iso-x86_64: x86_64 $(LIMINE_DIR)/limine
	rm -rf Build/iso-x86_64
	mkdir -p Build/iso-x86_64/boot/limine Build/iso-x86_64/EFI/BOOT
	cp limine.conf Build/iso-x86_64/boot/limine/limine.conf
	cp Build/x86_64/nux.elf Build/iso-x86_64/boot/nux.elf
	cp $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin $(LIMINE_DIR)/limine-uefi-cd.bin Build/iso-x86_64/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI Build/iso-x86_64/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label Build/iso-x86_64 -o Build/lenix-x86_64.iso
	$(LIMINE_DIR)/limine bios-install Build/lenix-x86_64.iso

iso-aarch64: aarch64 $(LIMINE_DIR)/limine
	rm -rf Build/iso-aarch64
	mkdir -p Build/iso-aarch64/boot/limine Build/iso-aarch64/EFI/BOOT
	cp limine.conf Build/iso-aarch64/boot/limine/limine.conf
	cp Build/aarch64/nux.elf Build/iso-aarch64/boot/nux.elf
	cp $(LIMINE_DIR)/limine-uefi-cd.bin Build/iso-aarch64/boot/limine/
	cp $(LIMINE_DIR)/BOOTAA64.EFI Build/iso-aarch64/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -hfsplus -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label Build/iso-aarch64 -o Build/lenix-aarch64.iso

uefi-disk: uefi-disk-x86_64 uefi-disk-aarch64

uefi-disk-x86_64: x86_64 $(LIMINE_DIR)/limine
	rm -f Build/lenix-x86_64-uefi.img
	dd if=/dev/zero of=Build/lenix-x86_64-uefi.img bs=1048576 count=128
	mformat -i Build/lenix-x86_64-uefi.img -F ::
	mmd -i Build/lenix-x86_64-uefi.img ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i Build/lenix-x86_64-uefi.img $(LIMINE_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i Build/lenix-x86_64-uefi.img limine.conf ::/limine.conf
	mcopy -i Build/lenix-x86_64-uefi.img limine.conf ::/boot/limine/limine.conf
	mcopy -i Build/lenix-x86_64-uefi.img Build/x86_64/nux.elf ::/boot/nux.elf

uefi-disk-aarch64: aarch64 $(LIMINE_DIR)/limine
	rm -f Build/lenix-aarch64-uefi.img
	dd if=/dev/zero of=Build/lenix-aarch64-uefi.img bs=1M count=128
	mformat -i Build/lenix-aarch64-uefi.img -F ::
	mmd -i Build/lenix-aarch64-uefi.img ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i Build/lenix-aarch64-uefi.img $(LIMINE_DIR)/BOOTAA64.EFI ::/EFI/BOOT/BOOTAA64.EFI
	mcopy -i Build/lenix-aarch64-uefi.img limine.conf ::/limine.conf
	mcopy -i Build/lenix-aarch64-uefi.img limine.conf ::/boot/limine/limine.conf
	mcopy -i Build/lenix-aarch64-uefi.img Build/aarch64/nux.elf ::/boot/nux.elf

aarch64-qemu:
	mkdir -p Build/aarch64-qemu
	@for source in $(C_SOURCES); do object=Build/aarch64-qemu/$$(echo $$source | tr '/.' '__').o; $(CC_ARM) $(COMMON_CFLAGS) -DNUX_QEMU_SEMIHOST -mgeneral-regs-only -c $$source -o $$object || exit 1; done
	$(AS_ARM) $(COMMON_CFLAGS) -DNUX_QEMU_SEMIHOST -mgeneral-regs-only -c Architecture/ARM64/Boot/entry.S -o Build/aarch64-qemu/entry.o
	$(AS_ARM) $(COMMON_CFLAGS) -DNUX_QEMU_SEMIHOST -mgeneral-regs-only -c Architecture/ARM64/CPU/early.S -o Build/aarch64-qemu/early.o
	$(AS_ARM) $(COMMON_CFLAGS) -DNUX_QEMU_SEMIHOST -mgeneral-regs-only -c Architecture/ARM64/CPU/lock.S -o Build/aarch64-qemu/lock.o
	$(AS_ARM) $(COMMON_CFLAGS) -DNUX_QEMU_SEMIHOST -mgeneral-regs-only -c Architecture/ARM64/CPU/exception.S -o Build/aarch64-qemu/exception.o
	$(AS_ARM) $(COMMON_CFLAGS) -DNUX_QEMU_SEMIHOST -mgeneral-regs-only -c Architecture/ARM64/CPU/context.S -o Build/aarch64-qemu/context.o
	$(LD_ARM) $(LDFLAGS) -Wl,-T,Kernel/Linker/aarch64.ld -o Build/aarch64-qemu/nux.elf Build/aarch64-qemu/*.o

uefi-disk-aarch64-qemu: aarch64-qemu $(LIMINE_DIR)/limine
	rm -f Build/lenix-aarch64-qemu-uefi.img
	dd if=/dev/zero of=Build/lenix-aarch64-qemu-uefi.img bs=1048576 count=128
	mformat -i Build/lenix-aarch64-qemu-uefi.img -F ::
	mmd -i Build/lenix-aarch64-qemu-uefi.img ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i Build/lenix-aarch64-qemu-uefi.img $(LIMINE_DIR)/BOOTAA64.EFI ::/EFI/BOOT/BOOTAA64.EFI
	mcopy -i Build/lenix-aarch64-qemu-uefi.img limine.conf ::/limine.conf
	mcopy -i Build/lenix-aarch64-qemu-uefi.img limine.conf ::/boot/limine/limine.conf
	mcopy -i Build/lenix-aarch64-qemu-uefi.img Build/aarch64-qemu/nux.elf ::/boot/nux.elf
