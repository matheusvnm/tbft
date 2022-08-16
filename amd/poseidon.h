#include "libgomp.h"
#include <stdlib.h>
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
#define MAX_CPUS                128 
#define MAX_PACKAGES            4 


/*Define AURORA environment*/

#define MAX_KERNEL              61
#define MAX_THREADS             32
#define PERFORMANCE             1
#define EDP                     2
#define POWER		        3
#define TEMPERATURE             4
#define TURBO_ON		1
#define TURBO_OFF		0



#define END                     10
#define S0                      0
#define S1                      1
#define S2                      2
#define S3                      3
#define REPEAT                  4
#define SEQUENTIAL_BASE_TESTED  115
#define SEQUENTIAL_BASE_NOT_TESTED 116





/*Global variables*/

static int package_map[MAX_PACKAGES];
char packname[MAX_PACKAGES][256];
char tempfile[256];
double initGlobalTime = 0.0;
unsigned long int idKernels[MAX_KERNEL];
short int id_actual_region=0;
short int metric;
short int totalKernels=0;
short int libTotalPackages=0;
short int libTotalCores=0;



typedef struct{
        short int numThreads;
        short int numCores;
        short int bestThread;
        short int startThreads;
        short int metric;
        short int state;
        int bestFreq;
        short int hasSequentialBase;
        short int pass;
        short int lastThread;
        double bestResult, bestTime, initResult;
        long long kernelBefore[MAX_PACKAGES];
        long long kernelAfter[MAX_PACKAGES];
        long long kernelBeforeSeq[MAX_PACKAGES];
        long long kernelAfterSeq[MAX_PACKAGES];
}typeFrame;

typeFrame libKernels[MAX_KERNEL];

