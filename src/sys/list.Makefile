CFILES_0=init1.c cat.c stat.c ls.c ps.c wc.c env.c mkdir.c \
		head.c pwd.c ln.c
CFILES_SH=sh/main.c sh/ast.c sh/parse.c sh/run.c sh/builtin.c
SHFILES=echo.sh kill.sh

OUT=$(CFILES_0:.c=.out) sh.out
CFILES=$(CFILES_0) $(CFILES_SH)
SRCDEP=$(CFILES) "-MT start.s start.S"
