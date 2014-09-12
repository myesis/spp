#ifndef AVIARY_THREADS_H__
#define AVIARY_THREADS_H__

#include "sys.h"
//#include "aviary_mutex.h"
//#include "athr_mutex.h"
#include "athread.h"
//#include "aviary_atomic.h"
//#include "athr_atomics.h"

#define AVIARY_SCHEDULER_IDX(IDX)                                           \
  (ASSERT(0 <= (IDX) && (IDX) < aviary_no_schedulers),                      \
   &aviary_aligned_scheduler_data[(IDX)].esd)

#define AVIARY_ATOMIC_BSET_IMPL__(Type, ReadOp, CmpxchgOp, VarP, Mask, Set) \
do {                                                                    \
    Type act = ReadOp((VarP));                                          \
    while (1) {                                                         \
        Type exp = act;                                                 \
        Type new = exp & ~(Mask);                                       \
        new |= ((Mask) & (Set));                                        \
        act = CmpxchgOp((VarP), new, exp);                              \
        if (act == exp)                                                 \
            return act;                                                 \
    }                                                                   \
} while (0)


typedef long aviary_aint_t;
typedef int aviary_aint32_t;
typedef long long aviary_aint64_t;

typedef aviary_aint32_t athr_sint32_t;
typedef aviary_aint64_t athr_sint64_t;

typedef athr_tsd_key aviary_tsd_key_t;

#define aviary_atomic32_t athr_atomic32_t
#define aviary_atomic_t athr_atomic_t

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

static inline aviary_aint32_t
aviary_atomic32_read_bset_relb(aviary_atomic32_t *var,
    aviary_aint32_t mask,
    aviary_aint32_t set)
{
    AVIARY_ATOMIC_BSET_IMPL__(aviary_aint32_t,
                        athr_atomic32_read,
                        athr_atomic32_cmpxchg_relb,
                        var, mask, set);
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
#define aviary_atomic_read_nob athr_atomic_read

#define aviary_atomic32_init_nob athr_atomic32_init
#define aviary_atomic32_read_nob athr_atomic32_read
#define aviary_atomic32_read_acqb athr_atomic32_read_acqb
#define aviary_atomic32_cmpxchg_relb athr_atomic32_cmpxchg_relb
#define aviary_atomic32_cmpxchg_acqb athr_atomic32_cmpxchg_acqb
#define aviary_atomic32_cmpxchg_mb athr_atomic32_cmpxchg_mb
#define aviary_atomic32_read_bor_nob athr_atomic32_read_bor
#define aviary_atomic32_set_relb athr_atomic32_set_relb

#define aviary_atomic_set_nob athr_atomic_set

#define aivary_aint32_t int

#endif