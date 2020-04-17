#include <kernel/kutil.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <util/string.h>
#include <util/vga.h>
#include <kernel/tty.h>
#include <kernel/int.h>
#include <kernel/proc.h>
#include <kernel/memory/kmem.h> //low_addr

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

enum klog_level klog_level = Log_error;
char klog_filtr[256] = "";

bool klog_pass_filtr(enum klog_level lvl, const char* hd) {
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

void kpanic(const char *msg) {
    kprintf("PANIC! cpid=%d\n", (int)state.st_curr_pid);
    kprintf("%s", msg);
    kpanic_stop();
}
void kpanicf(const char *msg, const char *fmt, ...) {
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
