// adapted from
// https://nachtimwald.com/2019/04/12/thread-pool-in-c/

#include "thread.h"
#include <unistd.h>

vm_pool_work_t *vm_pool_work_create(vm_pool_func_t func, void *arg)
{
    if (func == NULL)
        return NULL;

    vm_pool_work_t *work = vm_malloc(sizeof(*work));
    work->func = func;
    work->arg = arg;
    work->next = NULL;
    return work;
}

void vm_pool_work_destroy(vm_pool_work_t *work)
{
    if (work == NULL)
        return;
    vm_free(work);
}

vm_pool_work_t *vm_pool_work_get(vm_pool_t *tm)
{
    if (tm == NULL)
        return NULL;
    vm_pool_work_t *work = tm->work_first;
    if (work == NULL)
        return NULL;
    if (work->next == NULL) {
        tm->work_first = NULL;
        tm->work_last = NULL;
    } else {
        tm->work_first = work->next;
    }

    return work;
}

void *vm_pool_worker(void *arg)
{
    vm_pool_t *tm = arg;
    vm_pool_work_t *work;

    while (1) {
        pthread_mutex_lock(&(tm->work_mutex));
        while (tm->work_first == NULL && !tm->stop)
            pthread_cond_wait(&(tm->work_cond), &(tm->work_mutex));
        if (tm->stop)
            break;
        work = vm_pool_work_get(tm);
        tm->working_cnt++;
        pthread_mutex_unlock(&(tm->work_mutex));
        if (work != NULL) {
            work->func(work->arg);
            vm_pool_work_destroy(work);
        }
        pthread_mutex_lock(&(tm->work_mutex));
        tm->working_cnt--;
        if (!tm->stop && tm->working_cnt == 0 && tm->work_first == NULL)
            pthread_cond_signal(&(tm->working_cond));
        pthread_mutex_unlock(&(tm->work_mutex));
    }
    tm->thread_cnt--;
    pthread_cond_signal(&(tm->working_cond));
    pthread_mutex_unlock(&(tm->work_mutex));
    return NULL;
}

vm_pool_t *vm_pool_create(size_t num)
{
    vm_pool_t *tm;
    pthread_t thread;
    size_t i;

    if (num == 0)
        num = 2;
    tm = vm_malloc(sizeof(*tm));
    tm->thread_cnt = num;

    pthread_mutex_init(&(tm->work_mutex), NULL);
    pthread_cond_init(&(tm->work_cond), NULL);
    pthread_cond_init(&(tm->working_cond), NULL);

    tm->work_first = NULL;
    tm->work_last = NULL;

    for (i=0; i<num; i++) {
        pthread_create(&thread, NULL, vm_pool_worker, tm);
        pthread_detach(thread);
    }

    return tm;
}

void vm_pool_destroy(vm_pool_t *tm)
{
    vm_pool_work_t *work;
    vm_pool_work_t *work2;

    if (tm == NULL)
        return;

    pthread_mutex_lock(&(tm->work_mutex));
    work = tm->work_first;
    while (work != NULL) {
        work2 = work->next;
        vm_pool_work_destroy(work);
        work = work2;
    }
    tm->stop = true;
    pthread_cond_broadcast(&(tm->work_cond));
    pthread_mutex_unlock(&(tm->work_mutex));

    vm_pool_wait(tm);

    pthread_mutex_destroy(&(tm->work_mutex));
    pthread_cond_destroy(&(tm->work_cond));
    pthread_cond_destroy(&(tm->working_cond));

    vm_free(tm);
}

bool vm_pool_add_work(vm_pool_t *tm, vm_pool_func_t func, void *arg)
{
    vm_pool_work_t *work;

    if (tm == NULL)
        return false;

    work = vm_pool_work_create(func, arg);
    if (work == NULL)
        return false;

    pthread_mutex_lock(&(tm->work_mutex));
    if (tm->work_first == NULL) {
        tm->work_first = work;
        tm->work_last = tm->work_first;
    } else {
        tm->work_last->next = work;
        tm->work_last = work;
    }

    pthread_cond_broadcast(&(tm->work_cond));
    pthread_mutex_unlock(&(tm->work_mutex));

    return true;
}

void vm_pool_wait(vm_pool_t *tm)
{
    if (tm == NULL)
        return;

    pthread_mutex_lock(&(tm->work_mutex));
    while (1) {
        if ((!tm->stop && tm->working_cnt != 0) || (tm->stop && tm->thread_cnt != 0)) {
            pthread_cond_wait(&(tm->working_cond), &(tm->work_mutex));
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&(tm->work_mutex));
}
