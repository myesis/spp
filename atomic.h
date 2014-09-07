#ifndef ATOMIC_H__
#define ATOMIC_H__

#include "athread.h"

#define ATHR_AINT_T__ athr_sint32_t
//#define ATHR_ATMC_T__ athr_native_atomic32_t
#define ATHR_NATMC_FUNC__(X) athr_native_atomic32_ ## X

#define ATHR_AINT_SUFFIX__ "l"

typedef struct {
    volatile ATHR_AINT_T__ counter;
} ATHR_ATMC_T__;

static inline void
ATHR_NATMC_FUNC__(set)(ATHR_ATMC_T__ *var, ATHR_AINT_T__ i)
{
    var->counter = i;
}

static inline ATHR_AINT_T__
ATHR_NATMC_FUNC__(read)(ATHR_ATMC_T__ *var)
{
    return var->counter;
}

static inline ATHR_AINT_T__
ATHR_NATMC_FUNC__(read)(ATHR_ATMC_T__ *var)
{
    return var->counter;
}

static inline ATHR_AINT_T__
ATHR_NATMC_FUNC__(cmpxchg_mb)(ATHR_ATMC_T__ *var,
                              ATHR_AINT_T__ new,
                              ATHR_AINT_T__ old)
{
    __asm__ __volatile__(
      "lock; cmpxchg" ATHR_AINT_SUFFIX__ " %2, %3"
      : "=a"(old), "=m"(var->counter)
      : "r"(new), "m"(var->counter), "0"(old)
      : "cc", "memory"); /* full memory clobber to make this a compiler barrier */
    return old;
}


#endif