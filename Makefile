ISO=ecos.iso
OS=ecos.bin
AS=i686-elf-as
CC=i686-elf-gcc
FLAGS=-ffreestanding -O2 -nostdlib

.PHONY: all clean tests start re boot

all: $(ISO)

src/kernel/kernel.o:
	$(MAKE) -C src/kernel

boot:
	$(MAKE) -C src/boot

$(OS): boot src/kernel/kernel.o src/linker.ld
	$(CC) -T src/linker.ld -o $(OS) $(FLAGS) src/boot/boot.o src/boot/init.o src/kernel/kernel.o -lgcc
	@echo "Grub check multiboot"
	grub-file --is-x86-multiboot $(OS)

$(ISO): $(OS) src/grub.cfg
	mkdir -p isodir/boot/grub
	cp $(OS) isodir/boot/$(OS)
	cp src/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

tests:
	@echo "Testing"

start: $(OS)
	qemu-system-x86_64 -kernel $(OS)

clean:
	$(MAKE) -C src/kernel clean
	$(MAKE) -C src/boot clean
	rm -rf *.o *.iso *.bin isodir
	rm -f src/*.o


re: clean all
