#ifndef ATHR_ATOMICS_H__
#define ATHR_ATOMICS_H__

#include "atomic.h"

//typedef athr_native_atomic64_t athr_atomic32_t;

typedef athr_sint32_t athr_atomic32_t;

#define ATHR_INLINE_ATMC32_FUNC_NAME_(X) X ## __
#define ATHR_ATMC32_FUNC__(X) ATHR_INLINE_ATMC32_FUNC_NAME_(athr_atomic32_ ## X)

#define ATHR_NAINT32_T__ athr_sint32_t

#define ATHR_COMPILER_BARRIER __asm__ __volatile__("" : : : "memory")

static inline void ATHR_ATMC32_FUNC__(init)(athr_atomic32_t *var, athr_sint32_t val)
{
    //    ATHR_ATMC32_FUNC__(set)(var, val);
    ATHR_NATMC_FUNC__(set)(var, (ATHR_NAINT32_T__) val);
}

static inline athr_sint32_t ATHR_ATMC32_FUNC__(read)(athr_atomic32_t *var) {
    athr_sint32_t res;
    res = (athr_sint32_t) ATHR_NATMC_FUNC__(read)(var);
    return res;
}

static inline athr_sint_t ATHR_ATMC_FUNC__(read)(athr_atomic_t *var)
{
    athr_sint_t res;
    res = (athr_sint_t) ATHR_NATMC_FUNC__(read)(var);
    return res;
}

static inline athr_sint32_t ATHR_ATMC32_FUNC__(cmpxchg_relb)(
    athr_atomic32_t *var,
    athr_sint32_t val,
    athr_sint32_t old_val) {

    athr_sint32_t res;
    res = (athr_sint32_t) ATHR_NATMC_FUNC__(cmpxchg_mb)(var, (ATHR_NAINT32_T__) val, (ATHR_NAINT32_T__) old_val);
    return res;
}

static inline athr_sint32_t ATHR_ATMC32_FUNC__(cmpxchg_mb)(
    athr_atomic32_t *var,
    athr_sint32_t val,
    athr_sint32_t old_val) {

    athr_sint32_t res;
    res = (athr_sint32_t) ATHR_NATMC_FUNC__(cmpxchg_mb)(var, (ATHR_NAINT32_T__) val, (ATHR_NAINT32_T__) old_val);
    return res;
}

#endif