#include <pthread.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include "aviary_errno.h"
#include "aviary_misc_utils.h"

struct aviary_cpu_info_t_ {
    int configured;
    int online;
    int available;
    int topology_size;
    aviary_cpu_topology_t *topology;
    pid_t pid;
};

aviary_cpu_info_t *
AviaryCpuInfoCreate(void) {
    aviary_cpu_info_t *cpuinfo = malloc(sizeof(aviary_cpu_info_t));
    cpuinfo->pid = getpid();
    cpuinfo->topology_size = 0;
    cpuinfo->topology = NULL;
    cpuinfo->configured = -1;
    cpuinfo->online = -1;
    cpuinfo->available = -1;
    AviaryCpuInfoUpdate(cpuinfo);
    return cpuinfo;
}

void
AviaryCpuInfoDestory(aviary_cpu_info_t *cpuinfo) {
    if (cpuinfo) {
        cpuinfo->configured = 0;
        cpuinfo->online = 0;
        cpuinfo->available = 0;
        cpuinfo->topology_size = 0;
        free(cpuinfo);
    }
}

int
AviaryCpuInfoUpdate(aviary_cpu_info_t *cpuinfo) {
    int configured;
    int online;
    configured = (int) sysconf(_SC_NPROCESSORS_CONF);
    if (configured < 0) {
        configured = 0;
    }
    online = (int) sysconf(_SC_NPROCESSORS_ONLN);
    if (online < 0) {
        online = 0;
    }

    cpuinfo->configured = configured;
    cpuinfo->online = online;

    //    read_topology(cpuinfo);
}

int
aviary_get_cpu_configured(aviary_cpu_info_t *cpuinfo)
{
    if (!cpuinfo)
        return -EINVAL;
    if (cpuinfo->configured <= 0)
        return -ENOTSUP;
    return cpuinfo->configured;
}

int
aviary_get_cpu_online(aviary_cpu_info_t *cpuinfo)
{
    if (!cpuinfo)
        return -EINVAL;
    if (cpuinfo->online <= 0)
        return -ENOTSUP;
    return cpuinfo->online;
}

int
aviary_get_cpu_available(aviary_cpu_info_t *cpuinfo)
{
    if (!cpuinfo)
        return -EINVAL;
    if (cpuinfo->available <= 0)
        return -ENOTSUP;
    return cpuinfo->available;
}

