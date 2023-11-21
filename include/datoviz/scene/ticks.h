/*************************************************************************************************/
/* Ticks                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TICKS
#define DVZ_HEADER_TICKS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTicks DvzTicks;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_TICKS_HORIZONTAL = 0x0000,
    DVZ_TICKS_VERTICAL = 0x0001,
    DVZ_TICKS_LOG = 0x0010,
} DvzTicksFlags;



// Tick format type.
typedef enum
{
    DVZ_TICKS_FORMAT_UNDEFINED,
    DVZ_TICKS_FORMAT_DECIMAL,
    DVZ_TICKS_FORMAT_SCIENTIFIC,
} DvzTicksFormat;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTicks
{
    int flags;
    double range_size; // size in pixels between the two ends of the axis
    double glyph_size; // average size of a glyph
    double dmin, dmax; // requested min and max of the range

    double lmin, lmax, lstep; // computed min and max of the ticks
    DvzTicksFormat format;    // computed tick format
    uint32_t precision;       // computed tick precision
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzTicks* dvz_ticks(int flags);



DVZ_EXPORT void dvz_ticks_size(DvzTicks* ticks, double range_size, double glyph_size);



DVZ_EXPORT bool
dvz_ticks_compute(DvzTicks* ticks, double dmin, double dmax, uint32_t requested_count);



DVZ_EXPORT uint32_t dvz_ticks_range(DvzTicks* ticks, dvec3 range);



DVZ_EXPORT DvzTicksFormat dvz_ticks_format(DvzTicks* ticks);



DVZ_EXPORT uint32_t dvz_ticks_precision(DvzTicks* ticks);



DVZ_EXPORT bool dvz_ticks_dirty(DvzTicks* ticks, double dmin, double dmax);



DVZ_EXPORT void dvz_ticks_print(DvzTicks* ticks);



DVZ_EXPORT void dvz_ticks_destroy(DvzTicks* ticks);



EXTERN_C_OFF

#endif
