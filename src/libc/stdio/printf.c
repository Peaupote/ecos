#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void print(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) putchar(str[i]);
}

const char *decimal_digits = "0123456789";
const char *hex_digits     = "0123456789ABCDEF";

static size_t
itoa(int x, const char *digits, size_t base, char *dst, size_t len_dst) {
    if (x == 0) {
        *dst = *digits;
        return 1;
    }

    size_t i;
    for (i = 0; i < len_dst && x > 0; i++, x /= base)
        dst[len_dst - i - 1] = digits[x % base];
    return i;
}

int printf(const char *format, ...) {
    va_list params;
    va_start(params, format);

    int count = 0;
    while(*format) {
        if (format[0] != '%' || format[1] == '%') {
            size_t len = 1;
            if (format[0] == '%') format++;
            while(format[len] && format[len] != '%') len++;
            print(format, len);
            count += len;
            format += len;
            continue;
        }

        // TODO : more format
        if (format[1] == 'c') {
            char c = (char)va_arg(params, int);
            putchar(c);
            count += 1;
        } else if (format[1] == 's') {
            const char *s = va_arg(params, const char*);
            size_t len = strlen(s);
            count += len;
            print(s, len);
        } else if (format[1] == 'd') {
            int x = va_arg(params, int);
            char buf[256] = { 0 };
            size_t len = itoa(x, decimal_digits, 10, buf, 256);
            print(buf + 256 - len, len);
        } else if (format[1] == 'h') {
            int x = va_arg(params, int);
            char buf[256] = { 0 };
            size_t len = itoa(x, hex_digits, 16, buf, 256);
            print(buf + 256 - len, len);
        } else return -1;

        // works for now because all format is made of only one character
        format += 2;
    }

    return count;
}
