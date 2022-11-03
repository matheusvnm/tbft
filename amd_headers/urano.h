#include "libgomp.h"
#include "urano_states.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <unistd.h>
#include <stdint.h>

/*Define AMD_MSR ENVIRONMENT*/

#define AMD_MSR_PWR_UNIT 0xC0010299
#define AMD_MSR_CORE_ENERGY 0xC001029A
#define AMD_MSR_PACKAGE_ENERGY 0xC001029B
#define AMD_TIME_UNIT_MASK 0xF0000
#define AMD_ENERGY_UNIT_MASK 0x1F00
#define AMD_POWER_UNIT_MASK 0xF
#define STRING_BUFFER 1024
#define MAX_CPUS 128
#define MAX_PACKAGES 4


/*Global variables*/

static int package_map[MAX_PACKAGES];
char packname[MAX_PACKAGES][256];
char tempfile[256];
double initGlobalTime = 0.0;
unsigned long int idKernels[MAX_KERNEL];
short int id_actual_region = 0;
short int metric;
short int totalKernels = 0;
short int libTotalPackages = 0;
short int libTotalCores = 0;

typedef struct
{
        short int numThreads;
        short int numCores;
        short int bestThread;
        short int startThreads;
        short int metric;
        short int state;
        short int hasSequentialBase;
        short int pass;
        short int lastThread;
        double bestResult, bestTime, initResult;
        long long kernelBefore[MAX_PACKAGES];
        long long kernelAfter[MAX_PACKAGES];
        long long kernelBeforeSeq[MAX_PACKAGES];
        long long kernelAfterSeq[MAX_PACKAGES];
} typeFrame;

typeFrame libKernels[MAX_KERNEL];

/* First function called. It initiallizes all the functions and variables used by TBFT/Urano */
void lib_init(int metric, int start_search)
{
    int i;
    int numCores = sysconf(_SC_NPROCESSORS_ONLN);
    /*Initialization of RAPL */
    lib_detect_packages();
    /*End initialization of RAPL */

    int startThreads = numCores;
    while (startThreads != 2 && startThreads != 3 && startThreads != 5)
    {
        startThreads = startThreads / 2;
    }

    /* Initialization of the variables necessary to perform the search algorithm */
    for (i = 0; i < MAX_KERNEL; i++)
    {
        libKernels[i].numThreads = numCores;
        libKernels[i].startThreads = startThreads;
        libKernels[i].numCores = numCores;
        libKernels[i].initResult = 0.0;
        libKernels[i].state = REPEAT;
        libKernels[i].metric = metric;
        libKernels[i].hasSequentialBase = SEQUENTIAL_BASE_NOT_TESTED;
        idKernels[i] = 0;
    }

    /* Start the counters for energy and time for all the application execution */
    id_actual_region = MAX_KERNEL - 1;
    lib_start_energy_collection();
    initGlobalTime = omp_get_wtime();
}

void lib_detect_packages()
{

    char filename[STRING_BUFFER];
    FILE *fff;
    int package;
    int i;

    for (i = 0; i < MAX_PACKAGES; i++)
        package_map[i] = -1;

    for (i = 0; i < MAX_CPUS; i++)
    {
        sprintf(filename, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
        fff = fopen(filename, "r");
        if (fff == NULL)
            break;
        fscanf(fff, "%d", &package);
        fclose(fff);

        if (package_map[package] == -1)
        {
            libTotalPackages++;
            package_map[package] = i;
        }
    }
}

void lib_start_energy_collection()
{
    char msr_filename[STRING_BUFFER];
    int fd;
    sprintf(msr_filename, "/dev/cpu/0/msr");
    fd = open(msr_filename, O_RDONLY);
    if (fd < 0)
    {
        if (errno == ENXIO)
        {
            fprintf(stderr, "rdmsr: No CPU 0\n");
            exit(2);
        }
        else if (errno == EIO)
        {
            fprintf(stderr, "rdmsr: CPU 0 doesn't support MSRs\n");
            exit(3);
        }
        else
        {
            perror("rdmsr:open");
            fprintf(stderr, "Trying to open %s\n", msr_filename);
            exit(127);
        }
    }
    uint64_t data;
    pread(fd, &data, sizeof data, AMD_MSR_PACKAGE_ENERGY);
    libKernels[id_actual_region].kernelBefore[0] = (long long)data;
    close(fd);
}

double lib_end_energy_collection()
{
    char msr_filename[STRING_BUFFER];
    int fd;
    sprintf(msr_filename, "/dev/cpu/0/msr");
    fd = open(msr_filename, O_RDONLY);
    uint64_t data;
    pread(fd, &data, sizeof data, AMD_MSR_PWR_UNIT);
    int core_energy_units = (long long)data;
    unsigned int energy_unit = (core_energy_units & AMD_ENERGY_UNIT_MASK) >> 8;
    pread(fd, &data, sizeof data, AMD_MSR_PACKAGE_ENERGY);
    libKernels[id_actual_region].kernelAfter[0] = (long long)data;
    double result = (libKernels[id_actual_region].kernelAfter[0] - libKernels[id_actual_region].kernelBefore[0]) * pow(0.5, (float)(energy_unit));
    close(fd);
    return result;
}