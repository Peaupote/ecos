include ../fs/proc/list.Makefile

CFILES=main.c \
		kutil.c \
		idt.c \
		gdt.c \
		keyboard.c \
		tty.c \
		tests.c \
		file.c \
		proc/proc.c \
		proc/signal.c \
		syscalls/sleep.c \
		syscalls/sys.c \
		syscalls/sys2.c \
		syscalls/execve.c \
		memory/kmem.c \
		memory/page_alloc.c \
		memory/shared_pages.c \
		memory/shared_ptr.c \
		$(addprefix ../fs/proc/, $(PROCFS_CFILES))

ASFILES=entry.S \
		int.S \
		data.S \
		proc/idle.S \
		proc/init.S \
		syscalls/int7Ecall.S

SRCDEP=$(foreach file,$(ASFILES),"-MT $(file:.S=.s) $(file)") $(CFILES)
OBJ=$(ASFILES:.S=.o) $(CFILES:.c=.o)
