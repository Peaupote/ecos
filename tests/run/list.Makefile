CFILES0=test.c test2.c test3.c test_printf.c test_args.c test_display.c \
		test_proc.c
CFILES_MAT=demo/mat.c demo/fxm.c demo/graph.c
OFILES_MAT=$(CFILES_MAT:.c=.o)
CFILES1=$(CFILES_MAT)
CPFILES=test.c demo/def.mat
CPDIRS=sh
OUT=$(CFILES0:.c=.out) test_reg.out demo/mat.out
SRCDEP=$(CFILES0) $(CFILES1)
CLEANF=$(OUT) $(CFILES1:.c=.o)
