/*
 * C11 <threads.h> emulation library
 *
 * (C) Copyright yohhoy 2012.
 * Distributed under the Boost Software License, Version 1.0.
 * (See copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef EMULATED_THREADS_H_INCLUDED_
#define EMULATED_THREADS_H_INCLUDED_

#include <time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// check configuration
#if defined(EMULATED_THREADS_USE_NATIVE_CALL_ONCE) && (_WIN32_WINNT < 0x0600)
#error EMULATED_THREADS_USE_NATIVE_CALL_ONCE requires _WIN32_WINNT>=0x0600
#endif

#if defined(EMULATED_THREADS_USE_NATIVE_CV) && (_WIN32_WINNT < 0x0600)
#error EMULATED_THREADS_USE_NATIVE_CV requires _WIN32_WINNT>=0x0600
#endif

/*---------------------------- macros ----------------------------*/
#ifdef EMULATED_THREADS_USE_NATIVE_CALL_ONCE
#define ONCE_FLAG_INIT INIT_ONCE_STATIC_INIT
#else
#define ONCE_FLAG_INIT \
{ 0 }
#endif
#define TSS_DTOR_ITERATIONS 1

#ifndef thread_local
#define thread_local _Thread_local
#endif

/*---------------------------- types ----------------------------*/
typedef struct cnd_t {
    #ifdef EMULATED_THREADS_USE_NATIVE_CV
    CONDITION_VARIABLE condvar;
    #else
    int blocked;
    int gone;
    int to_unblock;
    HANDLE sem_queue;
    HANDLE sem_gate;
    CRITICAL_SECTION monitor;
    #endif
} cnd_t;

typedef HANDLE thrd_t;

typedef DWORD tss_t;

typedef struct mtx_t {
    CRITICAL_SECTION cs;
} mtx_t;

#ifdef EMULATED_THREADS_USE_NATIVE_CALL_ONCE
typedef INIT_ONCE once_flag;
#else
typedef struct once_flag_t {
    volatile LONG status;
} once_flag;
#endif

#elif defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <pthread.h>

/*---------------------------- macros ----------------------------*/
#define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#ifdef INIT_ONCE_STATIC_INIT
#define TSS_DTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS
#else
#define TSS_DTOR_ITERATIONS 1 // assume TSS dtor MAY be called at least once.
#endif

/*---------------------------- types ----------------------------*/
typedef pthread_cond_t cnd_t;
typedef pthread_t thrd_t;
typedef pthread_key_t tss_t;
typedef pthread_mutex_t mtx_t;
typedef pthread_once_t once_flag;

#else
#error Not supported on this platform.
#endif

/*---------------------------- types ----------------------------*/
typedef void (*tss_dtor_t)(void*);
typedef int (*thrd_start_t)(void*);

struct xtime {
    time_t sec;
    long nsec;
};
typedef struct xtime xtime;

/*-------------------- enumeration constants --------------------*/
enum {
    mtx_plain = 0,
    mtx_try = 1,
    mtx_timed = 2,
    mtx_recursive = 4
};

enum {
    thrd_success = 0, // succeeded
    thrd_timeout,     // timeout
    thrd_error,       // failed
    thrd_busy,        // resource busy
    thrd_nomem        // out of memory
};

/*-------------------------- functions --------------------------*/
void call_once(once_flag* flag, void (*func)(void));

int cnd_broadcast(cnd_t* cond);
void cnd_destroy(cnd_t* cond);
int cnd_init(cnd_t* cond);
int cnd_signal(cnd_t* cond);
int cnd_timedwait(cnd_t* cond, mtx_t* mtx, const xtime* xt);
int cnd_wait(cnd_t* cond, mtx_t* mtx);

void mtx_destroy(mtx_t* mtx);
int mtx_init(mtx_t* mtx, int type);
int mtx_lock(mtx_t* mtx);
int mtx_timedlock(mtx_t* mtx, const xtime* xt);
int mtx_trylock(mtx_t* mtx);
int mtx_unlock(mtx_t* mtx);

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg);
thrd_t thrd_current(void);
int thrd_detach(thrd_t thr);
int thrd_equal(thrd_t thr0, thrd_t thr1);
void thrd_exit(int res);
int thrd_join(thrd_t thr, int* res);
void thrd_sleep(const xtime* xt);
void thrd_yield(void);

int tss_create(tss_t* key, tss_dtor_t dtor);
void tss_delete(tss_t key);
void* tss_get(tss_t key);
int tss_set(tss_t key, void* val);

int xtime_get(xtime* xt, int base);
#define TIME_UTC 1

#endif /* EMULATED_THREADS_H_INCLUDED_ */
