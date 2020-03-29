TEST_SEED=

export TRG=host
export CC=gcc
export FLAGS=-g -Wall -Wextra -std=gnu99 -fno-builtin
ifdef TEST_SEED
	FLAGS:=$(FLAGS) -DTEST_SEED=$(TEST_SEED)
endif
CFILES=tutil.c
TESTC=page_alloc.c shared_ptr.c malloc.c
TESTDEP=$(foreach file,$(TESTC),"-MT $(file:.c=.out) $(file)") $(CFILES)
OBJ=$(CFILES:.c=.o)
TESTRUN=$(TESTC:.c=.out)
INCLUDES=-I ../../include

.PHONY: FORCE clean all run

all:$(TESTRUN) run

%.tst:FORCE
	../../tools/test_section.sh ../../ "$@"

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

%.out:%.c $(OBJ)
	$(CC) $(FLAGS) -o $@ $< $(OBJ) $(INCLUDES)

run:$(TESTRUN)
	$(foreach e,$(TESTRUN),"./$(e)" ;)

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS) $(INCLUDES)" $(TESTDEP) > .depends

include .depends

clean:
	rm -f *.tst *.o *.out .depends

FORCE:
