/* File that contains the variable declarations */
#include "urano.h"

/* It defines the number of threads that will execute the actual parallel region based on the current state of the search algorithm */
int lib_resolve_num_threads(uintptr_t ptr_region)
{
    int i;
    id_actual_region = -1;

    /* Find the actual parallel region */
    for (i = 0; i < totalKernels; i++)
    {
        if (idKernels[i] == ptr_region)
        {
            id_actual_region = i;
            break;
        }
    }

    /* If a new parallel region is discovered */
    if (id_actual_region == -1)
    {
        idKernels[totalKernels] = ptr_region;
        id_actual_region = totalKernels;
        totalKernels++;
    }

    /* Check the state of the search algorithm. */
    switch (libKernels[id_actual_region].state)
    {
    case END:
        return libKernels[id_actual_region].bestThread;
    default:
        lib_start_energy_collection();
        libKernels[id_actual_region].initResult = omp_get_wtime();
        return libKernels[id_actual_region].numThreads;
    }
}

/* It is responsible for performing the search algorithm */
void lib_end_parallel_region()
{
    double time, energy, power, result = 0;
    if (libKernels[id_actual_region].state != END)
    {
        /* Check the metric that is being evaluated and collect the results */
        switch (libKernels[id_actual_region].metric)
        {
        case PERFORMANCE:
            time = omp_get_wtime();
            result = time - libKernels[id_actual_region].initResult;
            break;
        case EDP:
            time = omp_get_wtime() - libKernels[id_actual_region].initResult;
            energy = lib_end_energy_collection();
            result = time * energy;
            /* If the result is negative, it means some problem while reading of the hardware counter.
            Then, the metric changes to performance */
            if (result == 0.00000 || result < 0)
            {
                libKernels[id_actual_region].state = REPEAT;
                libKernels[id_actual_region].metric = PERFORMANCE;
            }
            break;
        case POWER:
            time = omp_get_wtime() - libKernels[id_actual_region].initResult;
            energy = lib_end_energy_collection();
            result = energy / time;
            if (result == 0.00000 || result < 0)
            {
                libKernels[id_actual_region].state = REPEAT;
                libKernels[id_actual_region].metric = PERFORMANCE;
            }
            break;
        case TEMPERATURE:
            time = omp_get_wtime() - libKernels[id_actual_region].initResult;
            energy = lib_end_energy_collection();
            result = sqrt((energy * energy) + (time * time));
            if (result == 0.00000 || result < 0)
            {
                libKernels[id_actual_region].state = REPEAT;
                libKernels[id_actual_region].metric = PERFORMANCE;
            }
            break;
        case POWER_TIME_DIST:
            time = omp_get_wtime() - libKernels[id_actual_region].initResult;
            energy = lib_end_energy_collection();
            power = energy / time;
            result = sqrt((power * power) + (time * time));
            if (result == 0.00000 || result < 0)
            {
                libKernels[id_actual_region].state = REPEAT;
                libKernels[id_actual_region].metric = PERFORMANCE;
            }
            break;
        }
        switch (libKernels[id_actual_region].state)
        {
        case REPEAT:
            libKernels[id_actual_region].state = S0;
            libKernels[id_actual_region].numThreads = libKernels[id_actual_region].startThreads;
            libKernels[id_actual_region].lastThread = libKernels[id_actual_region].numThreads;
            break;
        case S0:
            libKernels[id_actual_region].bestResult = result;
            libKernels[id_actual_region].bestThread = libKernels[id_actual_region].numThreads;
            libKernels[id_actual_region].numThreads = libKernels[id_actual_region].bestThread * 2;
            libKernels[id_actual_region].state = S1;
            break;
        case S1:
            if (result < libKernels[id_actual_region].bestResult)
            { // comparing S0 to REPEAT
                libKernels[id_actual_region].bestResult = result;
                libKernels[id_actual_region].bestThread = libKernels[id_actual_region].numThreads;
                if (libKernels[id_actual_region].hasSequentialBase == SEQUENTIAL_BASE_NOT_TESTED)
                {
                    /* if there are opportunities for improvements, then double the number of threads */
                    if (libKernels[id_actual_region].numThreads * 2 <= libKernels[id_actual_region].numCores)
                    {
                        libKernels[id_actual_region].lastThread = libKernels[id_actual_region].numThreads;
                        libKernels[id_actual_region].numThreads = libKernels[id_actual_region].bestThread * 2;
                        libKernels[id_actual_region].state = S1;
                    }
                    else
                    {
                        /* It means that the best number so far is equal to the number of cores */
                        /* Then, it will realize a guided search near to this number */
                        libKernels[id_actual_region].pass = libKernels[id_actual_region].lastThread / 2;
                        libKernels[id_actual_region].numThreads = libKernels[id_actual_region].numThreads - libKernels[id_actual_region].pass;
                        if (libKernels[id_actual_region].pass == 1)
                        {
                            libKernels[id_actual_region].state = S3;
                        }
                        else
                        {
                            libKernels[id_actual_region].state = S2;
                        }
                    }
                }
                else
                {
                    libKernels[id_actual_region].state = END;
                }
            }
            else
            {
                /* Thread scalability stopped */
                /* Find the interval of threads that provided this result. */
                /* if the best number of threads so far is equal to the number of cores, then go to.. */
                if (libKernels[id_actual_region].numThreads == libKernels[id_actual_region].startThreads * 2 && libKernels[id_actual_region].hasSequentialBase == SEQUENTIAL_BASE_NOT_TESTED)
                {
                    libKernels[id_actual_region].lastThread = libKernels[id_actual_region].numThreads;
                    libKernels[id_actual_region].numThreads = 1;
                    libKernels[id_actual_region].hasSequentialBase = SEQUENTIAL_BASE_TESTED;
                }

                else if (libKernels[id_actual_region].bestThread == libKernels[id_actual_region].numCores / 2)
                {
                    libKernels[id_actual_region].pass = libKernels[id_actual_region].lastThread / 2;
                    libKernels[id_actual_region].numThreads = libKernels[id_actual_region].numThreads - libKernels[id_actual_region].pass;
                    if (libKernels[id_actual_region].pass == 1)
                    {
                        libKernels[id_actual_region].state = S3;
                    }
                    else
                    {
                        libKernels[id_actual_region].state = S2;
                    }
                }
                else
                {
                    libKernels[id_actual_region].pass = libKernels[id_actual_region].lastThread / 2;
                    libKernels[id_actual_region].numThreads = libKernels[id_actual_region].numThreads + libKernels[id_actual_region].pass;
                    if (libKernels[id_actual_region].pass == 1)
                    {
                        libKernels[id_actual_region].state = S3;
                    }
                    else
                    {
                        libKernels[id_actual_region].state = S2;
                    }
                }
            }
            break;
        case S2:
            if (libKernels[id_actual_region].bestResult < result)
            {
                libKernels[id_actual_region].pass = libKernels[id_actual_region].pass / 2;
                libKernels[id_actual_region].numThreads = libKernels[id_actual_region].numThreads + libKernels[id_actual_region].pass;
                if (libKernels[id_actual_region].pass == 1)
                {
                    libKernels[id_actual_region].state = S3;
                }
                else
                {
                    libKernels[id_actual_region].state = S2;
                }
            }
            else
            {
                libKernels[id_actual_region].bestThread = libKernels[id_actual_region].numThreads;
                libKernels[id_actual_region].bestResult = result;
                libKernels[id_actual_region].pass = libKernels[id_actual_region].pass / 2;
                libKernels[id_actual_region].numThreads = libKernels[id_actual_region].numThreads + libKernels[id_actual_region].pass;
                if (libKernels[id_actual_region].pass == 1)
                {
                    libKernels[id_actual_region].state = S3;
                }
                else
                {
                    libKernels[id_actual_region].state = S2;
                }
            }
            break;
        case S3: /*The last comparison to define the best number of threads*/
            libKernels[id_actual_region].state = END;
            if (result < libKernels[id_actual_region].bestResult)
            {
                libKernels[id_actual_region].bestThread = libKernels[id_actual_region].numThreads;
            }
            break;
        }
    }
}


/* It finalizes the environment of TBFT/Urano */
void lib_destructor()
{
    double time = omp_get_wtime() - initGlobalTime;
    id_actual_region = MAX_KERNEL - 1;
    double energy = lib_end_energy_collection();
    float edp = time * energy;
    float power = energy / time;
    float euclidian_distance = sqrt((power * power) + (time * time));
    printf("TBFT/Urano - Execution Time: %.5f seconds\n", time);
    printf("TBFT/Urano - Energy: %.5f joules\n", energy);
    printf("TBFT/Urano - EDP: %.5f\n", edp);
    printf("TBFT/Urano - Power Consumption: %.5f watts\n", power);
    printf("TBFT/Urano - Eudiclian Distance (Power - Execution Time): %.5f\n", euclidian_distance);
    printf("TBFT/Urano - Number of Detected Regions: %d\n", totalKernels);
    printf("TBFT/Urano - Number of Threads per Region [");
    
    for (int i = 0; i < totalKernels; i++)
    {
        if (libKernels[i].bestThread != 0)
        {
            printf(" %d ", libKernels[i].bestThread);
        }
        else
        {
            printf(" %d* ", libKernels[i].numThreads);
        }
    }
    printf("]\n");
    printf("TBFT/Urano - Regions with * after the number means that the given region did not finished the search because it executes too few iterations.\n");
}