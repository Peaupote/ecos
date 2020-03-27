TRG_LIST=32 64 test
# Default Target:
TRG?=test
CC?=gcc
FLAGS?=-O2 -Wall -Wextra -std=gnu99
INCLUDES=-I ../../include
SRCDEP=vga.c string.c elf64.c

$(TRG)/%.o:%.c
	mkdir -p $(TRG)
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

$(TRG)/.depends:
	mkdir -p $(TRG)
	../../tools/mkdep.sh "$(CC) $(FLAGS) $(INCLUDES)"\
		$(foreach file,$(SRCDEP),"-MT $(TRG)/$(file) $(file)")\
		> $(TRG)/.depends

include $(TRG)/.depends

.PHONY: clean clean-depends

clean:
	rm -rf $(TRG_LIST)

clean-depends:
	rm -f $(patsubst %,%/.depends,$(TRG_LIST))
