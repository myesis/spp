#ifndef ATHR_MUTEX_H__
#define ATHR_MUTEX_H__
#include <pthread.h>

static inline void
athr_mutex_lock(athr_mutex *mtx) {
    int res = pthread_mutex_lock(&mtx->pt_mtx);
}

static inline void
athr_mutex_unlock(athr_mutex *mtx) {
    int res = pthread_mutex_unlock(&mtx->pt_mtx);
}
#endif