// adapted from
// https://nachtimwald.com/2019/04/12/thread-pool-in-c/

#pragma once
#include "libc.h"
#include <pthread.h>

struct vm_pool_t;
struct vm_pool_work_t;
typedef struct vm_pool_t vm_pool_t;
typedef struct vm_pool_work_t vm_pool_work_t;

typedef void (*vm_pool_func_t)(void *arg);

struct vm_pool_work_t {
    vm_pool_func_t func;
    void *arg;
    vm_pool_work_t *next;
};

struct vm_pool_t {
    vm_pool_work_t *work_first;
    vm_pool_work_t *work_last;
    pthread_mutex_t work_mutex;
    pthread_cond_t work_cond;
    pthread_cond_t working_cond;
    size_t working_cnt;
    size_t thread_cnt;
    bool stop;
};


vm_pool_t *vm_pool_create(size_t num);
void vm_pool_destroy(vm_pool_t *tm);

bool vm_pool_add_work(vm_pool_t *tm, vm_pool_func_t func, void *arg);
void vm_pool_wait(vm_pool_t *tm);

vm_pool_work_t *vm_pool_work_create(vm_pool_func_t func, void *arg);
void vm_pool_work_destroy(vm_pool_work_t *work);
void *vm_pool_worker(void *arg);