#ifndef AVIARY_SMP_H__
#define AVIARY_SMP_H__
#include "aviary_threads.h"

typedef aviary_mtx_t aviary_smp_mtx_t;
typedef aviary_cnd_t aviary_smp_cnd_t;

#define aviary_smp_atomic32_t aviary_atomic32_t

#define aviary_smp_atomic32_init_nob aviary_atomic32_init_nob
#define aviary_smp_atomic32_read_nob aviary_atomic32_read_nob
#define aviary_smp_atomic32_cmpxchg_acqb aviary_atomic32_cmpxchg_acqb
#define aviary_smp_atomic32_cmpxchg_relb aviary_atomic32_cmpxchg_relb
#define aviary_smp_atomic32_cmpxchg_mb aviary_atomic32_cmpxchg_mb

static inline void
aviary_smp_mtx_lock(aviary_smp_mtx_t *mtx) {
    aviary_mtx_lock(mtx);
}

static inline void
aviary_smp_mtx_unlock(aviary_smp_mtx_t *mtx) {
    aviary_mtx_unlock(mtx);
}

#endif