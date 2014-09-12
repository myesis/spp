#include <stdint.h>

#include "aviary_threads.h"
#include "aviary_process.h"
//#include "aviary_process_lock.h"
#include "aviary_alloc.h"

#define MAX_BIT       (1 << PRIORITY_MAX)
#define HIGH_BIT      (1 << PRIORITY_HIGH)
#define NORMAL_BIT    (1 << PRIORITY_NORMAL)
#define LOW_BIT       (1 << PRIORITY_LOW)

#define RUNQ_SET_RQ(X, RQ) aviary_smp_atomic_set_nob((X), (aviary_aint_t) (RQ))


Uint aviary_no_schedulers;
Uint aviary_no_run_queues;
AviaryAlignedRunQueue *aviary_aligned_run_queues;
AviaryAlignedSchedulerData *aviary_aligned_scheduler_data;

static aviary_tsd_key_t sched_data_key;

Uint aviary_default_process_flags;

typedef union {
    AviarySchedulerSleepInfo ssi;
    char align[AVIARY_ALC_CACHE_LINE_ALIGN_SIZE(sizeof(AviarySchedulerSleepInfo))];
} AviaryAlignedSchedulerSleepInfo;

static struct {
    aviary_smp_mtx_t update_mtx;
    aviary_smp_atomic32_t no_runqs;
    int last_active_runqs;
    int forced_check_balance;
    aviary_smp_atomic32_t checking_balance;
    int halftime;
    int full_reds_history_index;
    struct {
        int active_runqs;
        int reds;
        aviary_aint32_t max_len;
    } prev_rise;
    Uint n;
} balance_info;


AviaryAlignedSchedulerSleepInfo *aligned_sched_sleep_info;

AviarySchedulerData *aviary_scheduler_data;

static aviary_smp_atomic32_t no_empty_run_queues;

#define AVIARY_SCHED_SLEEP_INFO_IDX(IDX)                                    \
    (ASSERT(-1 <= ((int) (IDX))                                          \
                 && ((int) (IDX)) < ((int) aviary_no_schedulers)),         \
     &aligned_sched_sleep_info[(IDX)].ssi)

AviarySchedulerData *
aviary_get_scheduler_data(void) {
    return (AviarySchedulerData *) aviary_tsd_get(sched_data_key);
}

static inline void
aviary_smp_inc_runq_len(AviaryRunQueue *rq, AviaryRunQueueInfo *rqi, int prio)
{
    aviary_aint32_t len;

    len = aviary_smp_atomic32_read_nob(&rqi->len);

    if (len == 0) {
        aviary_smp_atomic32_read_bor_nob(&rq->flags,
            (aviary_aint32_t) (1 << prio));
    }
    len++;

    if (rqi->max_len < len) {
        rqi->max_len = len;
    }

    aviary_smp_atomic32_set_relb(&rqi->len, len);

    if (rq->len == 0) {
        //        aviary_non_empty_runq(rq);
    }

    rq->len++;
    if (rq->max_len < rq->len) {
        rq->max_len = len;
    }
}

static inline void
enqueue_process(AviaryRunQueue *runq, int prio, Process *p)
{
    AviaryRunPrioQueue *rpq;

    aviary_smp_inc_runq_len(runq, &runq->procs.prio_info[prio], prio);

    if (prio == PRIORITY_LOW) {
        p->schedule_count = RESCHEDULE_LOW;
        rpq = &runq->procs.prio[PRIORITY_NORMAL];
    }
    else {
        p->schedule_count = 1;
        rpq = &runq->procs.prio[prio];
    }

    p->next = NULL;
    if (rpq->last)
        rpq->last->next = p;
    else
        rpq->first = p;
    rpq->last = p;
}

static inline void
unqueue_process(
    AviaryRunQueue *runq,
    AviaryRunPrioQueue *rpq,
    AviaryRunQueueInfo *rqi,
    int prio,
    Process *prev_proc,
    Process *proc) {
    if (prev_proc)
        prev_proc->next = proc->next;
    else
        rpq->first = proc->next;
    if (!proc->next)
        rpq->last = prev_proc;

    if (!rpq->first)
        rpq->last = NULL;

    //    aviary_smp_dec_runq_len(runq, rqi, prio);
}

static inline Process *
dequeue_process(AviaryRunQueue *runq, int prio_q, aviary_aint32_t *statep) {
    aviary_aint32_t state;
    int prio;
    AviaryRunPrioQueue *rpq;
    AviaryRunQueueInfo *rqi;
    Process *p;

    rpq = &runq->procs.prio[prio_q];
    p = rpq->first;
    if (!p) {
        return NULL;
    }

    ATHR_COMPILER_BARRIER;

    state = aviary_smp_atomic32_read_nob(&p->state);
    if (statep) {
        *statep = state;
    }

    prio = (int) AVIARY_PSFLGS_GET_PRQ_PRIO(state);

    rqi = &runq->procs.prio_info[prio];

    if (p) {
        unqueue_process(runq, rpq, rqi, prio, NULL, p);
    }

    return p;
}

static inline int
check_requeue_process(AviaryRunQueue *rq, int prio_q) {
    AviaryRunPrioQueue *rpq = &rq->procs.prio[prio_q];
    Process *p = rpq->first;
    if (--p->schedule_count > 0 && p != rpq->last) {
        rpq->first = p->next;
        rpq->last->next = p;
        rpq->last = p;
        p->next = NULL;
        return 1;
    }
    return 0;
}
static void
init_scheduler_data(AviarySchedulerData* esdp, int num,
                    AviarySchedulerSleepInfo* ssi,
                    AviaryRunQueue* runq,
                    char** daww_ptr, size_t daww_sz)
{
    //    aviary_bits_init_state(&esdp->erl_bits_state);
    //    esdp->match_pseudo_process = NULL;
    esdp->free_process = NULL;


    esdp->no = (Uint) num;
    esdp->ssi = ssi;
    esdp->current_process = NULL;
    //    esdp->current_port = NULL;

    //    esdp->virtual_reds = 0;
    esdp->cpu_id = -1;

    //    aviary_init_atom_cache_map(&esdp->atom_cache_map);

    esdp->run_queue = runq;
    esdp->run_queue->scheduler = esdp;

#if 0
    if (daww_ptr) {
        init_aux_work_data(&esdp->aux_work_data, esdp, *daww_ptr);
        *daww_ptr += daww_sz;
    }
#endif

    esdp->reductions = 0;

    //    init_sched_wall_time(&esdp->sched_wall_time);
    //    aviary_port_task_handle_init(&esdp->nosuspend_port_task_handle);
}

void
aviary_init_scheduling(
    int no_schedulers,
    int no_schedulers_online) {
    int n, idx, pidx;
    size_t size_runqs;

    n = no_schedulers;
    size_runqs = sizeof(AviaryAlignedRunQueue) * n;
    aviary_aligned_run_queues = aviary_alloc_permanent_cache_aligned(
        size_runqs);

    aviary_no_run_queues = n;

    for (idx = 0; idx < n; idx++) {
        AviaryRunQueue *rq = AVIARY_RUNQ_IDX(idx);
        rq->idx = idx;
        rq->waiting = 0;
        rq->woken = 0;
        AVIARY_RUNQ_FLGS_INIT(rq, AVIARY_RUNQ_FLG_NONEMPTY);

        rq->procs.pending_exiters = NULL;
        rq->procs.context_switches = 0;
        rq->procs.reductions = 0;

        for (pidx = 0; pidx < AVIARY_NO_PROC_PRIO_LEVELS; pidx++) {
            rq->procs.prio_info[pidx].max_len = 0;
            rq->procs.prio_info[pidx].reds = 0;
            if (pidx < AVIARY_NO_PROC_PRIO_LEVELS - 1) {
                rq->procs.prio[pidx].first = NULL;
                rq->procs.prio[pidx].last = NULL;
            }
        }

        //        InitRunqueueSchedUtil(&rq->sched_util, aviary_sched_balance_util);
    }

    aviary_no_schedulers = n;

    aviary_aligned_scheduler_data =
        aviary_alloc_permanent_cache_aligned(n * sizeof(AviaryAlignedSchedulerData));

    for (idx = 0; idx < n; idx++) {
        AviarySchedulerData *adsp = AVIARY_SCHEDULER_IDX(idx);
        init_scheduler_data(adsp, idx+1, AVIARY_SCHED_SLEEP_INFO_IDX(idx),
            AVIARY_RUNQ_IDX(idx), NULL, 0);
    }

    //    init_misc_aux_work();

    //    aviary_port_task_init();

    //    aux_work_timeout_late_init();
}


void
ProcessMain(Uint32 low32, Uint32 hi32) {
    uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
    Process *p = (Process *)ptr;

    p->start_routine(p->args);

    /* TODO: Mark process flag to exit */
}

static int
try_steal_task_from_victim(AviaryRunQueue *rq, int *rq_lockedp, AviaryRunQueue *vrq, Uint32 flags)
{
    Uint32 procs_qmask = flags & AVIARY_RUNQ_FLGS_PROCS_QMASK;
    int max_prio_bit;
    AviaryRunPrioQueue *rpq;

    if (*rq_lockedp) {
        aviary_smp_runq_unlock(rq);
        *rq_lockedp = 0;
    }

    aviary_smp_runq_lock(vrq);

    while (procs_qmask) {
        Process *prev_proc;
        Process *proc;

        max_prio_bit = procs_qmask & -procs_qmask;
        switch (max_prio_bit) {
        case MAX_BIT:
            rpq = &vrq->procs.prio[PRIORITY_MAX];
            break;
        case HIGH_BIT:
            rpq = &vrq->procs.prio[PRIORITY_HIGH];
            break;
        case NORMAL_BIT:
        case LOW_BIT:
            rpq = &vrq->procs.prio[PRIORITY_NORMAL];
            break;
        case 0:
            goto no_procs;
        default:
            ASSERT(!"Invalid queue mask");
            goto no_procs;
        }

        prev_proc = NULL;
        proc = rpq->first;

        while (proc) {
            aviary_aint32_t state = aviary_smp_atomic32_read_acqb(&proc->state);
            if (!(AVIARY_PSFLG_BOUND & state)) {
                /* Steal process */
                int prio = (int) AVIARY_PSFLGS_GET_PRQ_PRIO(state);
                AviaryRunQueueInfo *rqi = &vrq->procs.prio_info[prio];
                unqueue_process(vrq, rpq, rqi, prio, prev_proc, proc);
                aviary_smp_runq_unlock(vrq);
                RUNQ_SET_RQ(&proc->run_queue, rq);

                aviary_smp_runq_lock(rq);
                *rq_lockedp = 1;
                enqueue_process(rq, prio, proc);
                return !0;
            }
            prev_proc = proc;
            proc = proc->next;
        }

        procs_qmask &= ~max_prio_bit;
    }

no_procs:
    aviary_smp_runq_unlock(vrq);

    return 0;
}

static inline int
check_possible_steal_victim(AviaryRunQueue *rq, int *rq_lockedp, int vix)
{
    AviaryRunQueue *vrq = AVIARY_RUNQ_IDX(vix);
    Uint32 flags = AVIARY_RUNQ_FLGS_GET(vrq);
    if ((flags & (AVIARY_RUNQ_FLG_NONEMPTY
                | AVIARY_RUNQ_FLG_PROTECTED)) == AVIARY_RUNQ_FLG_NONEMPTY) {
        return try_steal_task_from_victim(rq, rq_lockedp, vrq, flags);
    } else {
        return 0;
    }
}

static inline Uint32
empty_protected_runq(AviaryRunQueue *rq)
{
    Uint32 old_flags = AVIARY_RUNQ_FLGS_BSET(rq,
        AVIARY_RUNQ_FLG_NONEMPTY|AVIARY_RUNQ_FLG_PROTECTED,
        AVIARY_RUNQ_FLG_PROTECTED);
    return old_flags;
}

#define AVIARY_NO_USED_RUNQS_SHIFT 16
#define AVIARY_NO_RUNQS_MASK 0xffff

static inline void
get_no_runqs(int *active, int *used)
{
    aviary_aint32_t no_runqs = aviary_smp_atomic32_read_nob(&balance_info.no_runqs);
    if (active) {
        *active = (int) (no_runqs & AVIARY_NO_RUNQS_MASK);
    }
    if (used) {
        *used = (int) ((no_runqs >> AVIARY_NO_USED_RUNQS_SHIFT) & AVIARY_NO_RUNQS_MASK);
    }
}

static int
try_steal_task(AviaryRunQueue *rq)
{
    int res, rq_locked, vix, active_rqs, blnc_rqs;
    Uint32 flags;

    flags = empty_protected_runq(rq);
    if (flags & AVIARY_RUNQ_FLG_SUSPENDED) {
        return 0;
    }

    get_no_runqs(&active_rqs, &blnc_rqs);

    if (active_rqs > blnc_rqs) {
        active_rqs = blnc_rqs;
    }

    if (rq->idx < active_rqs) {
        if (active_rqs < blnc_rqs) {
            int no = blnc_rqs - active_rqs;
            int stop_idx = vix = active_rqs + rq->idx % no;
            while (aviary_smp_atomic32_read_acqb(&no_empty_run_queues) < blnc_rqs) {
                res = check_possible_steal_victim(rq, &rq_locked, vix);
                if (res) {
                    goto done;
                }
                vix++;
                if (vix >= blnc_rqs) {
                    vix = active_rqs;
                }
                if (vix == stop_idx) {
                    break;
                }
            }
        }

        vix = rq->idx;

        while (aviary_smp_atomic32_read_acqb(&no_empty_run_queues) < blnc_rqs) {
            vix++;
            if (vix >= active_rqs) {
                vix = 0;
            }
            if (vix == rq->idx) {
                break;
            }

            res = check_possible_steal_victim(rq, &rq_locked, vix);
            if (res) {
                goto done;
            }
        }
    }

done:
    if (!rq_locked) {
        aviary_smp_runq_lock(rq);
    }

    return res;
}

Process *
Schedule(Process *p, int calls) {
    AviarySchedulerData *asdp;
    AviaryRunQueue *rq;
    int context_reds;
    int fcalls;
    int actual_reds;
    int reds;
    int ready;
    Uint32 flags;
    aviary_aint32_t state = 0;

    if (!p) {
        asdp = aviary_get_scheduler_data();
        ASSERT(asdp);
        rq = aviary_get_runq_current(asdp);
        /* TODO
        fcalls = 0;
        */
        actual_reds = reds = 0;
        aviary_smp_runq_lock(rq);
    } else {
sched_out_proc:
        ready = 0;
        asdp = p->scheduler_data;
        ASSERT(asdp->current_process == p
            || asdp->free_process == p);
        asdp->current_process = NULL;
        p->scheduler_data = NULL;
    }

continue_check_activities_to_run:
    rq = aviary_get_runq_current(asdp);

    flags = AVIARY_RUNQ_FLGS_GET_NOB(rq);

    if ((!(flags & AVIARY_RUNQ_FLGS_QMASK))) {
        flags= AVIARY_RUNQ_FLGS_GET_NOB(rq);
        if (flags & AVIARY_RUNQ_FLG_SUSPENDED)
        ;
        if (flags & AVIARY_RUNQ_FLG_INACTIVE)
        ;//empty_runq(rq);
        while (1) {
            if (try_steal_task(rq)) {
                goto continue_check_activities_to_run;
            }
        }
    }
pick_next_process: {
    aviary_aint32_t psflg_band_mask;
    int prio_q;
    int qmask;
    flags = AVIARY_RUNQ_FLGS_GET_NOB(rq);
    qmask = (int) (flags & AVIARY_RUNQ_FLGS_PROCS_QMASK);
    switch(qmask & -qmask) {
        case MAX_BIT:
            prio_q = PRIORITY_MAX;
            break;
        case HIGH_BIT:
            prio_q = PRIORITY_HIGH;
            break;
        case NORMAL_BIT:
        case LOW_BIT:
            prio_q = PRIORITY_NORMAL;
            if (check_requeue_process(rq, PRIORITY_NORMAL)) {
                goto pick_next_process;
            }
            break;
        case 0:
        default:
            ASSERT(qmask == 0);
            /* TODO
               goto check_activities_to_run;
             */
    }

    //    AVIARY_START_TIMER(system);

    p = dequeue_process(rq, prio_q, &state);

    ASSERT(p);

    while (1) {
        aviary_aint32_t exp, new, tmp;
        tmp = new = exp = state;
        new &= psflg_band_mask;
        if (!(state & (AVIARY_PSFLG_RUNNING
                    | AVIARY_PSFLG_RUNNING_SYS))) {
            tmp = state & (AVIARY_PSFLG_SUSPENDED
                | AVIARY_PSFLG_PENDING_EXIT
                | AVIARY_PSFLG_ACTIVE_SYS);
            if (tmp != AVIARY_PSFLG_SUSPENDED) {
                if (state & AVIARY_PSFLG_ACTIVE_SYS)
                    new |= AVIARY_PSFLG_RUNNING_SYS;
                else
                    new |= AVIARY_PSFLG_RUNNING;
            }
        }
        state = aviary_smp_atomic32_cmpxchg_relb(&p->state, new, exp);
        if (state == exp) {
            if ((state & (AVIARY_PSFLG_RUNNING
                          | AVIARY_PSFLG_RUNNING_SYS
                          | AVIARY_PSFLG_FREE))
                || ((state & (AVIARY_PSFLG_SUSPENDED
                              | AVIARY_PSFLG_PENDING_EXIT
                              | AVIARY_PSFLG_ACTIVE_SYS))
                    == AVIARY_PSFLG_SUSPENDED)) {
                if (state & AVIARY_PSFLG_FREE) {
#ifdef AVIARY_SMP
                    aviary_smp_proc_dec_refc(p);
#else
                    //                    aviary_free_proc(p);
#endif
                }
#if 0
                if (proxy_p) {
                    free_proxy_proc(proxy_p);
                    proxy_p = NULL;
                }
#endif
                goto pick_next_process;
            }
            state = new;
            break;
        }
    }

    rq->procs.context_switches++;

    asdp->current_process = p;

    aviary_smp_runq_unlock(rq);


    p->scheduler_data = asdp;
    reds = context_reds;


    if (ready) {
        getcontext(&p->context);
        p->context.uc_link = &asdp->context;
        p->context.uc_stack.ss_sp = p->stack;
        p->context.uc_stack.ss_size = p->stack_size;

        uintptr_t ptr = p;
        makecontext(&p->context, (void (*) (void)) ProcessMain, 2,
            (Uint)ptr, (Uint)(ptr >> 32));
    }

    swapcontext(&asdp->context, &p->context);

    goto sched_out_proc;
    }
}

/* This call never returns. */
void
SchedulerMain(void) {
    Process *current = NULL;
    int reds_used = 0;

    current = Schedule(current, reds_used);
}

static void *
SchedThreadFunc(void *vasdp) {
    aviary_tsd_set(sched_data_key, vasdp);

    AviarySchedulerData *asdp = vasdp;
    Uint no = asdp->no;

    SchedulerMain();
}

static athr_tid aux_tid;

void
aviary_start_schedulers(void) {
    int res = 0;
    Uint actual;
    Uint wanted = aviary_no_schedulers;

    athr_thr_opts opts = ATHR_THR_OPTS_DEFAULT_INITER;

    opts.detached = 1;

    /* create runq_supervisor thread */


    /* create scheduler threads */
    for (actual = 0; actual < wanted; actual++) {
        AviarySchedulerData *asdp = AVIARY_SCHEDULER_IDX(actual);

        res = athr_thr_create(&asdp->tid, SchedThreadFunc, (void*)asdp, &opts);

        if (res != 0) {
            break;
        }
    }

    aviary_no_schedulers = actual;

    ATHR_COMPILER_BARRIER;

    /* create auxilairy thread */
}

void
aviary_pre_init_process(void)
{
    aviary_tsd_key_create(&sched_data_key, "aviary_sched_data_key");
}

#define AVIARY_ENQUEUE_NOT 0
#define AVIARY_ENQUEUE_NORMAL_QUEUE 1

static inline int
check_enqueue_in_prio_queue(Process *c_p,
			    aviary_aint32_t *prq_prio_p,
			    aviary_aint32_t *newp,
			    aviary_aint32_t actual)
{
    aviary_aint32_t aprio, qbit, max_qbit;

    aprio = (actual >> AVIARY_PSFLGS_ACT_PRIO_OFFSET) & AVIARY_PSFLGS_PRIO_MASK;
    qbit = 1 << aprio;

    *prq_prio_p = aprio;

    max_qbit = (actual >> AVIARY_PSFLGS_IN_PRQ_MASK_OFFSET) & AVIARY_PSFLGS_QMASK;
    max_qbit |= 1 << AVIARY_PSFLGS_QMASK_BITS;
    max_qbit &= -max_qbit;
    /*
     * max_qbit now either contain bit set for highest prio queue or a bit
     * out of range (which will have a value larger than valid range).
     */

    if (qbit >= max_qbit) {
        return AVIARY_ENQUEUE_NOT; /* Already queued in higher or equal prio */
    }

    /* Need to enqueue (if already enqueued, it is in lower prio) */
    *newp |= qbit << AVIARY_PSFLGS_IN_PRQ_MASK_OFFSET;

    if ((actual & (AVIARY_PSFLG_IN_RUNQ|AVIARY_PSFLGS_USR_PRIO_MASK))
	!= (aprio << AVIARY_PSFLGS_USR_PRIO_OFFSET)) {
	/*
	 * Process struct already enqueued, or actual prio not
	 * equal to user prio, i.e., enqueue using proxy.
	 */
        return -AVIARY_ENQUEUE_NORMAL_QUEUE;
    }

    /*
     * Enqueue using process struct.
     */
    *newp &= ~AVIARY_PSFLGS_PRQ_PRIO_MASK;
    *newp |= AVIARY_PSFLG_IN_RUNQ | (aprio << AVIARY_PSFLGS_PRQ_PRIO_OFFSET);
    return AVIARY_ENQUEUE_NORMAL_QUEUE;
}


static inline int
change_proc_schedule_state(
    Process *p,
    aviary_aint32_t clear_state_flags,
    aviary_aint32_t set_state_flags,
    aviary_aint32_t *statep,
    aviary_aint32_t *enq_prio_p)
{
    aviary_aint32_t a = *statep, n;
    int enqueue;

    while (1) {
        aviary_aint32_t e;
        n = e = a;
        enqueue = AVIARY_ENQUEUE_NOT;

        if (a & AVIARY_PSFLG_FREE) {
            break;
        }

        if (clear_state_flags) {
            n &= ~clear_state_flags;
        }

        if (set_state_flags) {
            n |= set_state_flags;
        }

        if ((n & (AVIARY_PSFLG_SUSPENDED
                  | AVIARY_PSFLG_RUNNING
                  | AVIARY_PSFLG_RUNNING_SYS
                  | AVIARY_PSFLG_IN_RUNQ
                    | AVIARY_PSFLG_ACTIVE)) == AVIARY_PSFLG_ACTIVE) {
            /*
             * Active and seemingly need to be enqueued, but
             * process may be in a run queue via proxy, need
             * further inspection...
             */
            enqueue = check_enqueue_in_prio_queue(p, enq_prio_p, &n, a);
        }

        a = aviary_smp_atomic32_cmpxchg_mb(&p->state, n, e);
        if (a == e) {
            break;
        }
        if (enqueue == AVIARY_ENQUEUE_NOT && n == a) {
            break;
        }
    }

    *statep = a;

    return enqueue;
}

static inline void
schedule_process(Process *p, aviary_aint32_t in_state)
{
    aviary_aint32_t enq_prio = -1;
    aviary_aint32_t state = in_state;
    int enqueue = change_proc_schedule_state(
        p,
        0,
        AVIARY_PSFLG_ACTIVE,
        &state,
        &enq_prio);

    if (enqueue != AVIARY_ENQUEUE_NOT) {
        assert(0);
    }
}

static Process *
alloc_process(AviaryRunQueue *rq, aviary_aint32_t state)
{
    Process *p;

    p = aviary_alloc(sizeof(Process));
    if (!p) {
        return NULL;
    }

    p->rcount = 0;
    return p;
}

int
aviary_create_process(
    Process* parent,
    void *(start_routine)(void *),
    void *args,
    AviarySpawnOpts *so)
{
    AviaryRunQueue *rq = NULL;
    Process *p;
    Uint arg_size;
    Uint sz;
    int res;

    aviary_aint32_t state = 0;
    aviary_aint32_t prio = (aviary_aint32_t) PRIORITY_NORMAL;

    aviary_smp_proc_lock(parent, AVIARY_PROC_LOCKS_ALL_MINOR);


    state |= (((prio & AVIARY_PSFLGS_PRIO_MASK) << AVIARY_PSFLGS_ACT_PRIO_OFFSET)
        | ((prio & AVIARY_PSFLGS_PRIO_MASK) << AVIARY_PSFLGS_USR_PRIO_OFFSET));

    if (!rq) {
        rq = aviary_get_runq_proc(parent);
    }

    p = alloc_process(rq, state);

    if (!p) {
        goto error;
    }

    p->flags = aviary_default_process_flags;
    p->schedule_count = 0;
    p->rcount = 0;
    p->stack = aviary_mmap(sz);
    p->stack_size = sz;
    p->stop = p->stack + sz - 1024;
    p->parent = NULL;
    p->scheduler_data = NULL;
    //    p->current = NULL;


    p->next = NULL;

    aviary_proc_lock_init(p);
    aviary_smp_proc_unlock(p, AVIARY_PROC_LOCKS_ALL);
    //    RUNQ_SET_RQ(&p->run_queue, AVIARY_RUNQ_IDX(0));

    schedule_process(p, state);

error:

    aviary_smp_proc_unlock(parent, AVIARY_PROC_LOCKS_ALL_MINOR);

    return res;
}