PROJECT(AVIARY)
SET(SRC_LIST aviary_main.c aviary_init.c aviary_cpu_topology.c aviary_process.c athread.c aviary_alloc.c time.c aviary_misc_utils.c athr_atomics.c aviary_process_lock.c)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -Wall -O0")
SET( CMAKE_VERBOSE_MAKEFILE on )
ADD_EXECUTABLE(aviary_test ${SRC_LIST})
TARGET_LINK_LIBRARIES(aviary_test pthread)
