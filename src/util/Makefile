TRG_LIST=32 64 test
# Default Target:
TRG?=test
CC?=gcc
FLAGS?=-O2 -Wall -Wextra -std=gnu99
INCLUDES?=
SRCDEP=vga.c string.c

ALLO2?=true
FLAGSO1=-Wall -Wextra -std=gnu99 -nostdlib \
		-mcmodel=large -ffreestanding -mno-red-zone

$(TRG)/%.o:%.c
	mkdir -p $(TRG)
	if $(ALLO2) && [ "$@" = "64/elf64.o" ]; then \
		$(CC) $(FLAGSO1) -o $@ -c $< $(INCLUDES); \
	else \
		$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES); \
	fi

$(TRG)/.depends:
	mkdir -p $(TRG)
	../../tools/mkdep.sh "$(CC) $(FLAGS)"\
		$(foreach file,$(SRCDEP),"-MT $(TRG)/$(file) $(file)")\
		> $(TRG)/.depends

include $(TRG)/.depends

.PHONY: clean clean-depends

clean:
	rm -rf $(TRG_LIST)

clean-depends:
	rm -f $(patsubst %,%/.depends,$(TRG_LIST))
