#include "athr_atomics.h"

void athr_atomic32_init(athr_atomic32_t *var, athr_sint32_t val)
{
    athr_atomic32_init__(var, val);
}

athr_sint32_t athr_atomic32_read(athr_atomic32_t *var) {
    athr_sint32_t res;
    res = athr_atomic32_read__(var);
    return res;
}

athr_sint32_t athr_atomic32_cmpxchg_relb(athr_atomic32_t *var, athr_sint32_t val, athr_sint32_t old_val) {
    athr_sint32_t res;
    res = athr_atomic32_cmpxchg_mb__(var, val, old_val);
    return res;
}

athr_sint32_t athr_atomic32_cmpxchg_acqb(athr_atomic32_t *var, athr_sint32_t val, athr_sint32_t old_val) {
    athr_sint32_t res;
    res = athr_atomic32_cmpxchg_mb__(var, val, old_val);
    return res;
}

athr_sint32_t athr_atomic32_cmpxchg_mb(athr_atomic32_t *var, athr_sint32_t val, athr_sint32_t old_val) {
    athr_sint32_t res;
    res = athr_atomic32_cmpxchg_mb__(var, val, old_val);
    return res;
}