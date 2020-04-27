export TEST_SEED?=

ROOT=../../..
include ../vars.Makefile

TESTC=page_alloc.c shared_ptr.c malloc.c
TESTDEP=$(foreach file,$(TESTC),"-MT $(file:.c=.out) $(file)") $(CFILES)
TESTRUN=$(TESTC:.c=.out)
INCLUDES=-I $(ROOT)/include -I $(ROOT)

.PHONY: FORCE clean all run tutil

all:$(TESTRUN) run

%.tst:FORCE
	@$(ROOT)/tools/test_section.sh ../../ "$@"

%.out:%.c tutil
	@$(CC) $(FLAGS_TU) -o $@ $< ../tutil.o $(INCLUDES)

tutil:
	@$(MAKE) -C ../ tutil.o

run:$(TESTRUN)
	$(foreach e,$(TESTRUN),"./$(e)" &&) echo done

.depends:
	@$(ROOT)/tools/mkdep.sh "$(CC) $(FLAGS) $(INCLUDES)" $(TESTDEP) > .depends

include .depends

clean:
	rm -f *.tst *.o *.out .depends

FORCE:
