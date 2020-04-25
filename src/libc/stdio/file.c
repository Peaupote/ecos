#include <libc/stdio.h>

#include <libc/unistd.h>
#include <libc/fcntl.h>
#include <libc/sys/stat.h>
#include <libc/errno.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <libc/assert.h>

#define FILE_BUF_SIZE 2048

static int mode_to_oflag(const char *mode) {
    int oflags = 0;

    switch (*mode++) {
    case 'r': oflags |= READ; break;
    case 'w': oflags |= O_CREAT|O_TRUNC|WRITE; break;
    case 'a': oflags |= O_CREAT|O_APPEND|WRITE; break;
    default: return 0;
    }

    if (*mode == '+') oflags |= RDWR;
    return oflags;
}

static FILE *open_file(int fd, int oflags) {
    int save_err;

    FILE *stream = malloc(sizeof(FILE));
    memset(stream, 0, sizeof(FILE));

    if (stream) {
        stream->fd = fd;
        stream->flags = oflags;

        if (oflags&READ) {
            stream->read_buf = malloc(FILE_BUF_SIZE);
            if (!stream->read_buf) goto fail;
            stream->read_ptr = stream->read_buf;
            stream->read_end = stream->read_buf;
        }

        if (oflags&WRITE) {
            stream->write_buf = malloc(FILE_BUF_SIZE);
            if (!stream->write_buf) goto fail;
            stream->write_ptr = stream->write_buf;
            stream->write_end = stream->write_buf + FILE_BUF_SIZE;
        }
    }

    return stream;

fail:
    save_err = errno;
    close(stream->fd);
    free(stream);
    errno = save_err;
    return 0;
}

FILE *fopen(const char *fname, const char *mode) {
    if (!fname) {
        errno = EINVAL;
        return 0;
    }

    int oflags = mode_to_oflag(mode);
    int fd = open(fname, oflags, 0640);
    if (fd < 0) return 0;
    return open_file(fd, oflags);
}

FILE *fdopen(int fd, const char *mode) {
    if (fd < 0 || !mode) {
        errno = EINVAL;
        return 0;
    }

    int oflags = mode_to_oflag(mode);
    return open_file(fd, oflags);
}

int fclose(FILE *stream) {
    if (!stream) return 0;
    fflush(stream);
    close(stream->fd);
    free(stream->read_buf);
    free(stream->write_buf);
    free(stream);
    return 0;
}

int fseek(FILE *stream, long offset, int whence) {
    if (!stream) {
        errno = EINVAL;
        return -1;
    }

    return lseek(stream->fd, offset, whence);
}

int fflush(FILE *stream) {
    if (!stream || !(stream->flags&WRITE)) {
        errno = EINVAL;
        return -1;
    }

    int rc = write(stream->fd, stream->write_buf,
                   stream->write_ptr - stream->write_buf);
    stream->write_ptr = stream->write_buf;
    return rc < 0 ? -1 : 0;
}

size_t fread(void *ptr, size_t size, size_t nmem, FILE *s) {
    if (!size || !s || !(s->flags&READ)) {
        errno = EINVAL;
        return 0;
    }

    size_t byte_size = size * nmem;
    size_t cnt = 0;

    while (byte_size > 0) {
        if (s->read_ptr == s->read_end) {
            int rc = read(s->fd, s->read_buf, FILE_BUF_SIZE);
            if (rc < 0) return 0;
            else if (rc == 0) break;
            s->read_ptr = s->read_buf;
            s->read_end = s->read_buf + rc;
        }

        size_t delta = s->read_end - s->read_ptr;
        size_t len = byte_size > delta ? delta : byte_size;
        memcpy(ptr, s->read_ptr, len);
        s->read_ptr += len;
        byte_size -= len;
        cnt += len;
        ptr += len;
    }

    return cnt / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmem, FILE *s) {
    if (!size || !s || !(s->flags&WRITE)) {
        errno = EINVAL;
        return 0;
    }

    size_t byte_size = size * nmem;
    size_t cnt = 0;

    while (byte_size > 0) {
        if (s->write_ptr == s->write_end) { // buffer is full
            int rc = write(s->fd, s->write_buf, s->write_ptr - s->write_buf);
            if (rc <= 0) return 0;
            s->write_ptr = s->write_buf;
        }

        size_t delta = s->write_end - s->write_ptr;
        size_t len = byte_size > delta ? delta : byte_size;

        memcpy(s->write_ptr, ptr, len);
        s->write_ptr += len;
        byte_size -= len;
        ptr += len;
        cnt += len;
    }

    return cnt / size;
}
