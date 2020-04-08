#include <libc/string.h>


char *strtok_r(char *src, const char *delim, char** saveptr) {
    char* it = src ? src : *saveptr;
    //On enlève les délimiteurs au début du token
    if (!*it) return NULL;
    while (*it && index(delim, *it)) ++it;

    char* rt = it;
    while (*it && !index(delim, *it)) ++it;

    if (*it) {
        *it = 0;
        *saveptr = it + 1;
    } else *saveptr = it;

    return rt;
}

char *strtok_rnull(char *src, const char *delim, char** saveptr) {
    char* it = src ? src : *saveptr;
    char   c = *it;
    //On enlève les délimiteurs au début du token
    if (!c) return NULL;
    for (const char* d_it = delim; *d_it; ) {
        if (c == *d_it) {
            if(!*++it) return NULL;
            d_it = delim;
        } else
            ++d_it;
    }

    char* rt = it;
    for (; c; c = *++it) {
        for (const char* d_it = delim; *d_it; ++d_it) {
            if (c == *d_it) {
                *it = '\0';
                *saveptr = it + 1;
                return rt;
            }
        }
    }
    *saveptr = it;
    return rt;
}
