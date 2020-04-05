ISO=ecos.iso

.PHONY: all clean tests start re src/kernel/kernel.bin src/boot/boot.bin \
	depends src/libc/libc.a tools

all: $(ISO)

src/kernel/kernel.bin:
	$(MAKE) -C src/kernel kernel.bin

src/boot/boot.bin:
	$(MAKE) -C src/boot boot.bin

src/libc/libc.a:
	$(MAKE) -C src/libc libc.a

$(ISO): src/grub.cfg src/boot/boot.bin src/kernel/kernel.bin \
		src/libc/libc.a tools
	mkdir -p isodir/boot/grub
	cp src/boot/boot.bin isodir/boot/ecos_boot.bin
	./tools/check_elf.sh
	cp src/kernel/kernel.bin isodir/boot/ecos_kernel.bin
	cp src/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

tools:
	make -C tools all

tests:
	@echo "Testing"
	$(MAKE) -C tests/host all

start: $(ISO)
	@echo "QEMU RUN"
	qemu-system-x86_64 -cdrom $(ISO) -monitor stdio | tee qemu.out

depends:
	$(MAKE) -B -C src/kernel         .depends
	$(MAKE) -B -C src/kernel/aux_prg .depends
	$(MAKE) -B -C src/boot           .depends
	$(MAKE) -B -C src/util      clean-depends
	$(MAKE) -B -C src/libc           .depends
	$(MAKE)    -C tests/host          depends
	$(MAKE) -B -C tools              .depends

clean:
	$(MAKE) -C src/kernel clean
	$(MAKE) -C src/boot   clean
	$(MAKE) -C src/util   clean
	$(MAKE) -C src/libc   clean
	$(MAKE) -C src/fs     clean
	$(MAKE) -C src/sys    clean
	$(MAKE) -C tests/host clean
	$(MAKE) -C tests/run  clean
	$(MAKE) -C tools      clean
	rm -rf *.o *.iso *.bin *.out isodir

re: clean all
