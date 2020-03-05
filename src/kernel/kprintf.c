#include <stdarg.h>

#include "../util/string.h"
#include "tty.h"

// kprintf

static char buf[256];
static const char *decimal_digits = "0123456789";
static const char *hex_digits     = "0123456789ABCDEF";

static size_t
itoa(int x, const char *digits, size_t base) {
    if (x == 0) {
        buf[255] = *digits;
        buf[256] = 0;
        return 1;
    }

    size_t i;
    for (i = 0; i < 256 && x > 0; i++, x /= base)
        buf[256 - i - 1] = digits[x % base];

    buf[256] = 0;
    return i;
}

static void cpy(const void *src, void *dst, size_t len) {
    uint8_t *s = (uint8_t*)src;
    uint8_t *d = (uint8_t*)dst;
    for (size_t i = 0; i < len; i++) *d++ = *s++;
}

int kprintf(const char *format, ...) {
    va_list params;
    va_start(params, format);

    size_t idx_b = tty_buffer_next_idx();
    int count = 0;
    int shift = 0;
    while(*format) {
        if (format[0] != '%' || format[1] == '%') {
            size_t len = 1;
            if (format[0] == '%') format++;
            while(format[len] && format[len] != '%') len++;
            cpy(format, buf, len);
            buf[len] = 0;
            shift += tty_writestring(buf);
            count += len;
            format += len;
            continue;
        }

        // TODO : more format
        if (format[1] == 'c') {
            char c = (char)va_arg(params, int);
            buf[0] = c; buf[1] = 0;
            shift += tty_writestring(buf);
            count += 1;
        } else if (format[1] == 's') {
            const char *s = va_arg(params, const char*);
            size_t len = ustrlen(s);
            count += len;
            shift += tty_writestring(s);
        } else if (format[1] == 'd') {
            int x = va_arg(params, int);
            size_t len = itoa(x, decimal_digits, 10);
            shift += tty_writestring(buf + 256 - len);
            count += len;
        } else if (format[1] == 'h') {
            int x = va_arg(params, int);
            size_t len = itoa(x, hex_digits, 16);
            shift += tty_writestring(buf + 256 - len);
            count += len;
        } else return -1;

        // works for now because all format is made of only one character
        format += 2;
    }

    if (shift) tty_afficher_buffer_all();
    else tty_afficher_buffer_range(idx_b, tty_buffer_next_idx());

    return count;
}
