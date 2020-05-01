#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
	int argi = 1;
	bool itp = false;
	bool nwl = true;

	for (; argi < argc; ++argi) {
		char* arg = argv[argi];
		if (arg[0] == '-' && arg[1] && !arg[2]) {
			switch (arg[1]) {
				case 'e': itp = true;  continue;
				case 'E': itp = false; continue;
				case 'n': nwl = false; continue;
			}
		}
		break;
	}

	for (int mi0 = argi; argi < argc; ++argi) {
		if (argi > mi0) printf(" ");
		if (itp) {
			for (char *c = argv[argi]; *c; ++c) {
				if (*c == '\\') {
					switch (*++c) {
						case 'n':  fputc('\n', stdout);    break;
						case 't':  fputc('\t', stdout);    break;
						case 'e':  fputc('\033', stdout);  break;
						case '\\': fputc('\\', stdout);    break;
						default: --c; fputc('\\', stdout); break;
					}
				} else fputc(*c, stdout);
			}
		} else
			printf("%s", argv[argi]);
	}

	if (nwl) printf("\n");
    return 0;
}
