TEST_SEED=

include ../vars.Makefile
ifdef TEST_SEED
	FLAGS_AUX:=$(FLAGS_T) -DTEST_SEED=$(TEST_SEED)
else
	FLAGS_AUX=$(FLAGS_T)
endif

ROOT=../../..
CFILES=tutil.c
TESTC=page_alloc.c shared_ptr.c malloc.c
TESTDEP=$(foreach file,$(TESTC),"-MT $(file:.c=.out) $(file)") $(CFILES)
OBJ=$(CFILES:.c=.o)
TESTRUN=$(TESTC:.c=.out)
INCLUDES=-I $(ROOT)/include -I $(ROOT)

.PHONY: FORCE clean all run

all:$(TESTRUN) run

%.tst:FORCE
	$(ROOT)/tools/test_section.sh ../../ "$@"

%.o:%.c
	$(CC) $(FLAGS_AUX) -o $@ -c $<

%.out:%.c $(OBJ)
	$(CC) $(FLAGS_T) -o $@ $< $(OBJ) $(INCLUDES)

run:$(TESTRUN)
	$(foreach e,$(TESTRUN),"./$(e)";)

.depends:
	$(ROOT)/tools/mkdep.sh "$(CC) $(FLAGS) $(INCLUDES)" $(TESTDEP) > .depends

include .depends

clean:
	rm -f *.tst *.o *.out .depends

FORCE:
