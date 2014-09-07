#ifndef AVIARY_ATOMIC_H__
#define AVIARY_ATOMIC_H__

#include "sys.h"
#include "aviary_threads.h"

typedef struct {
    volatile athr_sint64_t counter;
} aviary_atomic_t;

typedef struct {
    volatile athr_sint32_t counter;
} aviary_atomic32_t;

#endif