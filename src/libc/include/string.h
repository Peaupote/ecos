#ifndef _H_STRING
#define _H_STRING

#include <stddef.h>

int   memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);

/**
 * Copy a string from src to dest, returning a pointer
 * to the end of the resulting string at dest.
 */
char *stpcpy(char *dest, const char *src);

/**
 * Append the string src to the string dest, returning a pointer dest.
 */
char *strcat(char *dest, const char *src);

/**
 * Return a pointer to the first occurrence of the character c in the string s.
 */
char *strchr(const char *s, int c);

/**
 * Compare the strings s1 with s2.
 */
int strcmp(const char *s1, const char *s2);

/**
 * Copy the string src to dest, returning a pointer to the start of dest.
 */
char *strcpy(char *dest, const char *src);

/**
 * Calculate the length of the initial segment of the string s
 * which does not contain any of bytes in the string reject,
 */
size_t strcspn(const char *s, const char *reject);

/**
 * Return a duplicate of the string s in memory allocated using malloc(3).
 */
char *strdup(const char *s);

/**
 * Randomly swap the characters in string.
 */
char *strfry(char *string);

/**
 * Return the length of the string s.
 */
size_t strlen(const char *s);

/**
 * Append at most n bytes from the string src to the string dest,
 * returning a pointer to dest.
 */
char *strncat(char *dest, const char *src, size_t n);

/**
 * Compare at most n bytes of the strings s1 and s2.
 */
int strncmp(const char *s1, const char *s2, size_t n);

/**
 * Copy at most n bytes from string src to dest,
 * returning a pointer to the start of dest.
 */
char *strncpy(char *dest, const char *src, size_t n);


#endif
