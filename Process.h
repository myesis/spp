#ifndef __THREAD_H__
#define __THREAD_H__

typedef struct process Process;

/* process priorities */
#define PRIORITY_MAX          0
#define PRIORITY_HIGH         1
#define PRIORITY_NORMAL       2
#define PRIORITY_LOW          3
#define ERTS_NO_PROC_PRIO_LEVELS      4
#define ERTS_NO_PROC_PRIO_QUEUES      3

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

struct process {
#ifdef USE_UCONTEXT
    ucontext_t context;             /* save process context for switching */
#endif
    Uint tid;
    Uint status;
    Uint heap_size;
    Uint stack_size;

    void *args;

    int (*start_routine)(void *);

    void *htop;
    void *stop;

    Uint32 rcount;
    int schedule_count;

    Process *next;

    PPLMessageQueue msg;

    aviary_proc_lock_t lock;
    AviarySchedulerData *scheduler_data;
    aviary_atomic_t run_queue;
};

typedef struct {
    AviaryAtomicSchedTime last;
    struct {
        Uint64 short_interval;
        Uint64 long_interval;
    } worktime;
    int is_working;
} AviaryRunQueueSchedUtil;

struct AviarySchedulerData_ {
#ifdef USE_UCONTEXT
    ucontext_t context;             /* save process context for switching */
#endif
    Uint no;                     /* Scheudler number */
    Process *current_process;
    AviaryRunQueue *run_queue;
    int cpu_id;
};

struct AviaryRunQueue_ {
    int idx;
    int waiting;
    int woken;
    AviaryAtomic32_t flags;
    AviarySchedulerData *scheduler;
    struct {
        AviaryProcList *pending_exiters;
        Uint context_switches;
        Uint reductions;

        AviaryRunPrioQueue prio[AVIARY_NO_PROC_PRIO_LEVELS-1];
    } procs;

    AviaryRunQueueSchedUtil sched_util;
};

#endif