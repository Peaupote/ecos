#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/unistd.h>
#include <libc/ctype.h>
#include <util/misc.h>
#include <libc/errno.h>

static const char * const decimal_digits = "0123456789";
static const char * const hex_digits = "0123456789ABCDEF";

static size_t
itoa(char *buf, long long int x, const char *digits, size_t base) {
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
utoa(char *buf, uint64_t x, const char *digits, size_t base) {
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

static size_t complete_buf(char *buf, size_t clen, size_t objlen, char c) {
    for(;clen < objlen; ++clen)
        buf[255 - clen - 1] = c;
    return clen;
}

static int print_complete(char *buf, stringl_writer w, void* wi,
                          ssize_t clen, char c, size_t buf_lim) {
    if (clen <= 0) return 0;
    int rt = (int)clen;
    --buf_lim;
    buf[buf_lim] = '\0';
    size_t p = min_size_t(buf_lim, clen);
    for (size_t i = buf_lim - p; i < buf_lim; ++i)
        buf[i] = c;
    (*w)(wi, buf + buf_lim - p, p);
    clen -= p;
    while (clen > 0) {
        size_t p = min_size_t(buf_lim, clen);
        (*w)(wi, buf + buf_lim - p, p);
        clen -= p;
    }
    return rt;
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

static inline
uint64_t arg_uint(uint8_t mod, va_list ps) {
    switch (mod) {
        case 1:
            return va_arg(ps, long unsigned);
        case 2:
            return va_arg(ps, long long unsigned);
        default:
            break;
    }
    return va_arg(ps, unsigned);
}

int fpprintf(stringl_writer w, void* wi, const char* fmt, va_list ps) {
    char buf[256] = { 0 };
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

        //Modifiers
        char    compl_char = 0;
        bool    align_left = false;
        ssize_t  compl_len  = 0;
        uint8_t mod   = 0;
        while(true) {
            switch(* ++fmt) {
            case 'l':
                switch(* ++fmt) {
                case 'l':
                    mod = 2;
                    continue;
                default:
                    mod = 1;
                    --fmt;
                    continue;
                }
            case '-':
                align_left = true;
                continue;
            case '0':
                compl_char = '0';
                continue;

            case 0:
#ifndef __is_kernel
                errno = EINVAL;
#endif
                return -1;

            default:
                if (isdigit(*fmt)) {
                    if (!compl_char) compl_char = ' ';
                    compl_len = *fmt - '0';
                    while (isdigit(*++fmt))
                        compl_len = compl_len * 10 + *fmt - '0';
                    --fmt;
                    continue;
                }
                break;
            }
            break;
        }

        size_t len;
        switch(*fmt) {
        case 'c':
            buf[0] = (char)va_arg(ps, int);
            buf[1] = 0;
            len    = 1;
            if (compl_char && !align_left)
                count += print_complete(buf, w, wi,
                                        compl_len - 1, compl_char, 256);
            (*w)(wi, buf, 1);
            count += 1;
            goto print_comp;
        case 's':{
            const char *s = va_arg(ps, const char*);
            len = strlen(s);
            if (compl_char && !align_left)
                count += print_complete(buf, w, wi,
                                        compl_len - len, compl_char, 256);
            (*w)(wi, s, len);
            count += len;
        }goto print_comp;
        case 'd':
            len = itoa(buf, arg_int(mod, ps), decimal_digits, 10);
            goto print_buf;
        case 'u':
            len = utoa(buf, arg_uint(mod, ps), decimal_digits, 10);
            goto print_buf;
        case 'x':
            len = utoa(buf, arg_uint(mod, ps), hex_digits, 16);
            goto print_buf;
        case 'p':
            len = complete_buf(buf,
                               utoa(buf, va_arg(ps, uint64_t), hex_digits, 16),
                               16, '0');
        print_buf:
            if (compl_char && !align_left)
                count += print_complete(buf, w, wi,
                                        compl_len - len, compl_char,
                                        255 - len);
            (*w)(wi, buf + 255 - len, len);
            count += len;
        print_comp:
            if (compl_char && align_left)
                count += print_complete(buf, w, wi, compl_len - len,
                                        compl_char, 256);
            break;
        default:
#ifndef __is_kernel
            errno = EINVAL;
#endif
            return -1;
        }
        ++fmt;
    }

    return count;
}

#ifndef __is_kernel
static void
fprint(void *p_stream, const char *s, size_t len) {
    fwrite(s, 1, len, (FILE*)p_stream); // Write to stout
}

static void
fdprint(void *p_fd, const char *s, size_t len) {
    write(*(int*)p_fd, s, len); // Write to stout
}

int dprintf(int fd, const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);
    int cnt = fpprintf(&fdprint, &fd, fmt, params);
    va_end(params);
    return cnt;
}

int fprintf(FILE *stream, const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);
    int cnt = fpprintf(&fprint, stream, fmt, params);
    va_end(params);

    if (strchr(fmt, '\n')) fflush(stream);
    return cnt;
}

int vfprintf(FILE *stream, const char *fmt, va_list ap) {
    int cnt = fpprintf(&fprint, stream, fmt, ap);
    if (strchr(fmt, '\n')) fflush(stream);
    return cnt;
}

int printf(const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);
    int cnt = fpprintf(&fprint, stdout, fmt, params);
    va_end(params);

    if (strchr(fmt, '\n')) fflush(stdout);
    return cnt;
}

int vprinf(const char *fmt, va_list ap) {
    int cnt = fpprintf(&fprint, stdout, fmt, ap);
    if (strchr(fmt, '\n')) fflush(stdout);
    return cnt;
}

#endif

static void
nprint(void* none __attribute__((unused)),
    const char *s __attribute__((unused)),
    size_t    len __attribute__((unused))) {
    return;
}
static void
sprint(void *ptr, const char *s, size_t len) {
    char **str = (char**)ptr;
    while (len--) {
        *((*str)++) = *s++;
    }
}

int sprintf(char *str, const char *fmt, ...) {
    if (!str) {
        va_list params;
        va_start(params, fmt);
        int cnt = fpprintf(&nprint, NULL, fmt, params);
        va_end(params);
        return cnt;
    }

    va_list params;
    va_start(params, fmt);
    int cnt = fpprintf(&sprint, &str, fmt, params);
    *str = 0;
    va_end(params);

    return cnt;
}

int vsprintf(char *str, const char *fmt, va_list params) {
    if (!str) {
        return fpprintf(&nprint, NULL, fmt, params);
    }

    int cnt = fpprintf(&sprint, &str, fmt, params);
    *str = 0;
    return cnt;
}
