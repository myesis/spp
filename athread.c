#include "aviary_errno.h"
#include "aviary_threads.h"
//#include "athread.h"

typedef struct {
    //    athr_atomic32_t result;
    //    ethr_ts_event *tse;
    void *(*thr_func)(void *);
    void *arg;
    //    void *prep_func_res;
} athr_thr_wrap_data__;

static void *thr_wrapper(void *vtwd) {
    void *res;
    //    athr_sint32_t result;
    athr_thr_wrap_data__ *twd = (athr_thr_wrap_data__ *) vtwd;
    void *(*thr_func)(void *) = twd->thr_func;
    void *arg = twd->arg;

    //    athr_atomic32_set(&twd->result, result);

    res = (*thr_func)(arg);

    return res;
}

int
athr_thr_create(
    athr_tid *tid,
    void *(*func)(void *),
    void *arg,
    athr_thr_opts *opts) {

    athr_thr_wrap_data__ twd;
    pthread_attr_t attr;
    int res, dres;

    //    athr_atomic32_init(&twd.result, (athr_sint32_t) -1);
    twd.thr_func = func;
    twd.arg = arg;

    res = pthread_attr_init(&attr);
    if (res != 0) {
        return res;
    }

    res = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    if (res != 0 && res != ENOTSUP) {
        goto error;
    }

#ifdef ATHR_STACK_GUARD_SIZE
    (void) pthread_attr_setguardsize(&attr, ATHR_STACK_GUARD_SIZE);
#endif

    res = pthread_attr_setdetachstate(
        &attr,
        (opts && opts->detached
            ? PTHREAD_CREATE_DETACHED
            : PTHREAD_CREATE_JOINABLE));
    if (res != 0) {
        goto error;
    }

    res = pthread_create((pthread_t *)tid, &attr, thr_wrapper, (void*) &twd);

    if (res == 0) {
        /* TODO : wait for child to initialize... */
    }

error:
    dres = pthread_attr_destroy(&attr);
    if (res == 0) {
        res = dres;
    }
    return res;
}

int
athr_tsd_key_create(athr_tsd_key *keyp, char *keyname) {
    return pthread_key_create((pthread_key_t *) keyp, NULL);
}

int
athr_tsd_set(athr_tsd_key key, void *value) {
    return pthread_setspecific((pthread_key_t) key, value);
}

void *
athr_tsd_get(athr_tsd_key key) {
    return pthread_getspecific((pthread_key_t) key);
}