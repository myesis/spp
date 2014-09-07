#include <Python.h>
#include <assert.h>
#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <ucontext.h>

#define SCHEDULE_PYTHON 1
#define MMAP_ALLOC 1
#define DEFAULT_STACK_SIZE 1024 * 1024

typedef int (*fthread_func_t)(void *);
typedef int fthread_t;
struct list_head {
    struct list_head *next, *prev;
};

typedef struct _fthread_s
{
    fthread_t id;
    void * stack;
    fthread_func_t thread;
    void *args;
    int status;
    struct list_head list;
    ucontext_t ctx;
    PyThreadState *tstate;
} fthread_s;

enum {
    READY = 1,
    RUNNING = 2,
    SUSPEND = 3,
    DEAD = 4,
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *_new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = _new;
	_new->next = next;
	_new->prev = prev;
	prev->next = _new;
}
/**
 * list_add - add a _new entry
 * @_new: _new entry to be added
 * @head: list head to add it after
 *
 * Insert a _new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *_new, struct list_head *head)
{
	__list_add(_new, head, head->next);
}

/**
 * list_add_tail - add a _new entry
 * @_new: _new entry to be added
 * @head: list head to add it before
 *
 * Insert a _new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *_new, struct list_head *head)
{
	__list_add(_new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void list_del(struct list_head *entry)
{
#if 0
    if (!entry->prev || !entry->next) {
	    entry->next = (struct list_head *)LIST_POISON1;
	    entry->prev = (struct list_head *)LIST_POISON2;
        return;
    }
#endif
	__list_del(entry->prev, entry->next);
#if 0
	entry->next = (struct list_head *)LIST_POISON1;
	entry->prev = (struct list_head *)LIST_POISON2;
#endif
}

struct list_head schedule_list = {&schedule_list, &schedule_list};

fthread_t tid = 0;
jmp_buf c_env;
int c_status = READY;
ucontext_t main_ctx;

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

volatile fthread_s *g_current = NULL;
volatile fthread_s *prev = NULL;

void
fthread_add_to_sched_list(fthread_s *f)
{
    list_add_tail(&f->list, &schedule_list);
}

void
fthread_set_current_thread(fthread_s *f)
{
    g_current = f;
}

fthread_s *
fthread_get_first_runnable_thread()
{
    fthread_s *current = NULL;
    fthread_do_scheduling();
    if (schedule_list.next != &schedule_list) {
        current = container_of(schedule_list.next, fthread_s, list);
    }
    fthread_set_current_thread(current);
    return current;
}

fthread_s *
fthread_create(fthread_func_t func, void *args) {
    void *fstack = NULL;
#if MMAP_ALLOC
    fstack = mmap(NULL,
        DEFAULT_STACK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1,
        0);
    if (fstack == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
#else
    fstack = malloc(DEFAULT_STACK_SIZE);
#endif

    fthread_s *fth = (fthread_s *)malloc(sizeof(*fth));
    memset(fth, 0, sizeof(*fth));

    fth->id = ++tid;
    fth->stack = fstack;
    fth->args = args;
    fth->status = 0;
    fth->thread = func;
    fth->status = READY;
    fth->tstate = NULL;

    INIT_LIST_HEAD(&fth->list);
    fthread_add_to_sched_list(fth);

    return fth;
}

fthread_s *
fthread_get_current_thread()
{
    return g_current;
}

void
fthread_main(uint32_t low32, uint32_t hi32) {
    uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
    fthread_s *fth = (fthread_s *)ptr;
    fth->thread(fth->args);
    fth->status = DEAD;
    //    list_del(&fth->list);
}

void
fthread_resched()
{
    fthread_s *runnable = fthread_get_first_runnable_thread();
    if (runnable == NULL) {
        return;
    }
    switch(runnable->status) {
        case READY:
            PyThreadState_Swap(runnable->tstate);
            runnable->status = RUNNING;
            getcontext(&runnable->ctx);
            runnable->ctx.uc_link = &main_ctx;
            runnable->ctx.uc_stack.ss_sp = runnable->stack;
            runnable->ctx.uc_stack.ss_size = DEFAULT_STACK_SIZE;
            //    thread->ctx.uc_stack.ss_flags = 0;
            uintptr_t ptr = (uintptr_t)runnable;
            makecontext(&runnable->ctx, (void (*) (void)) fthread_main, 2,
                (uint32_t)ptr, (uint32_t)(ptr >> 32));

            //            printf("begin thread %d...\n", runnable->id);
            swapcontext(&main_ctx, &runnable->ctx);
            break;
        case SUSPEND:
            PyThreadState_Swap(runnable->tstate);
            runnable->status = RUNNING;
            //            printf("resume thread %d...\n", runnable->id);
            swapcontext(&main_ctx, &runnable->ctx);
            break;
        default:
            assert(0);
            break;
    }

    fthread_s *current = fthread_get_current_thread();
    if (current->status != DEAD) {
        PyThreadState_Swap(NULL);
        current->status = SUSPEND;
        list_del(&current->list);
        fthread_add_to_sched_list(current);
    } else {
        --tid;
        list_del(&current->list);
        fthread_finalize(current);
        PyThreadState_Swap(NULL);
    }
}

int
fthread_do_scheduling()
{
    return 0;
}


void
fthread_start(fthread_s *thread)
{
    fthread_add_to_sched_list(thread);
    //    __asm__("movq %0, %%rsp" : "=m"(thread->stack):);
#if 0
#endif
    //    thread->thread(thread->args);
    return;
}

int
fthread_join(fthread_s *thread)
{
    int ret = -1;
    while (1) {
        if (thread->status == DEAD) {
            break;
        } else {
        }
    }
    return ret;
}

int
fthread_yield() {
    fthread_s *current = fthread_get_current_thread();
    current->status = SUSPEND;
    list_del(&current->list);
    fthread_add_to_sched_list(current);
    swapcontext(&current->ctx, &main_ctx);
}

int
fthread_finalize(fthread_s *fth) {
    Py_EndInterpreter(fth->tstate);
#if MMAP_ALLOC
    munmap(fth->stack, DEFAULT_STACK_SIZE);
#else
    free(fth->stack);
#endif
    free(fth);
}


void print_subinterp(void)
{
    /* Just output some debug stuff */
    PyThreadState *ts = PyThreadState_Get();
    printf("interp %p, thread state %p: ", ts->interp, ts);
    fflush(stdout);
    //    const char *filename = "test_schedule_python.cpython-33.pyc";
    //    FILE *fp = fopen(filename, "r");
#if 1
    PyRun_SimpleString(
        "import sys;"
        "import json;"
        "print('id(modules) =', id(sys.modules));"
        "sys.stdout.flush()"
    );
#endif
    //    PyRun_SimpleFile(fp, filename);
}

void test_main(int total_thread) {
    fthread_s *fth = NULL;
    PyThreadState *substate = NULL;
    int j;
    for (j=0; j<total_thread; j++) {
        substate = Py_NewInterpreter();
        substate->interp->enable_scheduling = 1;
        //        print_subinterp();
        //            Py_EndInterpreter(substate);
        fth = fthread_create(print_subinterp, NULL);
        fth->tstate = substate;
        substate->p_ctx = &fth->ctx;
        substate->interp->p_main_ctx = &main_ctx;
    }

    exit(0);
    while(1) {
        if (tid == 0) {
            break;
        }
        fthread_resched();
    }
}

int main(int argc, char *argv[])
{
    PyThreadState *mainstate, *substate;
#ifdef WITH_THREAD
    PyGILState_STATE gilstate;
#endif
    int i, j;

    for (i=0; i<1; i++) {
        printf("--- Pass %d ---\n", i);
        /* HACK: the "./" at front avoids a search along the PATH in
           Modules/getpath.c */
        Py_SetProgramName(L"./_testembed");
        Py_Initialize();
        mainstate = PyThreadState_Get();

#ifdef WITH_THREAD
        PyEval_InitThreads();
        PyEval_ReleaseThread(mainstate);

        gilstate = PyGILState_Ensure();
#endif
        print_subinterp();
        PyThreadState_Swap(NULL);

#if SCHEDULE_PYTHON
        test_main(300);
#else
        for (j=0; j<300; j++) {
            substate = Py_NewInterpreter();
            print_subinterp();
            Py_EndInterpreter(substate);
        }
#endif


        PyThreadState_Swap(mainstate);
        print_subinterp();
#ifdef WITH_THREAD
        PyGILState_Release(gilstate);
#endif

        PyEval_RestoreThread(mainstate);
        Py_Finalize();
    }
    return 0;
}
