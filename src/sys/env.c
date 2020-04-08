#include <stdio.h>

int main(int argc __attribute__((unused)),
         char *argv[] __attribute__((unused)), char *env[]) {
    char **ptr = env;
    while (*ptr) {
        printf("%s\n", *ptr++);
    }

    return 0;
}
