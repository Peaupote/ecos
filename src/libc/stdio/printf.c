#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <libc/stdio.h>
#include <libc/string.h>

static char buf[256];
static const char *decimal_digits = "0123456789";
static const char *hex_digits     = "0123456789ABCDEF";

static size_t
itoa(long long int x, const char *digits, size_t base) {
    if (x == 0) {
        buf[254] = digits[0];
        buf[255] = 0;
        return 1;
    }

    size_t i;
    uint8_t sign = 0;
    if (x < 0) {
        sign = 1;
        x = -x;
    }
    for (i = 0; i < 256 && x > 0; i++, x /= base)
        buf[255 - i - 1] = digits[x % base];

    buf[255] = 0;
    if (sign) buf[255 - i++ - 1] = '-';
    return i;
}

static size_t
ultoa(uint64_t x, const char *digits, size_t base) {
    if (x == 0) {
        buf[254] = digits[0];
        buf[255] = 0;
        return 1;
    }

    size_t i;
    for (i = 0; i < 256 && x > 0; i++, x /= base)
        buf[255 - i - 1] = digits[x % base];

    buf[255] = 0;
    return i;
}

static size_t complete_buf(size_t clen, size_t objlen, char c) {
    for(;clen < objlen; ++clen)
        buf[255 - clen - 1] = c;
    return clen;
}

static inline
int64_t arg_int(uint8_t mod, va_list ps) {
    switch (mod) {
        case 1:
            return va_arg(ps, long int);
        case 2:
            return va_arg(ps, long long int);
        default:
            break;
    }
    return va_arg(ps, int);
}

int fpprintf(stringl_writer w, void* wi, const char* fmt, va_list ps) {
    int count = 0;
    while (*fmt) {
        if (fmt[0] != '%' || fmt[1] == '%') {
            size_t len = 1;
            if (fmt[0] == '%') fmt++;
            while(fmt[len] && fmt[len] != '%') len++;
            (*w)(wi, fmt, len);
            count += len;
            fmt += len;
            continue;
        }

        // TODO : more format
        //Modifiers
        uint8_t mod = 0;
        switch(* ++fmt) {
            case 'l':
                switch(* ++fmt) {
                    case 'l':
                        mod = 2;
                        ++fmt;
                        break;
                    default:
                        mod = 1;
                }
                break;
            default:
                break;
        }

        size_t len;
        switch(*fmt) {
            case 'c':
                buf[0] = (char)va_arg(ps, int);
                buf[1] = 0;
                (*w)(wi, buf, 1);
                count += 1;
            break;
            case 's':{
                const char *s = va_arg(ps, const char*);
                len = strlen(s);
                (*w)(wi, s, len);
                count += len;
            }break;
            case 'd':
                len = itoa(arg_int(mod, ps), decimal_digits, 10);
            goto print_buf;
            case 'x':
                len = ultoa(arg_int(mod, ps), hex_digits, 16);
            goto print_buf;
            case 'p':
                len = complete_buf(
                        ultoa(va_arg(ps, uint64_t), hex_digits, 16),
                        16, '0');
            print_buf:
                (*w)(wi, buf + 255 - len, len);
                count += len;
            break;
            default:
                return -1;
        }
        ++fmt;
    }

    return count;
}

#if defined(__is_libc)
static void
print(void *seq __attribute__((unused)), const char *s, size_t len) {
    while (len--) putchar(*s++);
}

int printf(const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);
    int cnt = fpprintf(&print, 0, fmt, params);
    va_end(params);
    return cnt;
}
#endif
