// String Buffer
#ifndef NL_BUFFER_H
#define NL_BUFFER_H

struct nl_buffer_t;
typedef struct nl_buffer_t nl_buffer_t;

struct nl_buffer_t {
    char *buf;
    int len;
    int alloc;
};

nl_buffer_t *nl_buffer_new(void);
void nl_buffer_alloc(nl_buffer_t *buf);

void nl_buffer_format(nl_buffer_t *buf, const char *fmt, ...);
char *nl_buffer_get(nl_buffer_t *buf);

#ifdef NL_BUFFER_IMPL

nl_buffer_t *nl_buffer_new(void) {
    nl_buffer_t *buf = NL_MALLOC(sizeof(nl_buffer_t));
    buf->alloc = 16;
    buf->buf = NL_MALLOC(sizeof(char) * buf->alloc);
    buf->buf[0] = '\0';
    buf->len = 0;
    return buf;
}

void nl_buffer_format(nl_buffer_t *buf, const char *restrict fmt, ...) {
    while (true) {
        int avail = buf->alloc - buf->len;
        va_list ap;
        va_start(ap, fmt);
        int written = vsnprintf(&buf->buf[buf->len], avail, fmt, ap);
        va_end(ap);
        if (avail <= written) {
            buf->alloc = buf->alloc * 2 + 16;
            buf->buf = NL_REALLOC(buf->buf, sizeof(char) * buf->alloc);
            continue;
        }
        buf->len += written;
        break;
    }
}

char *nl_buffer_get(nl_buffer_t *buf) {
    return buf->buf;
}

#endif

#endif

