#include "athread.h"

athr_sint_t athr_atomic_read(athr_atomic_t *var)
{
    athr_sint_t res;
    res = athr_atomic_read__(var);
    return res;
}

void athr_atomic32_init(athr_atomic32_t *var, athr_sint32_t val)
{
    athr_atomic32_init__(var, val);
}

athr_sint32_t athr_atomic32_read(athr_atomic32_t *var) {
    athr_sint32_t res;
    res = athr_atomic32_read__(var);
    return res;
}

athr_sint32_t athr_atomic32_read_bor(athr_atomic32_t *var, athr_sint32_t val)
{
    athr_sint32_t res;
    res = athr_atomic32_read_bor__(var, val);
    return res;
}

athr_sint32_t athr_atomic32_read_acqb(athr_atomic32_t *var) {
    athr_sint32_t res;
    res = athr_atomic32_read_acqb__(var);
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

void athr_atomic32_set_relb(athr_atomic32_t *var, athr_sint32_t val)
{
    athr_atomic32_set_relb__(var, val);
}

void athr_atomic_set(athr_atomic_t *var, athr_sint_t val)
{
    athr_atomic_set__(var, val);
}