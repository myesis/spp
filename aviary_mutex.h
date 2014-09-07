#ifndef AVIARY_MUTEX_H__
#define AVIARY_MUTEX_H__

#include <pthread.h>

#define ATHR_MTX_QLOCK_TYPE__ pthread_mutex_t

typedef struct athr_mutex_ athr_mutex;
struct athr_mutex_ {
    pthread_mutex_t pt_mtx;
};

typedef struct athr_cond_ athr_cond;
struct athr_cond_ {
    pthread_cond_t pt_cnd;
};
#endif