/*************************************************************************************************/
/*  Tick structures used by both the axes visuals and the tick positioning algorithm             */
/*************************************************************************************************/

#ifndef DVZ_TICKS_STRUCTS_HEADER
#define DVZ_TICKS_STRUCTS_HEADER

#include "common.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Tick format type.
typedef enum
{
    DVZ_TICK_FORMAT_UNDEFINED,
    DVZ_TICK_FORMAT_DECIMAL,
    DVZ_TICK_FORMAT_SCIENTIFIC,
} DvzTickFormat;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAxesContext DvzAxesContext;
typedef struct DvzAxesTicks DvzAxesTicks;
typedef struct Q Q;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAxesContext
{
    DvzAxisCoord coord;
    float size_viewport; // along the current dimension
    float size_glyph;    // either width or height
    float scale_orig;    // scale
    uint32_t extensions; // number of extensions on each side (typically 1)
};



struct DvzAxesTicks
{
    double dmin, dmax;              // requested range values
    double lmin_in, lmax_in, lstep; // computed tick range (initial range)
    double lmin_ex, lmax_ex;        // extended tick range (extended left/right or bottom/top)
    uint32_t value_count;           // final number of labels
    uint32_t value_count_req;       // number of values requested
    DvzTickFormat format;           // decimal or scientific notation
    uint32_t precision;             // number of digits after the dot
    double* values;                 // from lmin to lmax by lstep
    char* labels;                   // hold all tick labels
};



#endif
