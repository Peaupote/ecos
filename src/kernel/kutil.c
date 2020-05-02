#include <kernel/kutil.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <util/vga.h>
#include <kernel/tty.h>
#include <kernel/int.h>
#include <kernel/proc.h>

// kprintf

int vprintf(const char *fmt, va_list ps) {
    tty_seq_t sq;
    tty_seq_init(&sq);
    int cnt = fpprintf(&tty_seq_write, &sq, fmt, ps);
    tty_seq_commit(&sq);
    return cnt;
}

int kprintf(const char *format, ...) {
    va_list params;
    int count;
    va_start(params, format);
    count = vprintf(format, params);
    va_end(params);
    return count;
}

enum klog_level klog_level = Log_early;
char klog_filtr[256] = "";
int  nb_early_error  = 0;
bool kpanic_is_early = true;

bool klog_pass_filtr(enum klog_level lvl, const char* hd) {
	if (klog_level == Log_early && lvl <= Log_error) ++nb_early_error;
	return lvl <= klog_level
		&& (*klog_filtr == '\0' || !strcmp(hd, klog_filtr));
}

void klog(enum klog_level lvl, const char *hd, const char *msg) {
    if (klog_pass_filtr(lvl, hd))
        kprintf("[%s] %s\n", hd, msg);
}

void klogf(enum klog_level lvl, const char *hd, const char *msgf, ...) {
    if (klog_pass_filtr(lvl, hd)) {
        kprintf("[%s] ", hd);
        va_list params;
        va_start(params, msgf);
        vprintf(msgf, params);
        va_end(params);
        kprintf("\n");
    }
}

pid_t    panic_cpid;
uint_ptr panic_rsp;

__attribute__((noreturn))
extern void kpanic_do_stop(void);

__attribute__((noreturn))
void kpanic_stop() {
    tty_set_mode(ttym_panic);
    panic_cpid = state.st_curr_pid;
    state.st_proc[PID_STOP].p_stat = RUN;
    proc_set_curr_pid(PID_STOP);
    kpanic_do_stop();
}

__attribute__((noreturn))
void kpanic_early() {
	while(true) halt();
}

void kpanic(const char *msg) {
	if (kpanic_is_early) kpanic_early();
    kprintf("PANIC! cpid=%d\n", (int)state.st_curr_pid);
    kprintf("%s", msg);
    kpanic_stop();
}
void kpanicf(const char *msg, const char *fmt, ...) {
	if (kpanic_is_early) kpanic_early();
    kprintf("PANIC! cpid=%d\n", (int)state.st_curr_pid);
    kprintf("%s\n", msg);
    va_list params;
    va_start(params, fmt);
    vprintf(fmt, params);
    va_end(params);
    kprintf("\n");
    kpanic_stop();
}
void kassert(uint8_t b, const char *msg) {
    if(!b) kpanic(msg);
}
