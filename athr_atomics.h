#ifndef ATHR_ATOMICS_H__
#define ATHR_ATOMICS_H__

//#include "athread.h"
//#include "atomic.h"
//#include "athr_membar.h"

//typedef athr_native_atomic64_t athr_atomic32_t;

typedef athr_sint32_t athr_atomic32_t;

typedef athr_native_atomic64_t athr_atomic_t;

#define ATHR_INLINE_ATMC32_FUNC_NAME_(X) X ## __
#define ATHR_ATMC32_FUNC__(X) ATHR_INLINE_ATMC32_FUNC_NAME_(athr_atomic32_ ## X)

#define ATHR_INLINE_ATMC_FUNC_NAME_(X) X ## __
#define ATHR_ATMC_FUNC__(X) ATHR_INLINE_ATMC_FUNC_NAME_(athr_atomic_ ## X)

#define ATHR_NATMC32_FUNC__(X) athr_native_atomic32_ ## X

#define ATHR_NATMC_FUNC__(X) athr_native_atomic64_ ## X

#define ATHR_NAINT32_T__ athr_sint32_t
#define ATHR_NAINT_T__ athr_sint_t

#define ATHR_COMPILER_BARRIER __asm__ __volatile__("" : : : "memory")

#define ATHR_NATMC32_CMPXCHG_FALLBACK_READ__(VAR) \
  ATHR_NATMC32_FUNC__(read)(VAR)

#define ATHR_NATMC32_CMPXCHG_FALLBACK__(CMPXCHG, VAR, AVAL, OPS) \
do { \
    athr_sint32_t AVAL; \
    ATHR_NAINT32_T__ new__, act__, exp__; \
    act__ = ATHR_NATMC32_CMPXCHG_FALLBACK_READ__(VAR); \
    do { \
        exp__ = act__; \
        AVAL = (athr_sint32_t) act__; \
        { OPS; } \
        new__ = (ATHR_NAINT32_T__) AVAL; \
        act__ = CMPXCHG(VAR, new__, exp__); \
    } while (__builtin_expect(act__ != exp__, 0)); \
} while (0)


static inline void ATHR_ATMC32_FUNC__(init)(athr_atomic32_t *var, athr_sint32_t val)
{
    //    ATHR_ATMC32_FUNC__(set)(var, val);
    ATHR_NATMC32_FUNC__(set)(var, (ATHR_NAINT32_T__) val);
}

static inline athr_sint32_t ATHR_ATMC32_FUNC__(read)(athr_atomic32_t *var) {
    athr_sint32_t res;
    res = (athr_sint32_t) ATHR_NATMC32_FUNC__(read)(var);
    return res;
}

static inline athr_sint32_t ATHR_ATMC32_FUNC__(read_bor)(athr_atomic32_t *var, athr_sint32_t val)
{
    athr_sint32_t res;
    ATHR_NATMC32_CMPXCHG_FALLBACK__(ATHR_NATMC32_FUNC__(cmpxchg_mb), var, aval, res = aval; aval |= val);
    return res;
}

static inline athr_sint32_t ATHR_ATMC32_FUNC__(read_acqb)(athr_atomic32_t *var) {
    athr_sint32_t res;
    res = (athr_sint32_t) ATHR_NATMC32_FUNC__(read)(var);
    ATHR_MEMBAR(ATHR_LoadLoad|ATHR_LoadStore);
    return res;
}


static inline athr_sint32_t ATHR_ATMC32_FUNC__(cmpxchg_relb)(
    athr_atomic32_t *var,
    athr_sint32_t val,
    athr_sint32_t old_val) {

    athr_sint32_t res;
    res = (athr_sint32_t) ATHR_NATMC32_FUNC__(cmpxchg_mb)(var, (ATHR_NAINT32_T__) val, (ATHR_NAINT32_T__) old_val);
    return res;
}

static inline athr_sint32_t ATHR_ATMC32_FUNC__(cmpxchg_mb)(
    athr_atomic32_t *var,
    athr_sint32_t val,
    athr_sint32_t old_val) {

    athr_sint32_t res;
    res = (athr_sint32_t) ATHR_NATMC32_FUNC__(cmpxchg_mb)(var, (ATHR_NAINT32_T__) val, (ATHR_NAINT32_T__) old_val);
    return res;
}

static inline void ATHR_ATMC32_FUNC__(set_relb)(athr_atomic32_t *var, athr_sint32_t val)
{
    ATHR_MEMBAR(ATHR_LoadStore|ATHR_StoreStore);
    ATHR_NATMC32_FUNC__(set)(var, (ATHR_NAINT32_T__) val);
}

static inline athr_sint_t ATHR_ATMC_FUNC__(read_acqb)(athr_atomic_t *var)
{
    athr_sint_t res;
    res = (athr_sint_t) ATHR_NATMC_FUNC__(read)(var);
    ATHR_MEMBAR(ATHR_LoadLoad|ATHR_LoadStore);
    return res;
}

static inline void ATHR_ATMC_FUNC__(set)(athr_atomic_t *var, athr_sint_t val)
{
    ATHR_NATMC_FUNC__(set)(var, (ATHR_NAINT_T__) val);
}

static inline athr_sint_t ATHR_ATMC_FUNC__(read)(athr_atomic_t *var) {
    athr_sint_t res;
    res = (athr_sint32_t) ATHR_NATMC_FUNC__(read)(var);
    return res;
}

#endif