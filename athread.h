#ifndef ATHREAD_H__
#define ATHREAD_H__

#include <pthread.h>

#include "aviary_mutex.h"
#include "athr_mutex.h"

typedef pthread_t athr_tid;
typedef pthread_key_t athr_tsd_key;

typedef struct {
    int detached;                       /* boolean (default false) */
    int suggested_stack_size;           /* kilo words (default sys dependent) */
} athr_thr_opts;

typedef int athr_sint32_t;

int
athr_tsd_key_create(athr_tsd_key *keyp, char *keyname);

int
athr_tsd_set(athr_tsd_key key, void *value);

void *
athr_tsd_get(athr_tsd_key key);

#define ATHR_THR_OPTS_DEFAULT_INITER {0, -1}


#endif