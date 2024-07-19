/*************************************************************************************************/
/*  Common symbols, macros, and includes                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_COMMON
#define DVZ_HEADER_COMMON



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../datoviz_macros.h"
#include "../datoviz_math.h"
#include "../datoviz_version.h"
#include "_log.h"
#include "_macros.h"
#include "_obj.h"
#include "_thread.h"
#include "_time.h"

#include "_debug.h"



/*************************************************************************************************/
/*  Built-in fixed constants                                                                     */
/*************************************************************************************************/

#define ENGINE_NAME      DVZ_NAME
#define APPLICATION_NAME DVZ_NAME
#define APPLICATION_VERSION                                                                       \
    VK_MAKE_VERSION(DVZ_VERSION_MAJOR, DVZ_VERSION_MINOR, DVZ_VERSION_PATCH)

#define DVZ_NEVER -1000000



/*************************************************************************************************/
/*  Defaults                                                                                     */
/*************************************************************************************************/

#define DVZ_DEFAULT_VIEWPORT                                                                      \
    (vec2) { 0, 0 }
#define DVZ_DEFAULT_FORMAT DVZ_FORMAT_B8G8R8A8_UNORM
#define DVZ_DEFAULT_CLEAR_COLOR                                                                   \
    (cvec4) { 0, 0, 0, 0 }



#endif
