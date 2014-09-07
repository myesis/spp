#include "sys.h"
#include "aviary_alloc.h"


#ifndef AVIARY_CACHE_LINE_SIZE
#define AVIARY_CACHE_LINE_SIZE 64
#define AVIARY_CACHE_LINE_MASK (AVIARY_CACHE_LINE_SIZE - 1)
#endif

#define AVIARY_MEM_ALLOC(size) malloc(size)
#define AVIARY_MEM_REALLOC(ptr, size) realloc(ptr, size)
#define AVIARY_MEM_FREE(ptr) free(ptr)

#define AVIARY_STACK_ALLOC(size) aviary_mmap(size);
void *
aviary_mmap(Uint size) {
    void *p = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1,
        0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    return p;
}

inline
void *aviary_alloc(Uint size) {
    void *res;
    res = AVIARY_MEM_ALLOC(size);
    if (!res) {
        //        aviary_alloc_n_enomem(size);
    }
    return res;
}

inline
void *aviary_realloc(void *ptr, Uint size) {
    void *res;
    res = AVIARY_MEM_REALLOC(ptr, size);
    if (!res) {
        //        aviary_realloc_n_enomem(size);
    }
    return res;
}

inline
void aviary_free(void *ptr) {
    AVIARY_MEM_FREE(ptr);
}

void *aviary_alloc_permanent_cache_aligned(Uint size) {
    Uint v = (Uint) aviary_alloc(size + (AVIARY_CACHE_LINE_SIZE-1));
    if (v & AVIARY_CACHE_LINE_MASK) {
        v = (v & ~AVIARY_CACHE_LINE_MASK) + AVIARY_CACHE_LINE_SIZE;
    }
    return (void*)v;
}