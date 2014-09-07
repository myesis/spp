#ifndef AVIARY_ALLOC_H__
#define AVIARY_ALLOC_H__

#define AVIARY_ALC_INLINE static AVIARY_INLINE

#define AVIARY_CACHE_LINE_SIZE 64
#define AVIARY_CACHE_LINE_MASK (AVIARY_CACHE_LINE_SIZE-1)

#define AVIARY_ALC_CACHE_LINE_ALIGN_SIZE(SZ) \
  (((((SZ) - 1) / AVIARY_CACHE_LINE_SIZE) + 1) * AVIARY_CACHE_LINE_SIZE)


#endif