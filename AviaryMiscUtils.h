#ifndef __AVIARY_MISC_UTILS_H__
#define __AVIARY_MISC_UTILS_H__

typedef struct {
    int node;
    int processor;
    int processor_node;
    int core;
    int thread;
    int logical;
} AviaryCpuTopology;

AviaryCpuInfo *AviaryCpuInfoCreate(void);
void AviaryCpuInfoDestory(AviaryCpuInfo *cpuinfo);

#endif