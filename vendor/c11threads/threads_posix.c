/*
 * C11 <threads.h> emulation library
 *
 * (C) Copyright yohhoy 2012.
 * Distributed under the Boost Software License, Version 1.0.
 * (See copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

/*
Configuration macro:

  EMULATED_THREADS_USE_NATIVE_TIMEDLOCK
    Use pthread_mutex_timedlock() for `mtx_timedlock()'
    Otherwise use mtx_trylock() + *busy loop* emulation.
*/
#if !defined(__CYGWIN__) && !defined(__APPLE__)
#define EMULATED_THREADS_USE_NATIVE_TIMEDLOCK
#endif


#include "threads.h"

/*
Implementation limits:
  - Conditionally emulation for "mutex with timeout"
    (see EMULATED_THREADS_USE_NATIVE_TIMEDLOCK macro)
*/
struct impl_thrd_param {
    thrd_start_t func;
    void* arg;
};

void* impl_thrd_routine(void* p) {
    struct impl_thrd_param pack = *((struct impl_thrd_param*)p);
    free(p);
    return (void*)((size_t)pack.func(pack.arg));
}

/*--------------- 7.25.2 Initialization functions ---------------*/
// 7.25.2.1
void call_once(once_flag* flag, void (*func)(void)) {
    pthread_once(flag, func);
}

/*------------- 7.25.3 Condition variable functions -------------*/
// 7.25.3.1
int cnd_broadcast(cnd_t* cond) {
    if (!cond) return thrd_error;
    pthread_cond_broadcast(cond);
    return thrd_success;
}

// 7.25.3.2
void cnd_destroy(cnd_t* cond) {
    assert(cond);
    pthread_cond_destroy(cond);
}

// 7.25.3.3
int cnd_init(cnd_t* cond) {
    if (!cond) return thrd_error;
    pthread_cond_init(cond, NULL);
    return thrd_success;
}

// 7.25.3.4
int cnd_signal(cnd_t* cond) {
    if (!cond) return thrd_error;
    pthread_cond_signal(cond);
    return thrd_success;
}

// 7.25.3.5
int cnd_timedwait(cnd_t* cond, mtx_t* mtx, const xtime* xt) {
    struct timespec abs_time;
    int rt;
    if (!cond || !mtx || !xt) return thrd_error;
    rt = pthread_cond_timedwait(cond, mtx, &abs_time);
    if (rt == ETIMEDOUT)
        return thrd_busy;
    return (rt == 0) ? thrd_success : thrd_error;
}

// 7.25.3.6
int cnd_wait(cnd_t* cond, mtx_t* mtx) {
    if (!cond || !mtx) return thrd_error;
    pthread_cond_wait(cond, mtx);
    return thrd_success;
}

/*-------------------- 7.25.4 Mutex functions --------------------*/
// 7.25.4.1
void mtx_destroy(mtx_t* mtx) {
    assert(mtx);
    pthread_mutex_destroy(mtx);
}

// 7.25.4.2
int mtx_init(mtx_t* mtx, int type) {
    pthread_mutexattr_t attr;
    if (!mtx) return thrd_error;
    if (type != mtx_plain && type != mtx_timed && type != mtx_try && type != (mtx_plain | mtx_recursive) && type != (mtx_timed | mtx_recursive) && type != (mtx_try | mtx_recursive))
        return thrd_error;
    pthread_mutexattr_init(&attr);
    if ((type & mtx_recursive) != 0) {
#if defined(__linux__) || defined(__linux)
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#else
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#endif
    }
    pthread_mutex_init(mtx, &attr);
    pthread_mutexattr_destroy(&attr);
    return thrd_success;
}

// 7.25.4.3
int mtx_lock(mtx_t* mtx) {
    if (!mtx) return thrd_error;
    pthread_mutex_lock(mtx);
    return thrd_success;
}

// 7.25.4.4
int mtx_timedlock(mtx_t* mtx, const xtime* xt) {
    if (!mtx || !xt) return thrd_error;
    {
#ifdef EMULATED_THREADS_USE_NATIVE_TIMEDLOCK
        struct timespec ts;
        int rt;
        ts.tv_sec = xt->sec;
        ts.tv_nsec = xt->nsec;
        rt = pthread_mutex_timedlock(mtx, &ts);
        if (rt == 0)
            return thrd_success;
        return (rt == ETIMEDOUT) ? thrd_busy : thrd_error;
#else
        time_t expire = time(NULL);
        expire += xt->sec;
        while (mtx_trylock(mtx) != thrd_success) {
            time_t now = time(NULL);
            if (expire < now)
                return thrd_busy;
            // busy loop!
            thrd_yield();
        }
        return thrd_success;
#endif
    }
}

// 7.25.4.5
int mtx_trylock(mtx_t* mtx) {
    if (!mtx) return thrd_error;
    return (pthread_mutex_trylock(mtx) == 0) ? thrd_success : thrd_busy;
}

// 7.25.4.6
int mtx_unlock(mtx_t* mtx) {
    if (!mtx) return thrd_error;
    pthread_mutex_unlock(mtx);
    return thrd_success;
}

/*------------------- 7.25.5 Thread functions -------------------*/
// 7.25.5.1
int thrd_create(thrd_t* thr, thrd_start_t func, void* arg) {
    struct impl_thrd_param* pack;
    if (!thr) return thrd_error;
    pack = malloc(sizeof(struct impl_thrd_param));
    if (!pack) return thrd_nomem;
    pack->func = func;
    pack->arg = arg;

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if (pthread_create(thr, &attr, impl_thrd_routine, pack) != 0) {
        free(pack);
        return thrd_error;
    }
    return thrd_success;
}

// 7.25.5.2
thrd_t thrd_current(void) {
    return pthread_self();
}

// 7.25.5.3
int thrd_detach(thrd_t thr) {
    return (pthread_detach(thr) == 0) ? thrd_success : thrd_error;
}

// 7.25.5.4
int thrd_equal(thrd_t thr0, thrd_t thr1) {
    return pthread_equal(thr0, thr1);
}

// 7.25.5.5
void thrd_exit(int res) {
    pthread_exit((void*)((size_t)res));
}

// 7.25.5.6
int thrd_join(thrd_t thr, int* res) {
    void* code;
    if (pthread_join(thr, &code) != 0)
        return thrd_error;
    if (res)
        *res = (int)((size_t)code);
    return thrd_success;
}

// 7.25.5.7
void thrd_sleep(const xtime* xt) {
    struct timespec req;
    assert(xt);
    req.tv_sec = xt->sec;
    req.tv_nsec = xt->nsec;
    nanosleep(&req, NULL);
}

// 7.25.5.8
void thrd_yield(void) {
    sched_yield();
}

/*----------- 7.25.6 Thread-specific storage functions -----------*/
// 7.25.6.1
int tss_create(tss_t* key, tss_dtor_t dtor) {
    if (!key) return thrd_error;
    return (pthread_key_create(key, dtor) == 0) ? thrd_success : thrd_error;
}

// 7.25.6.2
void tss_delete(tss_t key) {
    pthread_key_delete(key);
}

// 7.25.6.3
void* tss_get(tss_t key) {
    return pthread_getspecific(key);
}

// 7.25.6.4
int tss_set(tss_t key, void* val) {
    return (pthread_setspecific(key, val) == 0) ? thrd_success : thrd_error;
}

/*-------------------- 7.25.7 Time functions --------------------*/
// 7.25.6.1
int xtime_get(xtime* xt, int base) {
    if (!xt) return 0;
    if (base == TIME_UTC) {
        xt->sec = time(NULL);
        xt->nsec = 0;
        return base;
    }
    return 0;
}
