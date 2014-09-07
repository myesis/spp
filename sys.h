#ifndef SYS_H__
#define SYS_H__

#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>

#define AIVARY_INLINE inline

#define AVIARY_GLOBAL_INLINE static AVIARY_INLINE

typedef unsigned long long Uint64;

typedef unsigned long long Uint;

typedef unsigned int Uint32;

typedef unsigned short Uint16;

#define ASSERT(e) assert(e)

#endif