/*************************************************************************************************/
/*  Environment variable utils                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ENV_UTILS
#define DVZ_HEADER_ENV_UTILS


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_macros.h"
#include "datoviz_macros.h"



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static inline char* capture_png(bool* offscreen)
{
    char* capture = getenv("DVZ_CAPTURE_PNG");
    if (capture != NULL && offscreen != NULL)
    {
        // Force offscreen rendering.
        log_info( //
            "DVZ_CAPTURE_PNG environment variable set, forcing offscreen rendering and "
            "capturing image to %s",
            capture);
        *offscreen = true;
    }
    return capture;
}



#endif
