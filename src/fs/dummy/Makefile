CC?=gcc
FLAGS?=-O2 -Wall -Wextra -std=gnu99
INCLUDES?=

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

.PHONY: clean clean-depends

clean:
	rm -rf $(TRG_LIST)

clean-depends:
	rm -f $(patsubst %,%/.depends,$(TRG_LIST))
