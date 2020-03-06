#ifndef _KUTIL_H
#define _KUTIL_H

int  kprintf(const char *format, ...);

enum klog_level {
	Log_error, Log_warn, Log_info, Log_verb
};

void klog (enum klog_level lvl, const char *head, const char *msg);
void klogf(enum klog_level lvl, const char *head, const char *msgf, ...);

void kpanic(const char *msg);

#endif
