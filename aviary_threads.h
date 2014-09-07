#ifndef AVIARY_THREADS_H__
#define AVIARY_THREADS_H__

#include "sys.h"
//#include "aviary_mutex.h"
//#include "athr_mutex.h"
#include "athread.h"
//#include "athr_atomics.h"

#define AVIARY_SCHEDULER_IDX(IDX)                                           \
  (ASSERT(0 <= (IDX) && (IDX) < aviary_no_schedulers),                      \
   &aviary_aligned_scheduler_data[(IDX)].esd)

typedef int aviary_aint32_t;
typedef long long aviary_aint64_t;

typedef aviary_aint32_t athr_sint32_t;
typedef aviary_aint64_t athr_sint64_t;

typedef athr_tsd_key aviary_tsd_key_t;

typedef struct {
    athr_mutex mtx;
} aviary_mtx_t;

typedef struct {
    athr_cond cond;
} aviary_cnd_t;

//AVIARY_GLOBAL_INLINE void
static inline void
aviary_mtx_lock(aviary_mtx_t *mutex) {
    athr_mutex_lock(&mutex->mtx);
}

static inline void
aviary_mtx_unlock(aviary_mtx_t *mutex) {
    athr_mutex_unlock(&mutex->mtx);
}

static inline void
aviary_tsd_key_create(aviary_tsd_key_t *keyp, char *keyname) {
    athr_tsd_key_create(keyp, keyname);
}

static inline void
aviary_tsd_set(aviary_tsd_key_t key, void *value) {
    athr_tsd_set(key, value);
}

static inline void *
aviary_tsd_get(aviary_tsd_key_t key) {
    return athr_tsd_get(key);
}

#define aviary_atomic32_init_nob athr_atomic32_init
#define aviary_atomic32_read_nob athr_atomic32_read
#define aviary_atomic32_cmpxchg_relb athr_atomic32_cmpxchg_relb
#define aviary_atomic32_cmpxchg_acqb athr_atomic32_cmpxchg_acqb
#define aviary_atomic32_cmpxchg_mb athr_atomic32_cmpxchg_mb
#define aivary_aint32_t int

#endif