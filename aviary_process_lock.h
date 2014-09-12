#ifndef AVIARY_PROCESS_LOCK_H__
#define AVIARY_PROCESS_LOCK_H__

#include "aviary_smp.h"
#include "athr_atomics.h"

//#ifdef AVIARY_PROCESS_LOCK_DEFINE_MACRO
#define AVIARY_PROC_LOCK_MAX_BIT 3

typedef aviary_aint32_t AviaryProcLocks;

typedef struct aviary_proc_lock_t_ {
    athr_atomic32_t flags;
    athr_atomic32_t refc;
} aviary_proc_lock_t;

#include "aviary_alloc.h"
typedef struct {
    union {
        aviary_mtx_t mtx;
        char buf[AVIARY_ALC_CACHE_LINE_ALIGN_SIZE(sizeof(aviary_mtx_t))];
    } u;
} aviary_pix_lock_t;

/* Process lock flags */

/*
 * Main lock:
 *   The main lock is held by the scheduler running a process. It
 *   is used to protect all fields in the process structure except
 *   for those fields protected by other process locks (follows).
 */
#define AVIARY_PROC_LOCK_MAIN             (((AviaryProcLocks) 1) << 0)

/*
 * Link lock:
 *   Protects the following fields in the process structure:
 *   * nlinks
 *   * monitors
 *   * suspend_monitors
 */
#define AVIARY_PROC_LOCK_LINK             (((AviaryProcLocks) 1) << 1)

/*
 * Message queue lock:
 *   Protects the following fields in the process structure:
 *   * msg_inq
 *   * bif_timers
 */
#define AVIARY_PROC_LOCK_MSGQ             (((AviaryProcLocks) 1) << 2)

/*
 * Status lock:
 *   Protects the following fields in the process structure:
 *   * pending_suspenders
 *   * suspendee
 *   * ...
 */
#define AVIARY_PROC_LOCK_STATUS           (((AviaryProcLocks) 1) << AVIARY_PROC_LOCK_MAX_BIT)


#define AVIARY_PROC_LOCK_WAITER_SHIFT (AVIARY_PROC_LOCK_MAX_BIT + 1)

#define AVIARY_PROC_LOCKS_ALL \
  ((((AviaryProcLocks) 1) << (AVIARY_PROC_LOCK_MAX_BIT + 1)) - 1)

#define AVIARY_PROC_LOCKS_ALL_MINOR       (AVIARY_PROC_LOCKS_ALL \
                                         & ~AVIARY_PROC_LOCK_MAIN)


#define AVIARY_PROC_LOCK_FLGS_BAND_(L, MSK) \
  ((AviaryProcLocks) aviary_smp_atomic32_read_band_nob(&(L)->flags, \
                                                   (aviary_aint32_t) (MSK)))
#define AVIARY_PROC_LOCK_FLGS_BOR_ACQB_(L, MSK) \
  ((AviaryProcLocks) aviary_smp_atomic32_read_bor_acqb(&(L)->flags, \
                                                   (aviary_aint32_t) (MSK)))
#define AVIARY_PROC_LOCK_FLGS_CMPXCHG_ACQB_(L, NEW, EXPECTED) \
  ((AviaryProcLocks) aviary_smp_atomic32_cmpxchg_acqb(&(L)->flags, \
                                                  (aviary_aint32_t) (NEW), \
                                                  (aviary_aint32_t) (EXPECTED)))
#define AVIARY_PROC_LOCK_FLGS_CMPXCHG_RELB_(L, NEW, EXPECTED) \
  ((AviaryProcLocks) aviary_smp_atomic32_cmpxchg_relb(&(L)->flags, \
                                                  (aviary_aint32_t) (NEW), \
                                                  (aviary_aint32_t) (EXPECTED)))
#define AVIARY_PROC_LOCK_FLGS_READ_(L) \
  ((AviaryProcLocks) aviary_smp_atomic32_read_nob(&(L)->flags))

//#endif



#endif

/*
 * Helper function for aviary_smp_proc_lock__ and aviary_smp_proc_trylock__.
 *
 * Attempts to grab all of 'locks' simultaneously.
 *
 * On success, returns zero.
 *
 * On failure, returns the p->locks at the moment it tried to grab them,
 * at least some of which will intersect with 'locks', so it is nonzero.
 *
 * This assumes p's pix lock is held on entry if !AVIARY_PROC_LOCK_ATOMIC_IMPL.
 * Does not release the pix lock.
 */
#ifdef AVIARY_PROCESS_LOCK_DEFINE_INLINE
static inline AviaryProcLocks
aviary_smp_proc_raw_trylock__(Process *p, AviaryProcLocks locks)
{
    AviaryProcLocks expct_lflgs = 0;

    while (1) {
        AviaryProcLocks lflgs = AVIARY_PROC_LOCK_FLGS_CMPXCHG_ACQB_(&p->lock,
								expct_lflgs | locks,
								expct_lflgs);
        //        if (AVIARY_LIKELY(lflgs == expct_lflgs)) {
        if ((lflgs == expct_lflgs)) {
            /* We successfully grabbed all locks. */
            return 0;
        }

        if (lflgs & locks) {
            /* Some locks we need are locked, give up. */
            return lflgs;
        }

        /* cmpxchg failed, try again (should be rare). */
        expct_lflgs = lflgs;
    }
}

static inline void
aviary_smp_proc_lock__(
    Process *p,
    aviary_pix_lock_t *pix_lck,
    AviaryProcLocks locks)
{
    AviaryProcLocks old_lflgs;

    old_lflgs = aviary_smp_proc_raw_trylock__(p, locks);

    if (old_lflgs != 0) {
        /*
         * There is lock contention, so let aviary_proc_lock_failed() deal
         * with it. Note that aviary_proc_lock_failed() returns with
         * pix_lck unlocked.
         */
        aviary_proc_lock_failed(p, pix_lck, locks, old_lflgs);
    }

    ATHR_COMPILER_BARRIER;
}

static inline void
aviary_smp_proc_unlock__(Process *p,
    aviary_pix_lock_t *pix_lck,
    AviaryProcLocks locks)
{
    AviaryProcLocks old_lflgs;
    ATHR_COMPILER_BARRIER;

    old_lflgs = AVIARY_PROC_LOCK_FLGS_READ_(&p->lock);

    while (1) {
        /*
         * We'll atomically unlock every lock that has no waiter.
         * If any locks with waiters remain we'll let
         * aviary_proc_unlock_failed() deal with them.
         */
        AviaryProcLocks wait_locks =
            (old_lflgs >> AVIARY_PROC_LOCK_WAITER_SHIFT) & locks;

        /* What p->lock will look like with all non-waited locks released. */
        AviaryProcLocks want_lflgs = old_lflgs & (wait_locks | ~locks);

        if (want_lflgs != old_lflgs) {
            AviaryProcLocks new_lflgs =
                AVIARY_PROC_LOCK_FLGS_CMPXCHG_RELB_(&p->lock, want_lflgs, old_lflgs);

            if (new_lflgs != old_lflgs) {
                /* cmpxchg failed, try again. */
                old_lflgs = new_lflgs;
                continue;
            }
        }

        /* We have successfully unlocked every lock with no waiter. */

        if (want_lflgs & locks) {
            /* Locks with waiters remain. */
            /* aviary_proc_unlock_failed() returns with pix_lck unlocked. */
            assert(0);
#if 0
            aviary_proc_unlock_failed(p, pix_lck, want_lflgs & locks);
#endif
        }
        else {
#if 0
            aviary_pix_unlock(pix_lck);
#endif
        }

        break;
    }

}

static inline void
aviary_smp_proc_lock(Process *p, AviaryProcLocks locks)
{
    aviary_smp_proc_lock__(p, NULL, locks);
}

static inline void
aviary_smp_proc_unlock(Process *p, AviaryProcLocks locks)
{
    aviary_smp_proc_unlock__(p, NULL, locks);
}

#endif
