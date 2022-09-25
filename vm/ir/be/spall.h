// SPDX-FileCopyrightText: © 2022 Phillip Trudeau-Tavara <pmttavara@protonmail.com>
// SPDX-License-Identifier: 0BSD

/*

TODO: Core API:

  - Completely contextless; you pass in params to begin()/end(), get a packed begin/end struct
      - Simple, handmade, user has full control and full responsibility

TODO: Optional Helper APIs:

  - Buffered-writing API
      - Caller allocates and stores a buffer for multiple events
      - begin()/end() writes chunks to the buffer
      - Function invokes a callback when the buffer is full and needs flushing
          - Can a callback be avoided? The function indicates when the buffer must be flushed?

  - Compression API: would require a mutexed lockable context (yuck...)
      - Either using a ZIP library, a name cache + TIDPID cache, or both (but ZIP is likely more than enough!!!)
      - begin()/end() writes compressed chunks to a caller-determined destination
          - The destination can be the buffered-writing API or a custom user destination
      - Ultimately need to take a lock with some granularity... can that be the caller's responsibility?

  - fopen()/fwrite() API: requires a context (no mutex needed, since fwrite() takes a lock)
      - begin()/end() writes chunks to a FILE*
          - before writing them to disk, the chunks can optionally be sent through the compression API
              - is this opt-in or opt-out?
          - the write to disk can optionally use the buffered writing API

*/

#ifndef SPALL_H
#define SPALL_H

#include "../../lib.h"

#if defined(__GNUC__) || defined(__clang__)
static inline uint64_t vm_trace_time(void)
{
    uint32_t eax, edx;
    __asm__ volatile("rdtsc\n\t" : "=a" (eax), "=d" (edx));
    return (uint64_t)eax | (uint64_t)edx << 32;
}
#endif

#pragma pack(push, 1)

typedef struct SpallHeader {
    uint64_t magic_header; // = 0x0BADF00D
    uint64_t version; // = 0
    double timestamp_unit;
    uint64_t must_be_0;
} SpallHeader;

typedef struct SpallString {
    uint8_t length;
    char bytes[1];
} SpallString;

enum {
    SpallEventType_Invalid             = 0,
    SpallEventType_Custom_Data         = 1, // Basic readers can skip this.
    SpallEventType_StreamOver          = 2,

    SpallEventType_Begin               = 3,
    SpallEventType_End                 = 4,
    SpallEventType_Instant             = 5,

    SpallEventType_Overwrite_Timestamp = 6, // Retroactively change timestamp units - useful for incrementally improving RDTSC frequency.
    SpallEventType_Update_Checksum     = 7, // Verify rolling checksum. Basic readers/writers can ignore/omit this.
};

typedef struct SpallBeginEvent {
    uint8_t type; // = SpallEventType_Begin
    uint32_t pid;
    uint32_t tid;
    double when;
    SpallString name;
} SpallBeginEvent;

typedef struct SpallEndEvent {
    uint8_t type; // = SpallEventType_End
    uint32_t pid;
    uint32_t tid;
    double when;
} SpallEndEvent;

typedef struct SpallBeginEventMax {
    SpallBeginEvent event;
    char name_bytes[254];
} SpallBeginEventMax;

#pragma pack(pop)

typedef struct SpallProfile {
    double timestamp_unit;
    uint64_t is_json;
    bool (*write)(struct SpallProfile *self, void *data, size_t length);
    bool (*flush)(struct SpallProfile *self);
    union {
        FILE *file;
        void *userdata;
    };
} SpallProfile;

typedef struct SpallRecentString {
    char *pointer;
    uint8_t length;
} SpallRecentString;

// Important!: If you are writing Begin/End events, then do NOT write
//             events for the same PID + TID pair on different buffers!!!
typedef struct SpallBuffer {
    void *data;
    size_t length;

    // Internal data - don't assign this
    size_t head;
    SpallProfile *ctx;
    uint8_t recent_string_index;
    SpallRecentString recent_strings[256]; // ring buffer
} SpallBuffer;

#ifdef __cplusplus
extern "C" {
#endif

// Profile context
SpallProfile SpallInit    (const char *filename, double timestamp_unit);
SpallProfile SpallInitJson(const char *filename, double timestamp_unit);
void         SpallQuit    (SpallProfile *ctx);

bool SpallFlush(SpallProfile *ctx);

// Buffer API
extern SpallBuffer SpallSingleThreadedBuffer;

bool SpallBufferInit (SpallProfile *ctx, SpallBuffer *wb);
bool SpallBufferQuit (SpallProfile *ctx, SpallBuffer *wb);

bool SpallBufferFlush(SpallProfile *ctx, SpallBuffer *wb);

// Begin events
bool SpallTraceBegin         (SpallProfile *ctx, SpallBuffer *wb, double when, const char *name);
bool SpallTraceBeginTid      (SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, uint32_t tid);
bool SpallTraceBeginLen      (SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, signed long name_len);
bool SpallTraceBeginLenTid   (SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, signed long name_len, uint32_t tid);
bool SpallTraceBeginTidPid   (SpallProfile *ctx, SpallBuffer *wb, double when, const char *name,                       uint32_t tid, uint32_t pid);
bool SpallTraceBeginLenTidPid(SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, signed long name_len, uint32_t tid, uint32_t pid);

// End events
bool SpallTraceEnd      (SpallProfile *ctx, SpallBuffer *wb, double when);
bool SpallTraceEndTid   (SpallProfile *ctx, SpallBuffer *wb, double when, uint32_t tid);
bool SpallTraceEndTidPid(SpallProfile *ctx, SpallBuffer *wb, double when, uint32_t tid, uint32_t pid);

#ifdef __cplusplus
}
#endif

#endif // SPALL_H

#ifdef SPALL_IMPLEMENTATION
#ifndef SPALL_IMPLEMENTED
#define SPALL_IMPLEMENTED

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SPALL_BUFFER_PROFILING) && !defined(SPALL_BUFFER_PROFILING_GET_TIME)
#error "You must #define SPALL_BUFFER_PROFILING_GET_TIME() to profile buffer flushes."
#endif

#ifdef SPALL_BUFFER_PROFILING
#define SPALL_BUFFER_PROFILE_BEGIN() double spall_time_begin = (SPALL_BUFFER_PROFILING_GET_TIME())
// Don't call this with anything other than a string literal
#define SPALL_BUFFER_PROFILE_END(name) do { \
        double spall_time_end = (SPALL_BUFFER_PROFILING_GET_TIME()); \
        char temp_buffer_data[sizeof(SpallBeginEvent) + sizeof("" name "") - 2 + sizeof(SpallEndEvent)]; \
        SpallBuffer temp_buffer = {temp_buffer_data, sizeof(temp_buffer_data)}; \
        if (!SpallTraceBeginLenTidPid(ctx, &temp_buffer, spall_time_begin, name, sizeof(name) - 1, (uint32_t)(uintptr_t)wb->data, 4222222222)) return false; \
        if (!SpallTraceEndTidPid(ctx, &temp_buffer, spall_time_end, (uint32_t)(uintptr_t)wb->data, 4222222222)) return false; \
        Spall__FileWrite(ctx, temp_buffer_data, sizeof(temp_buffer_data)); \
    } while (0)
#else
#define SPALL_BUFFER_PROFILE_BEGIN()
#define SPALL_BUFFER_PROFILE_END(name)
#endif

extern char SpallSingleThreadedBufferData[];
char SpallSingleThreadedBufferData[1 << 16];
SpallBuffer SpallSingleThreadedBuffer = {SpallSingleThreadedBufferData, sizeof(SpallSingleThreadedBufferData)};

static bool Spall__FileWrite(SpallProfile *ctx, void *p, size_t n) {
    if (!ctx->file) return false;
    // if (feof(ctx->file)) return false;
    if (ferror(ctx->file)) return false;
    if (fwrite(p, n, 1, ctx->file) != 1) return false;
    return true;
}
static bool Spall__FileFlush(SpallProfile *ctx) {
    if (!ctx->file) return false;
    if (fflush(ctx->file)) return false;
    return true;
}
// TODO: Spall__FilePrintf
// TODO: Spall__FileClose

static void Spall__BufferPushString(SpallBuffer *wb, size_t n, signed long name_len) {
    SpallRecentString recent_string = {0};
    // precon: wb
    // precon: n > 0
    // precon: name_len > 0
    // precon: name_len < n
    // precon: name_len <= 255
    if (wb->head + n > wb->length) return; // will (try to) flush or do an unbuffered write, so don't push a string
    recent_string.pointer = (char *)wb->data + wb->head + n - name_len;
    recent_string.length = (uint8_t)name_len;
    wb->recent_strings[wb->recent_string_index++] = recent_string;
}

// returns -1 or a backreference number in [0, 255] -- telling you how many "strings ago" the string can be found (0 is the most recent string)
static int Spall__BufferFindString(SpallBuffer *wb, char *name, signed long name_len) {
    // precon: wb
    // precon: wb->data
    // precon: name
    // precon: name_len > 0
    // precon: name_len <= 255
    for (int i = 0; i < 256; i++) {
        unsigned int index = ((wb->recent_string_index - 1) - i + 256) % 256;
        SpallRecentString recent_string = wb->recent_strings[index];
        if (!recent_string.pointer) return -1; // early-out: NULLs mean the ring buffer isn't even full yet
        if (recent_string.length == name_len && memcmp(name, recent_string.pointer, recent_string.length) == 0) {
            return i;
        }
    }
    return -1;
}

static bool Spall__BufferFlush(SpallProfile *ctx, SpallBuffer *wb) {
    // precon: wb
    // precon: wb->data
    // precon: wb->head <= wb->length
    if (wb->ctx != ctx) return false; // Buffer must be bound to this context (or to NULL)
    if (wb->head && ctx) {
        if (!ctx->file) return false;
        SPALL_BUFFER_PROFILE_BEGIN();
        if (!ctx->write(ctx, wb->data, wb->head)) return false;
        SPALL_BUFFER_PROFILE_END("Buffer Flush");
    }
    wb->head = 0;
    wb->recent_string_index = 0;
    memset(wb->recent_strings, 0, sizeof(wb->recent_strings));
    return true;
}

static bool Spall__BufferWrite(SpallProfile *ctx, SpallBuffer *wb, void *p, size_t n) {
    // precon: !wb || wb->head < wb->length
    // precon: ctx->file
    if (!wb) return ctx->write(ctx, p, n);
    if (wb->head + n > wb->length && !Spall__BufferFlush(ctx, wb)) return false;
    if (n > wb->length) {
        SPALL_BUFFER_PROFILE_BEGIN();
        if (!ctx->write(ctx, p, n)) return false;
        SPALL_BUFFER_PROFILE_END("Unbuffered Write");
        return true;
    }
    memcpy((char *)wb->data + wb->head, p, n);
    wb->head += n;
    return true;
}

bool SpallBufferFlush(SpallProfile *ctx, SpallBuffer *wb) {
    if (!wb) return false;
    if (!wb->data) return false;
    if (!Spall__BufferFlush(ctx, wb)) return false;
    return true;
}

bool SpallBufferInit(SpallProfile *ctx, SpallBuffer *wb) {
    if (!SpallBufferFlush(NULL, wb)) return false;
    wb->ctx = ctx;
    return true;
}
bool SpallBufferQuit(SpallProfile *ctx, SpallBuffer *wb) {
    if (!SpallBufferFlush(ctx, wb)) return false;
    wb->ctx = NULL;
    return true;
}

bool SpallBufferAbort(SpallBuffer *wb) {
    if (!wb) return false;
    wb->ctx = NULL;
    if (!Spall__BufferFlush(NULL, wb)) return false;
    return true;
}

static SpallProfile Spall__Init(const char *filename, double timestamp_unit, bool is_json) {
    SpallProfile ctx;
    memset(&ctx, 0, sizeof(ctx));
    if (timestamp_unit < 0) return ctx;
    if (!filename) return ctx;
    ctx.file = fopen(filename, "wb"); // TODO: handle utf8 and long paths on windows
    ctx.write = Spall__FileWrite;
    ctx.flush = Spall__FileFlush;
    if (!ctx.file) { SpallQuit(&ctx); return ctx; }
    ctx.timestamp_unit = timestamp_unit;
    ctx.is_json = is_json;
    if (ctx.is_json) {
        if (fprintf(ctx.file, "{\"traceEvents\":[\n") <= 0) { SpallQuit(&ctx); return ctx; }
    } else {
        SpallHeader header;
        header.magic_header = 0x0BADF00D;
        header.version = 0;
        header.timestamp_unit = timestamp_unit;
        header.must_be_0 = 0;
        if (!ctx.write(&ctx, &header, sizeof(header))) { SpallQuit(&ctx); return ctx; }
    }
    return ctx;
}

SpallProfile SpallInit    (const char *filename, double timestamp_unit) { return Spall__Init(filename, timestamp_unit, false); }
SpallProfile SpallInitJson(const char *filename, double timestamp_unit) { return Spall__Init(filename, timestamp_unit,  true); }

void SpallQuit(SpallProfile *ctx) {
    if (!ctx) return;
    if (ctx->file) {
        if (ctx->is_json) {
            fseek(ctx->file, -2, SEEK_CUR); // seek back to overwrite trailing comma
            fprintf(ctx->file, "\n]}\n");
        }
        fflush(ctx->file);
        fclose(ctx->file);
    }
    memset(ctx, 0, sizeof(*ctx));
}

bool SpallFlush(SpallProfile *ctx) {
    if (!ctx) return false;
    if (!ctx->file) return false;
    if (!ctx->flush(ctx)) return false;
    return true;
}

bool SpallTraceBeginLenTidPid(SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, signed long name_len, uint32_t tid, uint32_t pid) {
    SpallBeginEventMax ev;
    if (!ctx) return false;
    if (!name) return false;
    if (!ctx->file) return false;
    // if (ctx->times_are_u64) return false;
    if (name_len <= 0) return false;
    if (name_len > 255) name_len = 255; // will be interpreted as truncated in the app (?)
    ev.event.type = SpallEventType_Begin;
    ev.event.pid = pid;
    ev.event.tid = tid;
    ev.event.when = when;
    ev.event.name.length = (uint8_t)name_len;
    memcpy(ev.event.name.bytes, name, (uint8_t)name_len);
    if (ctx->is_json) {
        if (fprintf(ctx->file,
                    "{\"name\":\"%.*s\",\"ph\":\"B\",\"pid\":%u,\"tid\":%u,\"ts\":%f},\n",
                    (int)ev.event.name.length, ev.event.name.bytes,
                    ev.event.pid,
                    ev.event.tid,
                    ev.event.when * ctx->timestamp_unit)
            <= 0) return false;
    } else {
        if (!Spall__BufferWrite(ctx, wb, &ev, sizeof(SpallBeginEvent) + (uint8_t)name_len - 1)) return false;
    }
    return true;
}
bool SpallTraceBeginTidPid(SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, uint32_t tid, uint32_t pid) {
    unsigned long name_len;
    if (!name) return false;
    name_len = strlen(name);
    if (!name_len) return false;
    return SpallTraceBeginLenTidPid(ctx, wb, when, name, (signed long)name_len, tid, pid);
}
bool SpallTraceBeginLenTid(SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, signed long name_len, uint32_t tid) { return SpallTraceBeginLenTidPid(ctx, wb, when, name, name_len, tid, 0); }
bool SpallTraceBeginLen   (SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, signed long name_len)               { return SpallTraceBeginLenTidPid(ctx, wb, when, name, name_len,   0, 0); }
bool SpallTraceBeginTid   (SpallProfile *ctx, SpallBuffer *wb, double when, const char *name, uint32_t tid)                       { return SpallTraceBeginTidPid   (ctx, wb, when, name,           tid, 0); }
bool SpallTraceBegin      (SpallProfile *ctx, SpallBuffer *wb, double when, const char *name)                                     { return SpallTraceBeginTidPid   (ctx, wb, when, name,             0, 0); }

bool SpallTraceEndTidPid(SpallProfile *ctx, SpallBuffer *wb, double when, uint32_t tid, uint32_t pid) {
    SpallEndEvent ev;
    if (!ctx) return false;
    if (!ctx->file) return false;
    // if (ctx->times_are_u64) return false;
    ev.type = SpallEventType_End;
    ev.pid = pid;
    ev.tid = tid;
    ev.when = when;
    if (ctx->is_json) {
        if (fprintf(ctx->file,
                    "{\"ph\":\"E\",\"pid\":%u,\"tid\":%u,\"ts\":%f},\n",
                    ev.pid,
                    ev.tid,
                    ev.when * ctx->timestamp_unit)
            <= 0) return false;
    } else {
        if (!Spall__BufferWrite(ctx, wb, &ev, sizeof(ev))) return false;
    }
    return true;
}

bool SpallTraceEndTid(SpallProfile *ctx, SpallBuffer *wb, double when, uint32_t tid) { return SpallTraceEndTidPid(ctx, wb, when, tid, 0); }
bool SpallTraceEnd   (SpallProfile *ctx, SpallBuffer *wb, double when)               { return SpallTraceEndTidPid(ctx, wb, when,   0, 0); }

#ifdef __cplusplus
}
#endif

#endif // SPALL_IMPLEMENTED
#endif // SPALL_IMPLEMENTATION

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
