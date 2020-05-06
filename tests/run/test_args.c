#include<stdio.h>

int main(int argc, char* argv[], char* envv[]) {
	printf("argc=%d\n", argc);
	for (int i = 0; i < argc; ++i)
		printf("arg%-2d:'%s'\n", i, argv[i]);
	int envi = 0;
	for (char** ev = envv; *ev; ++ev, ++envi)
		printf("env%-2d:'%s'\n", envi, *ev);
	return 0;
}
