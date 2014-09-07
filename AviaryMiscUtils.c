
AviaryCpuInfo *
AviaryCpuInfoCreate(void) {
    AviaryCpuInfo *cpuinfo = malloc(sizeof(AviaryCpuInfo));
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
AviaryCpuInfoDestory(AviaryCpuInfo *cpuinfo) {
    if (cpuinfo) {
        cpuinfo->configured = 0;
        cpuinfo->online = 0;
        cpuinfo->available = 0;
        cpuinfo->topology_size = 0;
        free(cpuinfo);
    }
}

int
AviaryCpuInfoUpdate(AviaryCpuInfo *cpuinfo) {
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

    read_topology(cpuinfo);
}