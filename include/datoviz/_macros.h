/*************************************************************************************************/
/*  Common macros                                                                                */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MACROS
#define DVZ_HEADER_MACROS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "../datoviz_macros.h"



/*************************************************************************************************/
/*  Build macros                                                                                 */
/*************************************************************************************************/

#ifndef SPIRV_DIR
#define SPIRV_DIR ""
#endif

#ifndef SWIFTSHADER
#define SWIFTSHADER 0
#endif



/*************************************************************************************************/
/*  Misc                                                                                         */
/*************************************************************************************************/

#define ARRAY_COUNT(arr) sizeof((arr)) / sizeof((arr)[0])

#ifdef LANG_CPP
#define INIT(t, n) t n = {};
#else
#define INIT(t, n) t n = {0};
#endif

#define IF_VERBOSE if (getenv("DVZ_VERBOSE") && (strncmp(getenv("DVZ_VERBOSE"), "req", 3) == 0))

#define fsizeof(type, member) sizeof(((type*)0)->member)


#endif
