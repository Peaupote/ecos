export CC=x86_64-elf-gcc
export AS=x86_64-elf-as
ROOT=../..
include $(ROOT)/vars.Makefile

CFILES_0=init1.c cat.c echo.c stat.c ls.c ps.c wc.c env.c mkdir.c \
		head.c pwd.c test.c test2.c test3.c
CFILES_SH=sh/main.c sh/ast.c sh/parse.c sh/run.c
CFILES=$(CFILES_0) $(CFILES_SH)
OUT=$(CFILES_0:.c=.out) sh.out
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

sh.out:sh/sh.h $(CFILES_SH) libc start.o
	$(CC) $(FLAGS_UR) -o $@ $(CFILES_SH) $< start.o \
		../libc/libc.a $(INCLUDES) -lgcc

all:$(OUT)

libc:
	$(MAKE) -C ../libc libc.a

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_U) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	rm -f *.out *.s *.o .depends
