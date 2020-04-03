#ifndef _KUTIL_H
#define _KUTIL_H

#include <stddef.h>
#include <stdarg.h>

#include <libc/stdio.h>
#include <util/string.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define kAssert(P) kassert(P, __FILE__":"STR(__LINE__)" "#P)
#define never_reached kpanic(__FILE__":"STR(__LINE__)\
									" never reached assert");

int  wprintf(stringl_writer, void*, const char* fmt, va_list params);

int  kprintf(const char *format, ...);

enum klog_level {
    Log_error, Log_warn, Log_info, Log_verb
};

void klog (enum klog_level lvl, const char *head, const char *msg);
void klogf(enum klog_level lvl, const char *head, const char *msgf, ...);

__attribute__((noreturn))
void kpanic(const char *msg);

__attribute__((noreturn))
void kpanicf(const char *msg, const char *fmt, ...);

__attribute__((noreturn))
void kpanic_ct(const char *msg);
void kassert(uint8_t, const char *msg);

#endif
