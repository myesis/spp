#include "sys.h"
#include "aviary_process.h"
#include "aviary_port.h"

static int no_schedulers;
static int no_schedulers_online;

static int
early_init(int *argc, char **argv) {
    int max_reader_groups;
    int ncpu;
    int ncpuonline;
    int ncpuavailable;

    AviaryPreEarlyInitCpuTopology(
        &max_reader_groups,
        &ncpu,
        &ncpuonline,
        &ncpuavailable);

    no_schedulers = ncpu;
    no_schedulers_online = ncpu;

    aviary_pre_init_process();

    return ncpu;
}

static void
aviary_init(
    int ncpu,
    int proc_tab_sz,
    int legacy_proc_tab,
    int port_tab_sz,
    int port_tab_sz_ignore_files,
    int legacy_port_tab) {

    aviary_init_time();
    aviary_init_scheduling(
        no_schedulers,
        no_schedulers_online);
    aviary_init_cpu_topology();
}

void aviary_start(int argc, char **argv) {
    int ncpu = early_init(&argc, argv);
    int proc_tab_sz = AVIARY_DEFAULT_MAX_PROCESSES;
    int port_tab_sz = AVIARY_DEFAULT_MAX_PORTS;
    int port_tab_sz_ignore_files = 0;
    int legacy_proc_tab = 0;
    int legacy_port_tab = 0;

    aviary_init(
        ncpu,
        proc_tab_sz,
        legacy_proc_tab,
        port_tab_sz,
        port_tab_sz_ignore_files,
        legacy_port_tab);

    aviary_start_schedulers();
}