export CC=x86_64-elf-gcc
export AS=x86_64-elf-as
ROOT=../..
include $(ROOT)/vars.Makefile

CFILES=init1.c sh.c cat.c echo.c stat.c ls.c ps.c test.c wc.c
OUT=$(CFILES:.c=.out)
SRCDEP=$(CFILES) start.S
INCLUDE_LIBC?=-I ../../include/libc
INCLUDES=-I ../../include $(INCLUDE_LIBC)

.PHONY: all clean libc

%.o:%.S

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.o:%.s
	$(AS) $< -o $@ $(INCLUDES)

%.out:%.c libc start.o
	$(CC) $(FLAGS_UR) -o $@ $< start.o ../libc/libc.a $(INCLUDES) -lgcc

all:$(OUT)

libc:
	$(MAKE) -C ../libc libc.a

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_U) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	rm -f *.out *.s *.o .depends
