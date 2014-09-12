/*
 * %CopyrightBegin%
 *
 * Copyright Ericsson AB 2011. All Rights Reserved.
 *
 * The contents of this file are subject to the Erlang Public License,
 * Version 1.1, (the "License"); you may not use this file except in
 * compliance with the License. You should have received a copy of the
 * Erlang Public License along with this software. If not, it can be
 * retrieved online at http://www.erlang.org/.
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * %CopyrightEnd%
 */

/*
 * Description: Memory barriers for x86/x86-64
 * Author: Rickard Green
 */

#ifndef ATHR_X86_MEMBAR_H__
#define ATHR_X86_MEMBAR_H__

#define ATHR_LoadLoad	(1 << 0)
#define ATHR_LoadStore	(1 << 1)
#define ATHR_StoreLoad	(1 << 2)
#define ATHR_StoreStore	(1 << 3)

#define ATHR_NO_SSE2_MEMORY_BARRIER__			\
do {							\
    volatile athr_sint32_t x__ = 0;			\
    __asm__ __volatile__ ("lock; orl $0x0, %0\n\t"	\
			  : "=m"(x__)			\
			  : "m"(x__)			\
			  : "memory");			\
} while (0)

static __inline__ void
athr_cfence__(void)
{
    __asm__ __volatile__ ("" : : : "memory");
}

static __inline__ void
athr_mfence__(void)
{
#if ATHR_SIZEOF_PTR == 4
    if (ATHR_X86_RUNTIME_CONF_HAVE_NO_SSE2__)
	ATHR_NO_SSE2_MEMORY_BARRIER__;
    else
#endif
	__asm__ __volatile__ ("mfence\n\t" : : : "memory");
}

static __inline__ void
athr_sfence__(void)
{
#if ATHR_SIZEOF_PTR == 4
    if (ATHR_X86_RUNTIME_CONF_HAVE_NO_SSE2__)
	ATHR_NO_SSE2_MEMORY_BARRIER__;
    else
#endif
	__asm__ __volatile__ ("sfence\n\t" : : : "memory");
}

static __inline__ void
athr_lfence__(void)
{
#if ATHR_SIZEOF_PTR == 4
    if (ATHR_X86_RUNTIME_CONF_HAVE_NO_SSE2__)
	ATHR_NO_SSE2_MEMORY_BARRIER__;
    else
#endif
	__asm__ __volatile__ ("lfence\n\t" : : : "memory");
}

#define ATHR_X86_OUT_OF_ORDER_MEMBAR(B)				\
  ATHR_CHOOSE_EXPR((B) == ATHR_StoreStore,			\
		   athr_sfence__(),				\
		   ATHR_CHOOSE_EXPR((B) == ATHR_LoadLoad,	\
				    athr_lfence__(),		\
				    athr_mfence__()))

#ifdef ATHR_X86_OUT_OF_ORDER

#define ATHR_MEMBAR(B) \
  ATHR_X86_OUT_OF_ORDER_MEMBAR((B))

#else /* !ATHR_X86_OUT_OF_ORDER (the default) */

/*
 * We assume that only stores before loads may be reordered. That is,
 * we assume that *no* instructions like these are used:
 * - CLFLUSH,
 * - streaming stores executed with non-temporal move,
 * - string operations, or
 * - other instructions which aren't LoadLoad, LoadStore, and StoreStore
 *   ordered by themselves
 * If such instructions are used, either insert memory barriers
 * using ATHR_X86_OUT_OF_ORDER_MEMBAR() at appropriate places, or
 * define ATHR_X86_OUT_OF_ORDER. For more info see Intel 64 and IA-32
 * Architectures Software Developer's Manual; Vol 3A; Chapter 8.2.2.
 */

#define ATHR_MEMBAR(B) \
  ATHR_CHOOSE_EXPR((B) & ATHR_StoreLoad, athr_mfence__(), athr_cfence__())

#endif /* !ATHR_X86_OUT_OF_ORDER */

#endif /* ATHR_X86_MEMBAR_H__ */
