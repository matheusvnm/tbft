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

/*Define RAPL Environment*/

#define CPU_SANDYBRIDGE 42
#define CPU_SANDYBRIDGE_EP 45
#define CPU_IVYBRIDGE 58
#define CPU_IVYBRIDGE_EP 62
#define CPU_HASWELL 60
#define CPU_HASWELL_EP 63
#define CPU_HASWELL_1 69
#define CPU_BROADWELL 61
#define CPU_BROADWELL_EP 79
#define CPU_BROADWELL_DE 86
#define CPU_SKYLAKE 78
#define CPU_SKYLAKE_1 94
#define NUM_RAPL_DOMAINS 4
#define MAX_CPUS 128
#define MAX_PACKAGES 4
#define MIN_NUM_THREADS 1


/*Global variables*/

static int package_map[MAX_PACKAGES];
static int total_packages = 0, total_cores = 0;
char rapl_domain_names[NUM_RAPL_DOMAINS][30] = {"energy-cores", "energy-gpu", "energy-pkg", "energy-ram"};
char event_names[MAX_PACKAGES][NUM_RAPL_DOMAINS][256];
char filenames[MAX_PACKAGES][NUM_RAPL_DOMAINS][256];
char packname[MAX_PACKAGES][256];
char tempfile[256];
int valid[MAX_PACKAGES][NUM_RAPL_DOMAINS];
double initGlobalTime = 0.0;
unsigned long int idKernels[MAX_KERNEL];
short int id_actual_region = 0;
short int totalKernels = 0;

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
    long long kernelBefore[MAX_PACKAGES][NUM_RAPL_DOMAINS];
    long long kernelAfter[MAX_PACKAGES][NUM_RAPL_DOMAINS];
    long long kernelBeforeSeq[MAX_PACKAGES][NUM_RAPL_DOMAINS];
    long long kernelAfterSeq[MAX_PACKAGES][NUM_RAPL_DOMAINS];
} typeFrame;

typeFrame libKernels[MAX_KERNEL];



/* First function called. It initiallizes all the functions and variables used by TBFT/Urano */
void lib_init(int metric, int start_search)
{
    int i;
    int numCores = sysconf(_SC_NPROCESSORS_ONLN);
    /*Initialization of RAPL */
    lib_detect_cpu();
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

/* Function used by the Intel RAPL to detect the CPU Architecture*/
void lib_detect_cpu()
{
    FILE *fff;
    int family, model = -1;
    char buffer[BUFSIZ], *result;
    char vendor[BUFSIZ];
    fff = fopen("/proc/cpuinfo", "r");
    while (1)
    {
        result = fgets(buffer, BUFSIZ, fff);
        if (result == NULL)
            break;
        if (!strncmp(result, "vendor_id", 8))
        {
            sscanf(result, "%*s%*s%s", vendor);
            if (strncmp(vendor, "GenuineIntel", 12))
            {
                printf("%s not an Intel chip\n", vendor);
            }
        }
        if (!strncmp(result, "cpu family", 10))
        {
            sscanf(result, "%*s%*s%*s%d", &family);
            if (family != 6)
            {
                printf("Wrong CPU family %d\n", family);
            }
        }
        if (!strncmp(result, "model", 5))
        {
            sscanf(result, "%*s%*s%d", &model);
        }
    }
    fclose(fff);
}

/* Function used by the Intel RAPL to detect the number of cores and CPU sockets*/
void lib_detect_packages()
{
    char filename[BUFSIZ];
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
            total_packages++;
            package_map[package] = i;
        }
    }
    total_cores = i;
}

/* Function used by the Intel RAPL to store the actual value of the hardware counter*/
void lib_start_energy_collection()
{
    int i, j;
    FILE *fff;
    for (j = 0; j < total_packages; j++)
    {
        i = 0;
        sprintf(packname[j], "/sys/class/powercap/intel-rapl/intel-rapl:%d", j);
        sprintf(tempfile, "%s/name", packname[j]);
        fff = fopen(tempfile, "r");
        if (fff == NULL)
        {
            fprintf(stderr, "\tCould not open %s\n", tempfile);
            exit(0);
        }
        fscanf(fff, "%s", event_names[j][i]);
        valid[j][i] = 1;
        fclose(fff);
        sprintf(filenames[j][i], "%s/energy_uj", packname[j]);

        /* Handle subdomains */
        for (i = 1; i < NUM_RAPL_DOMAINS; i++)
        {
            sprintf(tempfile, "%s/intel-rapl:%d:%d/name", packname[j], j, i - 1);
            fff = fopen(tempfile, "r");
            if (fff == NULL)
            {
                // fprintf(stderr,"\tCould not open %s\n",tempfile);
                valid[j][i] = 0;
                continue;
            }
            valid[j][i] = 1;
            fscanf(fff, "%s", event_names[j][i]);
            fclose(fff);
            sprintf(filenames[j][i], "%s/intel-rapl:%d:%d/energy_uj", packname[j], j, i - 1);
        }
    }
    /* Gather before values */
    for (j = 0; j < total_packages; j++)
    {
        for (i = 0; i < NUM_RAPL_DOMAINS; i++)
        {
            if (valid[j][i])
            {
                fff = fopen(filenames[j][i], "r");
                if (fff == NULL)
                {
                    fprintf(stderr, "\tError opening %s!\n", filenames[j][i]);
                }
                else
                {
                    fscanf(fff, "%lld", &libKernels[id_actual_region].kernelBefore[j][i]);
                    fclose(fff);
                }
            }
        }
    }
}

/* Function used by the Intel RAPL to load the value of the hardware counter and returns the energy consumption*/
double lib_end_energy_collection()
{
    int i, j;
    FILE *fff;
    double total = 0;
    for (j = 0; j < total_packages; j++)
    {
        for (i = 0; i < NUM_RAPL_DOMAINS; i++)
        {
            if (valid[j][i])
            {
                fff = fopen(filenames[j][i], "r");
                if (fff == NULL)
                {
                    fprintf(stderr, "\tError opening %s!\n", filenames[j][i]);
                }
                else
                {
                    fscanf(fff, "%lld", &libKernels[id_actual_region].kernelAfter[j][i]);
                    fclose(fff);
                }
            }
        }
    }
    for (j = 0; j < total_packages; j++)
    {
        for (i = 0; i < NUM_RAPL_DOMAINS; i++)
        {
            if (valid[j][i])
            {
                if (strcmp(event_names[j][i], "core") != 0 && strcmp(event_names[j][i], "uncore") != 0)
                {
                    total += (((double)libKernels[id_actual_region].kernelAfter[j][i] - (double)libKernels[id_actual_region].kernelBefore[j][i]) / 1000000.0);
                }
            }
        }
    }
    return total;
}