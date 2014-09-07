#ifndef __PROCESS_H__
#define __PROCESS_H__

typedef struct process Process;

#include "sys.h"
#ifndef AVIARY_PROCESS_LOCK_DEFINE_MACRO
#define AVIARY_PROCESS_LOCK_DEFINE_MACRO
#include "aviary_process_lock.h"
#undef AVIARY_PROCESS_LOCK_DEFINE_MACRO
#endif

#include "athread.h"
#include "aviary_smp.h"
#include "aviary_atomic.h"
#include "aviary_alloc.h"

#define AVIARY_DEFAULT_MAX_PROCESSES (1 << 18)

/* process priorities */
#define PRIORITY_MAX          0
#define PRIORITY_HIGH         1
#define PRIORITY_NORMAL       2
#define PRIORITY_LOW          3
#define AVIARY_NO_PROC_PRIO_LEVELS      4
#define AVIARY_NO_PROC_PRIO_QUEUES      3

#define AVIARY_PORT_PRIO_LEVEL AVIARY_NO_PROC_PRIO_LEVELS
#define AVIARY_NO_PRIO_LEVELS (AVIARY_NO_PROC_PRIO_LEVELS + 1)

#define AVIARY_RUNQ_FLGS_PROCS_QMASK \
  ((((Uint32) 1) << AVIARY_NO_PROC_PRIO_LEVELS) - 1)

#define AVIARY_RUNQ_FLGS_QMASK \
  ((((Uint32) 1) << AVIARY_NO_PRIO_LEVELS) - 1)

#define RESCHEDULE_LOW       8

#define AVIARY_PSFLG_IN_PRQ_MAX           AVIARY_PSFLG_BIT(0)
#define AVIARY_PSFLG_IN_PRQ_HIGH          AVIARY_PSFLG_BIT(1)
#define AVIARY_PSFLG_IN_PRQ_NORMAL        AVIARY_PSFLG_BIT(2)
#define AVIARY_PSFLG_IN_PRQ_LOW           AVIARY_PSFLG_BIT(3)
#define AVIARY_PSFLG_FREE                 AVIARY_PSFLG_BIT(4)
#define AVIARY_PSFLG_EXITING              AVIARY_PSFLG_BIT(5)
#define AVIARY_PSFLG_PENDING_EXIT         AVIARY_PSFLG_BIT(6)
#define AVIARY_PSFLG_ACTIVE               AVIARY_PSFLG_BIT(7)
#define AVIARY_PSFLG_IN_RUNQ              AVIARY_PSFLG_BIT(8)
#define AVIARY_PSFLG_RUNNING              AVIARY_PSFLG_BIT(9)
#define AVIARY_PSFLG_SUSPENDED            AVIARY_PSFLG_BIT(10)
#define AVIARY_PSFLG_GC                   AVIARY_PSFLG_BIT(11)
#define AVIARY_PSFLG_BOUND                AVIARY_PSFLG_BIT(12)
#define AVIARY_PSFLG_TRAP_EXIT            AVIARY_PSFLG_BIT(13)
#define AVIARY_PSFLG_ACTIVE_SYS           AVIARY_PSFLG_BIT(14)
#define AVIARY_PSFLG_RUNNING_SYS          AVIARY_PSFLG_BIT(15)
#define AVIARY_PSFLG_PROXY                AVIARY_PSFLG_BIT(16)
#define AVIARY_PSFLG_DELAYED_SYS          AVIARY_PSFLG_BIT(17)

#define AVIARY_PSFLGS_PRIO_BITS 2
#define AVIARY_PSFLGS_PRIO_MASK \
    ((((aviary_aint32_t) 1) << AVIARY_PSFLGS_PRIO_BITS) - 1)

#define AVIARY_PSFLGS_ACT_PRIO_OFFSET (0*AVIARY_PSFLGS_PRIO_BITS)
#define AVIARY_PSFLGS_USR_PRIO_OFFSET (1*AVIARY_PSFLGS_PRIO_BITS)
#define AVIARY_PSFLGS_PRQ_PRIO_OFFSET (2*AVIARY_PSFLGS_PRIO_BITS)
#define AVIARY_PSFLGS_ZERO_BIT_OFFSET (3*AVIARY_PSFLGS_PRIO_BITS)

#define AVIARY_PSFLGS_QMASK_BITS 4
#define AVIARY_PSFLGS_QMASK \
    ((((aviary_aint32_t) 1) << AVIARY_PSFLGS_QMASK_BITS) - 1)
#define AVIARY_PSFLGS_IN_PRQ_MASK_OFFSET \
    AVIARY_PSFLGS_ZERO_BIT_OFFSET

#define AVIARY_PSFLG_BIT(N) \
    (((aviary_aint32_t) 1) << (AVIARY_PSFLGS_ZERO_BIT_OFFSET + (N)))

#define AVIARY_RUNQ_IDX(IDX)                                                \
  (ASSERT(0 <= (IDX) && (IDX) < aviary_no_run_queues),                      \
   &aviary_aligned_run_queues[(IDX)].runq)

#define AVIARY_PSFLG_BIT(N) \
    (((aviary_aint32_t) 1) << (AVIARY_PSFLGS_ZERO_BIT_OFFSET + (N)))

#define AVIARY_PSFLGS_GET_PRQ_PRIO(PSFLGS) \
    (((PSFLGS) >> AVIARY_PSFLGS_USR_PRIO_OFFSET) & AVIARY_PSFLGS_PRIO_MASK)

#define AVIARY_RUNQ_FLGS_GET_NOB(RQ)                                      \
    ((Uint32) aviary_smp_atomic32_read_nob(&(RQ)->flags))

#define AVIARY_PSFLGS_ACT_PRIO_MASK \
    (AVIARY_PSFLGS_PRIO_MASK << AVIARY_PSFLGS_ACT_PRIO_OFFSET)
#define AVIARY_PSFLGS_USR_PRIO_MASK \
    (AVIARY_PSFLGS_PRIO_MASK << AVIARY_PSFLGS_USR_PRIO_OFFSET)
#define AVIARY_PSFLGS_PRQ_PRIO_MASK \
    (AVIARY_PSFLGS_PRIO_MASK << AVIARY_PSFLGS_PRQ_PRIO_OFFSET)



typedef struct AviarySchedulerData_ AviarySchedulerData;
typedef struct AviaryRunQueue_ AviaryRunQueue;
typedef struct AviarySchedulerSleepInfo_ AviarySchedulerSleepInfo;

extern Uint aviary_no_schedulers;

extern AviarySchedulerData *aviary_scheduler_data;

typedef struct {
    Uint flags;
    int error_code;             /* Error code returned from create_process(). */

    /*
     * The following items are only initialized if the SPO_USE_ARGS flag is set.
     */
    Uint min_heap_size;         /* Minimum heap size (must be a valued returned
                                 * from next_heap_size()).  */
    Uint min_vheap_size;        /* Minimum virtual heap size  */
    int priority;               /* Priority for process. */
    Uint16 max_gen_gcs;         /* Maximum number of gen GCs before fullsweep. */
    int scheduler;
} AviarySpawnOpts;

typedef struct AviaryProcList_ AviaryProcList;
struct AviaryProcList_ {
    int pid;
    Uint64 started_interval;
    AviaryProcList *next;
    AviaryProcList *prev;
};

typedef struct {
    Process *first;
    Process *last;
} AviaryRunPrioQueue;

typedef struct {
    aviary_smp_atomic32_t len;
    aviary_aint32_t max_len;
    int reds;
} AviaryRunQueueInfo;


typedef struct AviarySchedulerData_ AviarySchedulerData;

typedef struct AviaryRunQueue_ AviaryRunQueue;

#include <ucontext.h>
struct process {
    ucontext_t context;             /* save process context for switching */
    Uint tid;
    Uint status;
    Uint heap_size;
    Uint stack_size;
    Uint flags;

    void *args;

    int (*start_routine)(void *);

    void *stack;
    void *htop;
    void *stop;

    aviary_smp_atomic32_t state;

    Uint32 rcount;
    int schedule_count;

    aviary_proc_lock_t lock;
    Process *next;

    Process *parent;

    //    PPLMessageQueue msg;

    AviarySchedulerData *scheduler_data;
    aviary_atomic_t run_queue;
};

typedef struct {
    //    AviaryAtomicSchedTime last;
    struct {
        Uint64 short_interval;
        Uint64 long_interval;
    } worktime;
    int is_working;
} AviaryRunQueueSchedUtil;

struct AviarySchedulerData_ {
    ucontext_t context;             /* save process context for switching */
    athr_tid tid;
    Uint no;                     /* Scheudler number */
    Process *current_process;
    Process *free_process;
    AviaryRunQueue *run_queue;
    AviarySchedulerSleepInfo *ssi;
    int cpu_id;
    Uint64 reductions;
};

struct AviaryRunQueue_ {
    int idx;

    aviary_smp_mtx_t mutex;
    aviary_smp_cnd_t cond;

    int waiting;
    int woken;
    aviary_atomic32_t flags;
    AviarySchedulerData *scheduler;
    struct {
        AviaryProcList *pending_exiters;
        Uint context_switches;
        Uint reductions;

        AviaryRunQueueInfo prio_info[AVIARY_NO_PROC_PRIO_LEVELS];
        AviaryRunPrioQueue prio[AVIARY_NO_PROC_PRIO_LEVELS-1];
    } procs;

    AviaryRunQueueSchedUtil sched_util;
};

struct AviarySchedulerSleepInfo_ {
    AviarySchedulerSleepInfo *next;
    AviarySchedulerSleepInfo *prev;
    aviary_smp_atomic32_t flags;
    //    aviary_tse_t *event;
    aviary_atomic32_t aux_work;
};

typedef union {
    AviaryRunQueue runq;
    char align[AVIARY_ALC_CACHE_LINE_ALIGN_SIZE(sizeof(AviaryRunQueue))];
} AviaryAlignedRunQueue;

extern AviaryAlignedRunQueue *aviary_aligned_run_queues;

typedef union {
    AviarySchedulerData esd;
    char align[AVIARY_ALC_CACHE_LINE_ALIGN_SIZE(sizeof(AviarySchedulerData))];
} AviaryAlignedSchedulerData;


static inline AviaryRunQueue *
aviary_get_runq_current(AviarySchedulerData *asdp) {
    //    ASSERT(!asdp);
    return asdp->run_queue;
}

static inline void
aviary_smp_runq_lock(AviaryRunQueue *rq) {
    aviary_smp_mtx_lock(&rq->mutex);
}

static inline void
aviary_smp_runq_unlock(AviaryRunQueue *rq) {
    aviary_smp_mtx_unlock(&rq->mutex);
}

static inline AviaryRunQueue *
aviary_get_runq_proc(Process *p)
{
    return (AviaryRunQueue *) aviary_atomic_read_nob(&p->run_queue);
}
#ifndef AVIARY_PROCESS_LOCK_DEFINE_INLINE
#define AVIARY_PROCESS_LOCK_DEFINE_INLINE
#include "aviary_process_lock.h"
#undef AVIARY_PROCESS_LOCK_DEFINE_INLINE
#endif

#endif