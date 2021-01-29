/*************************************************************************************************/
/*  Tick structures used by both the axes visuals and the tick positioning algorithm             */
/*************************************************************************************************/

#ifndef VKL_TICKS_STRUCTS_HEADER
#define VKL_TICKS_STRUCTS_HEADER

#include "common.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Tick format type.
typedef enum
{
    VKL_TICK_FORMAT_UNDEFINED,
    VKL_TICK_FORMAT_DECIMAL,
    VKL_TICK_FORMAT_SCIENTIFIC,
} VklTickFormat;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklAxesContext VklAxesContext;
typedef struct VklAxesTicks VklAxesTicks;
typedef struct Q Q;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklAxesContext
{
    VklAxisCoord coord;
    float size_viewport; // along the current dimension
    float size_glyph;    // either width or height
    float scale_orig;    // scale
    uint32_t extensions; // number of extensions on each side (typically 1)
};



struct VklAxesTicks
{
    double dmin, dmax;              // requested range values
    double lmin_in, lmax_in, lstep; // computed tick range (initial range)
    double lmin_ex, lmax_ex;        // extended tick range (extended left/right or bottom/top)
    uint32_t value_count;           // final number of labels
    uint32_t value_count_req;       // number of values requested
    VklTickFormat format;           // decimal or scientific notation
    uint32_t precision;             // number of digits after the dot
    double* values;                 // from lmin to lmax by lstep
    char* labels;                   // hold all tick labels
};



#endif
