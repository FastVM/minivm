// String Buffer
#ifndef NL_BUFFER_H
#define NL_BUFFER_H

struct nl_buffer_t;
typedef struct nl_buffer_t nl_buffer_t;

struct nl_buffer_t {
    void *arena;
    char *buf;
    int len;
    int alloc;
};

nl_buffer_t *nl_buffer_new(void *arena);
void nl_buffer_alloc(nl_buffer_t *buf);

void nl_buffer_format(nl_buffer_t *buf, const char *fmt, ...) __attribute__ (( format( printf, 2, 3 ) )) ;
char *nl_buffer_get(nl_buffer_t *buf);
char *nl_buffer_copy(nl_buffer_t *buf);

#ifdef NL_BUFFER_IMPL

nl_buffer_t *nl_buffer_new(void *alloc) {
    nl_buffer_t *buf = tb_arena_alloc(alloc, sizeof(nl_buffer_t));
    buf->arena = alloc;
    buf->alloc = 16;
    buf->buf = tb_arena_unaligned_alloc(buf->arena, sizeof(char) * buf->alloc);
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
            buf->buf = tb_arena_realloc(buf->arena, buf->buf, sizeof(char) * buf->alloc);
            continue;
        }
        buf->len += written;
        break;
    }
}

char *nl_buffer_get(nl_buffer_t *buf) {
    return buf->buf;
}

char *nl_buffer_copy(nl_buffer_t *buf) {
    int len = strlen(buf->buf);
    char *ret = NL_MALLOC(len + 1);
    memcpy(ret, buf->buf, len + 1);
    return ret;
}

#endif

#endif

