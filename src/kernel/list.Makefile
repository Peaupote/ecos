CFILES=main.c \
		kutil.c \
		idt.c \
		gdt.c \
		keyboard.c \
		tty.c \
		tests.c \
		proc.c \
		file.c \
		syscalls/sleep.c \
		syscalls/sys.c \
		syscalls/execve.c \
		memory/kmem.c \
		memory/page_alloc.c \
		memory/shared_pages.c \
		memory/shared_ptr.c \
		../fs/pipe.c \
		../fs/proc/proc.c

ASFILES=entry.S \
		int.S \
		data.S \
		proc/idle.S \
		proc/init.S \
		syscalls/int7Ecall.S

SRCDEP=$(foreach file,$(ASFILES),"-MT $(file:.S=.s) $(file)") $(CFILES)
OBJ=$(ASFILES:.S=.o) $(CFILES:.c=.o)
