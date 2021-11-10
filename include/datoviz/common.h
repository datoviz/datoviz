/*************************************************************************************************/
/*  Common symbols, macros, and includes                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_COMMON
#define DVZ_HEADER_COMMON



/*************************************************************************************************/
/*  Standard includes                                                                            */
/*************************************************************************************************/

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>



/*************************************************************************************************/
/*  External includes                                                                            */
/*************************************************************************************************/

#include "_macros.h"
MUTE_ON
#include <cglm/cglm.h>
MUTE_OFF



/*************************************************************************************************/
/*  Internal includes                                                                            */
/*************************************************************************************************/

#include "_atomic.h"
#include "_debug.h"
#include "_log.h"
#include "_math.h"
#include "_obj.h"
#include "_thread.h"
#include "_time.h"
#include "_types.h"



/*************************************************************************************************/
/*  Built-in fixed constants                                                                     */
/*************************************************************************************************/

#define ENGINE_NAME         "Datoviz"
#define APPLICATION_NAME    "Datoviz"
#define APPLICATION_VERSION VK_MAKE_VERSION(0, 2, 0)

#define DVZ_NEVER -1000000



#endif
