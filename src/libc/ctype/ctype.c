#include <libc/ctype.h>

int isspace(int c) {
	switch(c) {
		case  ' ': case '\f': case '\n': case '\r':
		case '\t': case '\v':
			return 1;
	}
	return 0;
}

int isdigit(int c) {
	return '0' <= c && c <= '9';
}

int isalpha(int c) {
	return ('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z');
}

int isalnum(int c) {
	return ('0' <= c && c <= '9')
		|| ('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z');
}
