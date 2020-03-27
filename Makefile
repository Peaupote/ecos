ISO=ecos.iso

.PHONY: all clean tests start re src/kernel/kernel.bin src/boot/boot.bin \
	depends src/libc/libc.a

all: $(ISO)

src/kernel/kernel.bin:
	$(MAKE) -C src/kernel kernel.bin

src/boot/boot.bin:
	$(MAKE) -C src/boot boot.bin

src/libc/libc.a:
	$(MAKE) -C src/libc libc.a

$(ISO): src/grub.cfg src/boot/boot.bin src/kernel/kernel.bin src/libc/libc.a
	mkdir -p isodir/boot/grub
	cp src/boot/boot.bin isodir/boot/ecos_boot.bin
	cp src/kernel/kernel.bin isodir/boot/ecos_kernel.bin
	cp src/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

tests:
	@echo "Testing"
	$(MAKE) -C tests all
	$(MAKE) -C tests/unit run
	$(MAKE) -C tests/libc run
	# $(MAKE) -C tests/ext2 run

start: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -monitor stdio | tee qemu.out

depends:
	$(MAKE) -B -C src/kernel         .depends
	$(MAKE) -B -C src/kernel/aux_prg .depends
	$(MAKE) -B -C src/boot           .depends
	$(MAKE) -B -C src/util      clean-depends
	$(MAKE) -B -C src/libc           .depends
	$(MAKE) -B -C tests              .depends

clean:
	$(MAKE) -C src/kernel clean
	$(MAKE) -C src/boot   clean
	$(MAKE) -C src/util   clean
	$(MAKE) -C src/libc   clean
	$(MAKE) -C src/fs     clean
	$(MAKE) -C tests      clean
	rm -rf *.o *.iso *.bin *.out isodir

re: clean all
