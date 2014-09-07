#ifndef __AVIARY_MISC_UTILS_H__
#define __AVIARY_MISC_UTILS_H__

typedef struct aviary_cpu_info_t_ aviary_cpu_info_t;
typedef struct {
    int node;
    int processor;
    int processor_node;
    int core;
    int thread;
    int logical;
} aviary_cpu_topology_t;

aviary_cpu_info_t *AviaryCpuInfoCreate(void);
void AviaryCpuInfoDestory(aviary_cpu_info_t *cpuinfo);

#endif