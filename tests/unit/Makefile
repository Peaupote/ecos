export TRG=test
export CC=gcc
export FLAGS=-g -Wall -Wextra -std=gnu99 -fno-builtin
CFILES=tutil.c
TESTC=pqueue.c page_alloc.c shared_ptr.c
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
	../../tools/mkdep.sh "$(CC) $(FLAGS)" $(TESTDEP) > .depends

include .depends

clean:
	rm -f *.tst *.o *.out .depends

FORCE:
