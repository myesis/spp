static int
EarlyInit(int *argc, char **argv) {
    int max_reader_groups;
    int ncpu;
    int ncpuonline;
    int ncpuavailable;

    AviaryPreEarlyInitCpuTopology(
        &max_reader_groups,
        &ncpu,
        &ncpuonline,
        &ncpuavailable);
}