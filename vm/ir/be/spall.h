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

#if defined(__x86_64__)
#if defined(__GNUC__) || defined(__clang__)
static inline double vm_trace_time(void) {
    uint32_t eax, edx;
    __asm__ volatile("rdtsc\n\t"
                     : "=a"(eax), "=d"(edx));
    return (double)((uint64_t)eax | (uint64_t)edx << 32);
}
#else
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#define vm_trace_time() ((double)__rdtsc())
#endif
#else
#define vm_trace_time() ((double) 0)
#endif

typedef struct __attribute__((__packed__)) vm_trace_header_t {
    uint64_t magic_header;  // = 0x0BADF00D
    uint64_t version;       // = 0
    double timestamp_unit;
    uint64_t must_be_0;
} vm_trace_header_t;

typedef struct vm_trace_String {
    uint8_t length;
    char bytes[1];
} vm_trace_String;

#define VM_TRACE_EVENT_TYPE_INVALID 0
#define VM_TRACE_EVENT_TYPE_CUSTOM_DATA 1 /* Basic readers can skip this. */
#define VM_TRACE_EVENT_TYPE_STREAMOVER 2
#define VM_TRACE_EVENT_TYPE_BEGIN 3
#define VM_TRACE_EVENT_TYPE_END 4
#define VM_TRACE_EVENT_TYPE_INSTANT 5
#define VM_TRACE_EVENT_TYPE_OVERWRITE_TIMESTAMP 6 /* Retroactively change timestamp units - useful for incrementally improving RDTSC frequency. */
#define VM_TRACE_EVENT_TYPE_UPDATE_CHECKSUM 7     /* Verify rolling checksum. Basic readers/writers can ignore/omit this. */

typedef struct __attribute__((__packed__)) vm_trace_begin_event_t {
    uint8_t type;  // = vm_trace_EventType_Begin
    uint32_t pid;
    uint32_t tid;
    double when;
    vm_trace_String name;
} vm_trace_begin_event_t;

typedef struct __attribute__((__packed__)) vm_trace_EndEvent {
    uint8_t type;  // = vm_trace_EventType_End
    uint32_t pid;
    uint32_t tid;
    double when;
} vm_trace_EndEvent;

typedef struct __attribute__((__packed__)) vm_trace_begin_event_max_t {
    vm_trace_begin_event_t event;
    char name_bytes[254];
} vm_trace_begin_event_max_t;

typedef struct vm_trace_profile_t {
    double timestamp_unit;
    uint64_t is_json;
    bool (*write)(struct vm_trace_profile_t *self, void *data, size_t length);
    bool (*flush)(struct vm_trace_profile_t *self);
    union {
        FILE *file;
        void *userdata;
    };
} vm_trace_profile_t;

typedef struct __attribute__((__packed__)) vm_trace_RecentString {
    char *pointer;
    uint8_t length;
} vm_trace_RecentString;

// Important!: If you are writing Begin/End events, then do NOT write
//             events for the same PID + TID pair on different buffers!!!
typedef struct vm_trace_buffer_t {
    void *data;
    size_t length;

    // Internal data - don't assign this
    size_t head;
    vm_trace_profile_t *ctx;
    uint8_t recent_string_index;
    vm_trace_RecentString recent_strings[256];  // ring buffer
} vm_trace_buffer_t;

#ifdef __cplusplus
extern "C" {
#endif

// Profile context
vm_trace_profile_t vm_trace_init(const char *filename, double timestamp_unit);
vm_trace_profile_t vm_trace_init_json(const char *filename, double timestamp_unit);
void vm_trace_quit(vm_trace_profile_t *ctx);

bool vm_trace_Flush(vm_trace_profile_t *ctx);

// Buffer API
bool vm_trace_buffer_init(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb);
bool vm_trace_buffer_quit(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb);

bool vm_trace_buffer_flush(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb);

// Begin events
bool vm_trace_begin(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name);
bool vm_trace_begin_tid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, uint32_t tid);
bool vm_trace_begin_len(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, signed long name_len);
bool vm_trace_begin_len_tid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, signed long name_len, uint32_t tid);
bool vm_trace_begin_tid_pid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, uint32_t tid, uint32_t pid);
bool vm_trace_begin_len_tid_pid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, signed long name_len, uint32_t tid, uint32_t pid);

// End events
bool vm_trace_end(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when);
bool vm_trace_end_tid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, uint32_t tid);
bool vm_trace_end_tid_pid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, uint32_t tid, uint32_t pid);

#ifdef __cplusplus
}
#endif

#endif  // SPALL_H

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
