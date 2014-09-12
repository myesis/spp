#ifndef ATHREAD_H__
#define ATHREAD_H__

#include <pthread.h>

//#include "aviary_mutex.h"
//#include "athr_mutex.h"

#include "athr_membar.h"

#define ATHR_CHOOSE_EXPR __builtin_choose_expr

typedef pthread_t athr_tid;
typedef pthread_key_t athr_tsd_key;

typedef struct {
    int detached;                       /* boolean (default false) */
    int suggested_stack_size;           /* kilo words (default sys dependent) */
} athr_thr_opts;

typedef int athr_sint32_t;
typedef long long athr_sint64_t;

typedef athr_sint64_t athr_sint_t;

int
athr_tsd_key_create(athr_tsd_key *keyp, char *keyname);

int
athr_tsd_set(athr_tsd_key key, void *value);

void *
athr_tsd_get(athr_tsd_key key);

#define ATHR_THR_OPTS_DEFAULT_INITER {0, -1}

#define ATHR_ATOMIC_WANT_32BIT_IMPL__
#include "atomic.h"
#define ATHR_ATOMIC_WANT_64BIT_IMPL__
#include "atomic.h"

#include "athr_atomics.h"

#include "aviary_mutex.h"
#include "athr_mutex.h"

#endif