#ifndef _KUTIL_H
#define _KUTIL_H

#include <stddef.h>
#include <stdarg.h>

#include "../util/string.h"

int  wprintf(stringl_writer, void*, const char* fmt, va_list params);

int  kprintf(const char *format, ...);

enum klog_level {
    Log_error, Log_warn, Log_info, Log_verb
};

void klog (enum klog_level lvl, const char *head, const char *msg);
void klogf(enum klog_level lvl, const char *head, const char *msgf, ...);

void kpanic(const char *msg);
void kpanic_ct(const char *msg);
void kassert(uint8_t, const char *msg);

void memcpy(void *dst, const void *src, size_t len);

#endif
