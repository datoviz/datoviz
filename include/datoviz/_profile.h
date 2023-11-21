/*************************************************************************************************/
/*  Profiling utils                                                                              */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PROFILE
#define DVZ_HEADER_PROFILE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <_log.h>
#include <memorymeasure.h>
#include <stddef.h>



/*************************************************************************************************/
/*  Functions                                                                          */
/*************************************************************************************************/

static unsigned long dvz_memory(void)
{
    unsigned long currRealMem, peakRealMem, currVirtMem, peakVirtMem;

    int success = getMemory(&currRealMem, &peakRealMem, &currVirtMem, &peakVirtMem);
    if (success)
    {
        log_error("memory measure failed");
        return 0;
    }

    return currRealMem;
}



#endif
