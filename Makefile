ISO=ecos.iso

.PHONY: all clean tests start re \
		src/boot/boot.bin src/kernel/kernel.bin

all: $(ISO)

src/kernel/kernel.bin:
	$(MAKE) -C src/kernel kernel.bin

src/boot/boot.bin:
	$(MAKE) -C src/boot boot.bin

$(ISO): src/grub.cfg src/boot/boot.bin src/kernel/kernel.bin
	mkdir -p isodir/boot/grub
	cp src/boot/boot.bin isodir/boot/ecos_boot.bin
	cp src/kernel/kernel.bin isodir/boot/ecos_kernel.bin
	cp src/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

tests:
	@echo "Testing"
	$(MAKE) -C tests all

start: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO)

clean:
	$(MAKE) -C src/kernel clean
	$(MAKE) -C src/boot clean
	$(MAKE) -C src/util clean
	$(MAKE) -C src/libc clean
	rm -rf *.o *.iso *.bin isodir

re: clean all
