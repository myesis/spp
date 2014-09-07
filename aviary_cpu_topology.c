#include "aviary_cpu_topology.h"
#include "aviary_process.h"

typedef struct {
    int bind_id;
    int bound_id;
} AviaryCpuBindData;

static aviary_cpu_info_t *cpuinfo;

static AviaryCpuBindData *scheduler2cpu_map;

static void
get_logical_processors(int *conf, int *onln, int *avail) {
    if (conf)
        *conf = aviary_get_cpu_configured(cpuinfo);
    if (onln)
        *onln = aviary_get_cpu_online(cpuinfo);
    if (avail)
        *avail = aviary_get_cpu_available(cpuinfo);
}

void AviaryPreEarlyInitCpuTopology(
    int *max_rg_p,
    int *conf,
    int *online,
    int *available) {
    cpuinfo = AviaryCpuInfoCreate();
    get_logical_processors(conf, online, available);
}


void
aviary_init_cpu_topology(void) {
    int idx;

    scheduler2cpu_map = aviary_alloc(
        sizeof(AviaryCpuBindData) * (aviary_no_schedulers+1));
    for (idx = 1; idx <= aviary_no_schedulers; idx++) {
        scheduler2cpu_map[idx].bind_id = -1;
        scheduler2cpu_map[idx].bound_id = -1;
    }

}

