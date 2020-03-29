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

enum klog_level log_level = Log_info;

void klog(enum klog_level lvl, const char *hd, const char *msg) {
    if (lvl <= log_level)
        kprintf("[%s] %s\n", hd, msg);
}

void klogf(enum klog_level lvl, const char *hd, const char *msgf, ...) {
    if (lvl <= log_level){
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
extern void kpanic_do_stop(void);

void kpanic_stop() {
	tty_set_mode(ttym_panic);
	panic_cpid = state.st_curr_pid;
	state.st_proc[PID_STOP].p_stat = RUN;
	proc_set_curr_pid(PID_STOP);
	kpanic_do_stop();
}

void kpanic(const char *msg) {
    kprintf("PANIC!\n");
    kprintf("%s", msg);
	kpanic_stop();
}
void kpanicf(const char *msg, const char *fmt, ...) {
    kprintf("PANIC!\n");
    kprintf("%s\n", msg);
	va_list params;
	va_start(params, fmt);
	vprintf(fmt, params);
	va_end(params);
	kprintf("\n");
	kpanic_stop();
}
void kpanic_ct(const char* msg) {
    vga_init((uint16_t*)(low_addr + VGA_BUFFER));
    vga_writestring("PANIC!\n");
    vga_writestring(msg);
    while(1) halt();
}
void kassert(uint8_t b, const char *msg) {
    if(!b) kpanic(msg);
}
