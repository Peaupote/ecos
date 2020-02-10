ISO=ecos.iso
OS=ecos.bin
AS=i686-elf-as
CC=i686-elf-gcc
FLAGS=-ffreestanding -O2 -nostdlib

all: $(ISO)

src/kernel/kernel.o:
	$(MAKE) -C src/kernel

src/boot.o: src/boot.s
	$(AS) src/boot.s -o src/boot.o

$(OS): src/boot.o src/kernel/kernel.o src/linker.ld
	$(CC) -T src/linker.ld -o $(OS) $(FLAGS) src/boot.o src/kernel/kernel.o -lgcc
	@echo "Grub check multiboot"
	grub-file --is-x86-multiboot $(OS)

$(ISO): $(OS) src/grub.cfg
	mkdir -p isodir/boot/grub
	cp $(OS) isodir/boot/$(OS)
	cp src/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

tests:
	@echo "Testing"

clean:
	$(MAKE) -C src/kernel clean
	rm -rf *.o *.iso *.bin isodir


re: clean all
