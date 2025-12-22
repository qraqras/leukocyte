#include <unistd.h>
#include <stdint.h>

#include "worker/jobs.h"

/**
 * @brief Number of worker threads to use (based on available CPUs).
 * @return Number of workers (at least 1)
 */
size_t leuko_num_workers(void)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n < 1)
    {
        return 1;
    }
    if ((unsigned long)n > (unsigned long)SIZE_MAX)
    {
        return (size_t)SIZE_MAX;
    }
    return (size_t)n;
}
