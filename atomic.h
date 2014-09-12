//#ifndef ATOMIC_H__
//#define ATOMIC_H__

#include "athread.h"
#if defined(ATHR_ATOMIC_WANT_32BIT_IMPL__)
#define ATHR_INCLUDE_ATOMIC_IMPL__ 4
#undef ATHR_ATOMIC_WANT_32BIT_IMPL__
#elif defined(ATHR_ATOMIC_WANT_64BIT_IMPL__)
#ifdef ATHR_INCLUDE_ATOMIC_IMPL__
#undef ATHR_INCLUDE_ATOMIC_IMPL__
#endif
#define ATHR_INCLUDE_ATOMIC_IMPL__ 8
#undef ATHR_ATOMIC_WANT_64BIT_IMPL__
#endif

#if ATHR_INCLUDE_ATOMIC_IMPL__ == 4
#define ATHR_AINT_T__ athr_sint32_t
#define ATHR_ATMC_T__ athr_native_atomic32_t
#define ATHR_NATMC_FUNC__(X) athr_native_atomic32_ ## X
#define ATHR_AINT_SUFFIX__ "l"
#elif ATHR_INCLUDE_ATOMIC_IMPL__ == 8
#define ATHR_AINT_T__ athr_sint64_t
#define ATHR_ATMC_T__ athr_native_atomic64_t
#define ATHR_NATMC_FUNC__(X) athr_native_atomic64_ ## X
#define ATHR_AINT_SUFFIX__ "q"
#endif

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

#undef ATHR_NATMC_FUNC__
#undef ATHR_ATMC_T__
#undef ATHR_AINT_T__
#undef ATHR_AINT_SUFFIX__

//#endif