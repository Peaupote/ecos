#include <kernel/kutil.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <util/string.h>
#include <util/vga.h>
#include <kernel/tty.h>
#include <kernel/int.h>
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

void kpanic(const char *msg) {
    kprintf("PANIC!\n");
    kprintf("%s", msg);
    while(1) halt();
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
