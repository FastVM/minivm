// SPDX-FileCopyrightText: © 2022 Phillip Trudeau-Tavara <pmttavara@protonmail.com>
// SPDX-License-Identifier: 0BSD

/*

TODO: Optional Helper APIs:

  - Compression API: would require a mutexed lockable context (yuck...)
      - Either using a ZIP library, a name cache + TIDPID cache, or both (but ZIP is likely more than enough!!!)
      - begin()/end() writes compressed chunks to a caller-determined destination
          - The destination can be the buffered-writing API or a custom user destination
      - Ultimately need to take a lock with some granularity... can that be the caller's responsibility?

  - Counter Event: should allow tracking arbitrary named values with a single event, for memory and frame profiling

  - Ring-buffer API
        spall_ring_init
        spall_ring_emit_begin
        spall_ring_emit_end
        spall_ring_flush
*/

#ifndef SPALL_H
#define SPALL_H

#if !defined(_MSC_VER) || defined(__clang__)
#define SPALL_NOINSTRUMENT __attribute__((no_instrument_function))
#define SPALL_FORCEINLINE __attribute__((always_inline))
#else
#define _CRT_SECURE_NO_WARNINGS
#define SPALL_NOINSTRUMENT // Can't noinstrument on MSVC!
#define SPALL_FORCEINLINE __forceinline
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define SPALL_FN static inline SPALL_NOINSTRUMENT

#define SPALL_MIN(a, b) (((a) < (b)) ? (a) : (b))

#pragma pack(push, 1)

typedef struct SpallHeader {
    uint64_t magic_header; // = 0x0BADF00D
    uint64_t version; // = 1
    double   timestamp_unit;
    uint64_t must_be_0;
} SpallHeader;

enum {
    SpallEventType_Invalid             = 0,
    SpallEventType_Custom_Data         = 1, // Basic readers can skip this.
    SpallEventType_StreamOver          = 2,

    SpallEventType_Begin               = 3,
    SpallEventType_End                 = 4,
    SpallEventType_Instant             = 5,

    SpallEventType_Overwrite_Timestamp = 6, // Retroactively change timestamp units - useful for incrementally improving RDTSC frequency.
    SpallEventType_Pad_Skip            = 7,
};

typedef struct SpallBeginEvent {
    uint8_t type; // = SpallEventType_Begin
    uint8_t category;

    uint32_t pid;
    uint32_t tid;
    double   when;

    uint8_t name_length;
    uint8_t args_length;
} SpallBeginEvent;

typedef struct SpallBeginEventMax {
    SpallBeginEvent event;
    char name_bytes[255];
    char args_bytes[255];
} SpallBeginEventMax;

typedef struct SpallEndEvent {
    uint8_t  type; // = SpallEventType_End
    uint32_t pid;
    uint32_t tid;
    double   when;
} SpallEndEvent;

typedef struct SpallPadSkipEvent {
    uint8_t  type; // = SpallEventType_Pad_Skip
    uint32_t size;
} SpallPadSkipEvent;

#pragma pack(pop)

typedef struct SpallProfile SpallProfile;

// Important!: If you define your own callbacks, mark them SPALL_NOINSTRUMENT!
typedef bool (*SpallWriteCallback)(SpallProfile *self, const void *data, size_t length);
typedef bool (*SpallFlushCallback)(SpallProfile *self);
typedef void (*SpallCloseCallback)(SpallProfile *self);

struct SpallProfile {
    double timestamp_unit;
    bool is_json;
    SpallWriteCallback write;
    SpallFlushCallback flush;
    SpallCloseCallback close;
    void *data;
};

// Important!: If you are writing Begin/End events, then do NOT write
//             events for the same PID + TID pair on different buffers!!!
typedef struct SpallBuffer {
    void *data;
    size_t length;

    // Internal data - don't assign this
    size_t head;
    SpallProfile *ctx;
} SpallBuffer;

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SPALL_BUFFER_PROFILING) && !defined(SPALL_BUFFER_PROFILING_GET_TIME)
#error "You must #define SPALL_BUFFER_PROFILING_GET_TIME() to profile buffer flushes."
#endif

SPALL_FN SPALL_FORCEINLINE void spall__buffer_profile(SpallProfile *ctx, SpallBuffer *wb, double spall_time_begin, double spall_time_end, const char *name, int name_len);
#ifdef SPALL_BUFFER_PROFILING
#define SPALL_BUFFER_PROFILE_BEGIN() double spall_time_begin = (SPALL_BUFFER_PROFILING_GET_TIME())
// Don't call this with anything other than a string literal
#define SPALL_BUFFER_PROFILE_END(name) spall__buffer_profile(ctx, wb, spall_time_begin, (SPALL_BUFFER_PROFILING_GET_TIME()), "" name "", sizeof("" name "") - 1)
#else
#define SPALL_BUFFER_PROFILE_BEGIN()
#define SPALL_BUFFER_PROFILE_END(name)
#endif

SPALL_FN SPALL_FORCEINLINE bool spall__file_write(SpallProfile *ctx, const void *p, size_t n) {
    if (!ctx->data) return false;
#ifdef SPALL_DEBUG
    if (feof((FILE *)ctx->data)) return false;
    if (ferror((FILE *)ctx->data)) return false;
#endif

    if (fwrite(p, n, 1, (FILE *)ctx->data) != 1) return false;
    return true;
}
SPALL_FN bool spall__file_flush(SpallProfile *ctx) {
    if (!ctx->data) return false;
    if (fflush((FILE *)ctx->data)) return false;
    return true;
}
SPALL_FN void spall__file_close(SpallProfile *ctx) {
    if (!ctx->data) return;

    if (ctx->is_json) {
#ifdef SPALL_DEBUG
        if (!feof((FILE *)ctx->data) && !ferror((FILE *)ctx->data))
#endif
        {
            fseek((FILE *)ctx->data, -2, SEEK_CUR); // seek back to overwrite trailing comma
            fwrite("\n]}\n", sizeof("\n]}\n") - 1, 1, (FILE *)ctx->data);
        }
    }
    fflush((FILE *)ctx->data);
    fclose((FILE *)ctx->data);
    ctx->data = NULL;
}

SPALL_FN SPALL_FORCEINLINE bool spall__buffer_flush(SpallProfile *ctx, SpallBuffer *wb) {
    // precon: wb
    // precon: wb->data
    // precon: wb->head <= wb->length
    // precon: !ctx || ctx->write
#ifdef SPALL_DEBUG
    if (wb->ctx != ctx) return false; // Buffer must be bound to this context (or to NULL)
#endif

    if (wb->head && ctx) {
        SPALL_BUFFER_PROFILE_BEGIN();
        if (!ctx->write) return false;
        if (ctx->write == spall__file_write) {
            if (!spall__file_write(ctx, wb->data, wb->head)) return false;
        } else {
            if (!ctx->write(ctx, wb->data, wb->head)) return false;
        }
        SPALL_BUFFER_PROFILE_END("Buffer Flush");
    }
    wb->head = 0;
    return true;
}

SPALL_FN SPALL_FORCEINLINE bool spall__buffer_write(SpallProfile *ctx, SpallBuffer *wb, void *p, size_t n) {
    // precon: !wb || wb->head < wb->length
    // precon: !ctx || ctx->write
    if (!wb) return ctx->write && ctx->write(ctx, p, n);
#ifdef SPALL_DEBUG
    if (wb->ctx != ctx) return false; // Buffer must be bound to this context (or to NULL)
#endif
    if (wb->head + n > wb->length && !spall__buffer_flush(ctx, wb)) return false;
    if (n > wb->length) {
        SPALL_BUFFER_PROFILE_BEGIN();
        if (!ctx->write || !ctx->write(ctx, p, n)) return false;
        SPALL_BUFFER_PROFILE_END("Unbuffered Write");
        return true;
    }
    memcpy((char *)wb->data + wb->head, p, n);
    wb->head += n;
    return true;
}

SPALL_FN bool spall_buffer_flush(SpallProfile *ctx, SpallBuffer *wb) {
#ifdef SPALL_DEBUG
    if (!wb) return false;
    if (!wb->data) return false;
#endif

    if (!spall__buffer_flush(ctx, wb)) return false;
    return true;
}

SPALL_FN bool spall_buffer_init(SpallProfile *ctx, SpallBuffer *wb) {
    if (!spall_buffer_flush(NULL, wb)) return false;
    wb->ctx = ctx;
    return true;
}
SPALL_FN bool spall_buffer_quit(SpallProfile *ctx, SpallBuffer *wb) {
    if (!spall_buffer_flush(ctx, wb)) return false;
    wb->ctx = NULL;
    return true;
}

SPALL_FN bool spall_buffer_abort(SpallBuffer *wb) {
    if (!wb) return false;
    wb->ctx = NULL;
    if (!spall__buffer_flush(NULL, wb)) return false;
    return true;
}

SPALL_FN size_t spall_build_header(void *buffer, size_t rem_size, double timestamp_unit) {
    size_t header_size = sizeof(SpallHeader);
    if (header_size > rem_size) {
        return 0;
    }

    SpallHeader *header = (SpallHeader *)buffer;
    header->magic_header = 0x0BADF00D;
    header->version = 1;
    header->timestamp_unit = timestamp_unit;
    header->must_be_0 = 0;
    return header_size;
}
SPALL_FN SPALL_FORCEINLINE size_t spall_build_begin(void *buffer, size_t rem_size, const char *name, signed long name_len, const char *args, signed long args_len, double when, uint32_t tid, uint32_t pid) {
    SpallBeginEventMax *ev = (SpallBeginEventMax *)buffer;
    uint8_t trunc_name_len = (uint8_t)SPALL_MIN(name_len, 255); // will be interpreted as truncated in the app (?)
    uint8_t trunc_args_len = (uint8_t)SPALL_MIN(args_len, 255); // will be interpreted as truncated in the app (?)

    size_t ev_size = sizeof(SpallBeginEvent) + trunc_name_len + trunc_args_len;
    if (ev_size > rem_size) {
        return 0;
    }

    ev->event.type = SpallEventType_Begin;
    ev->event.category = 0;
    ev->event.pid = pid;
    ev->event.tid = tid;
    ev->event.when = when;
    ev->event.name_length = trunc_name_len;
    ev->event.args_length = trunc_args_len;
    memcpy(ev->name_bytes,            name, trunc_name_len);
    memcpy(ev->name_bytes + name_len, args, trunc_args_len);

    return ev_size;
}
SPALL_FN SPALL_FORCEINLINE size_t spall_build_end(void *buffer, size_t rem_size, double when, uint32_t tid, uint32_t pid) {
    size_t ev_size = sizeof(SpallEndEvent);
    if (ev_size > rem_size) {
        return 0;
    }

    SpallEndEvent *ev = (SpallEndEvent *)buffer;
    ev->type = SpallEventType_End;
    ev->pid = pid;
    ev->tid = tid;
    ev->when = when;

    return ev_size;
}

SPALL_FN void spall_quit(SpallProfile *ctx) {
    if (!ctx) return;
    if (ctx->close) ctx->close(ctx);

    memset(ctx, 0, sizeof(*ctx));
}

SPALL_FN SpallProfile spall_init_callbacks(double timestamp_unit,
                                           SpallWriteCallback write,
                                           SpallFlushCallback flush,
                                           SpallCloseCallback close,
                                           void *userdata,
                                           bool is_json) {
    SpallProfile ctx;
    memset(&ctx, 0, sizeof(ctx));
    if (timestamp_unit < 0) return ctx;
    ctx.timestamp_unit = timestamp_unit;
    ctx.is_json = is_json;
    ctx.data = userdata;
    ctx.write = write;
    ctx.flush = flush;
    ctx.close = close;

    if (ctx.is_json) {
        if (!ctx.write(&ctx, "{\"traceEvents\":[\n", sizeof("{\"traceEvents\":[\n") - 1)) { spall_quit(&ctx); return ctx; }
    } else {
        SpallHeader header;
        size_t len = spall_build_header(&header, sizeof(header), timestamp_unit);
        if (!ctx.write(&ctx, &header, len)) { spall_quit(&ctx); return ctx; }
    }

    return ctx;
}

SPALL_FN SpallProfile spall_init_file_ex(const char *filename, double timestamp_unit, bool is_json) {
    SpallProfile ctx;
    memset(&ctx, 0, sizeof(ctx));
    if (!filename) return ctx;
    ctx.data = fopen(filename, "wb"); // TODO: handle utf8 and long paths on windows
    if (ctx.data) { // basically freopen() but we don't want to force users to lug along another macro define
        fclose((FILE *)ctx.data);
        ctx.data = fopen(filename, "ab");
    }
    if (!ctx.data) { spall_quit(&ctx); return ctx; }
    ctx = spall_init_callbacks(timestamp_unit, spall__file_write, spall__file_flush, spall__file_close, ctx.data, is_json);
    return ctx;
}

SPALL_FN SpallProfile spall_init_file     (const char* filename, double timestamp_unit) { return spall_init_file_ex(filename, timestamp_unit, false); }
SPALL_FN SpallProfile spall_init_file_json(const char* filename, double timestamp_unit) { return spall_init_file_ex(filename, timestamp_unit, true); }

SPALL_FN bool spall_flush(SpallProfile *ctx) {
#ifdef SPALL_DEBUG
    if (!ctx) return false;
#endif

    if (!ctx->flush || !ctx->flush(ctx)) return false;
    return true;
}

SPALL_FN SPALL_FORCEINLINE bool spall_buffer_begin_args(SpallProfile *ctx, SpallBuffer *wb, const char *name, signed long name_len, const char *args, signed long args_len, double when, uint32_t tid, uint32_t pid) {
#ifdef SPALL_DEBUG
    if (!ctx) return false;
    if (!name) return false;
    if (name_len <= 0) return false;
    if (!wb) return false;
#endif

    if (ctx->is_json) {
        char buf[1024];
        int buf_len = snprintf(buf, sizeof(buf),
                               "{\"ph\":\"B\",\"ts\":%f,\"pid\":%u,\"tid\":%u,\"name\":\"%.*s\",\"args\":\"%.*s\"},\n",
                               when * ctx->timestamp_unit, pid, tid, (int)(uint8_t)name_len, name, (int)(uint8_t)args_len, args);
        if (buf_len <= 0) return false;
        if (buf_len >= sizeof(buf)) return false;
        if (!spall__buffer_write(ctx, wb, buf, buf_len)) return false;
    } else {
        if ((wb->head + sizeof(SpallBeginEventMax)) > wb->length) {
            if (!spall__buffer_flush(ctx, wb)) {
                return false;
            }
        }

        wb->head += spall_build_begin((char *)wb->data + wb->head, wb->length - wb->head, name, name_len, args, args_len, when, tid, pid);
    }

    return true;
}

SPALL_FN SPALL_FORCEINLINE bool spall_buffer_begin_ex(SpallProfile *ctx, SpallBuffer *wb, const char *name, signed long name_len, double when, uint32_t tid, uint32_t pid) {
    return spall_buffer_begin_args(ctx, wb, name, name_len, "", 0, when, tid, pid);
}

SPALL_FN bool spall_buffer_begin(SpallProfile *ctx, SpallBuffer *wb, const char *name, signed long name_len, double when) {
    return spall_buffer_begin_args(ctx, wb, name, name_len, "", 0, when, 0, 0);
}

SPALL_FN SPALL_FORCEINLINE bool spall_buffer_end_ex(SpallProfile *ctx, SpallBuffer *wb, double when, uint32_t tid, uint32_t pid) {
#ifdef SPALL_DEBUG
    if (!ctx) return false;
    if (!wb) return false;
#endif

    if (ctx->is_json) {
        char buf[512];
        int buf_len = snprintf(buf, sizeof(buf),
                               "{\"ph\":\"E\",\"ts\":%f,\"pid\":%u,\"tid\":%u},\n",
                               when * ctx->timestamp_unit, pid, tid);
        if (buf_len <= 0) return false;
        if (buf_len >= sizeof(buf)) return false;
        if (!spall__buffer_write(ctx, wb, buf, buf_len)) return false;
    } else {
        if ((wb->head + sizeof(SpallEndEvent)) > wb->length) {
            if (!spall__buffer_flush(ctx, wb)) {
                return false;
            }
        }

        wb->head += spall_build_end((char *)wb->data + wb->head, wb->length - wb->head, when, tid, pid);
    }

    return true;
}

SPALL_FN bool spall_buffer_end(SpallProfile *ctx, SpallBuffer *wb, double when) { return spall_buffer_end_ex(ctx, wb, when, 0, 0); }

SPALL_FN SPALL_FORCEINLINE void spall__buffer_profile(SpallProfile *ctx, SpallBuffer *wb, double spall_time_begin, double spall_time_end, const char *name, int name_len) {
    // precon: ctx
    // precon: ctx->write
    char temp_buffer_data[2048];
    SpallBuffer temp_buffer = { temp_buffer_data, sizeof(temp_buffer_data) };
    if (!spall_buffer_begin_ex(ctx, &temp_buffer, name, name_len, spall_time_begin, (uint32_t)(uintptr_t)wb->data, 4222222222)) return;
    if (!spall_buffer_end_ex(ctx, &temp_buffer, spall_time_end, (uint32_t)(uintptr_t)wb->data, 4222222222)) return;
    if (ctx->write) ctx->write(ctx, temp_buffer_data, temp_buffer.head);
}

#ifdef __cplusplus
}
#endif

#endif // SPALL_H

/*
Zero-Clause BSD (0BSD)

Copyright (c) 2022, Phillip Trudeau-Tavara
All rights reserved.

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
