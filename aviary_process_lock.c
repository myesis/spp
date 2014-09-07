#include "aviary_process.h"
void
aviary_proc_lock_failed(Process *p,
                      aviary_pix_lock_t *pixlck,
                      AviaryProcLocks locks,
                      AviaryProcLocks old_lflgs)
{
    assert(0);
}

void
aviary_proc_lock_init(Process *p)
{
    int i;
    aviary_smp_atomic32_init_nob(
        &p->lock.flags,
        (aviary_aint32_t) AVIARY_PROC_LOCKS_ALL);

    aviary_atomic32_init_nob(&p->lock.refc, 1);
}