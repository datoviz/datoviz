/*************************************************************************************************/
/* Axes                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_AXES
#define DVZ_HEADER_AXES



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

typedef struct DvzAxes DvzAxes;

// Forward declarations.
typedef struct DvzPanel DvzPanel;
typedef struct DvzTicks DvzTicks;
typedef struct DvzAxis DvzAxis;
typedef struct DvzLabels DvzLabels;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAxes
{
    DvzPanel* panel;
    DvzTicks* xticks;
    DvzTicks* yticks;
    DvzAxis* xaxis;
    DvzAxis* yaxis;
    DvzLabels* xlabels;
    DvzLabels* ylabels;

    dvec2 xref, yref;

    int flags;
    void* user_data;
};



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/


EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzAxes* dvz_axes(DvzPanel* panel, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_axes_xref(DvzAxes* axes, dvec2 range);



/**
 *
 */
DVZ_EXPORT void dvz_axes_yref(DvzAxes* axes, dvec2 range);



/**
 *
 */
DVZ_EXPORT void dvz_axes_xget(DvzAxes* axes, dvec2 range_data, vec2 range_ndc);



/**
 *
 */
DVZ_EXPORT void dvz_axes_yget(DvzAxes* axes, dvec2 range_data, vec2 range_ndc);



/**
 *
 */
DVZ_EXPORT bool dvz_axes_xset(DvzAxes* axes, dvec2 range_data, vec2 range_ndc);



/**
 *
 */
DVZ_EXPORT bool dvz_axes_yset(DvzAxes* axes, dvec2 range_data, vec2 range_ndc);



/**
 *
 */
DVZ_EXPORT void dvz_axes_resize(DvzAxes* axes);
//, uint32_t viewport_width, uint32_t viewport_height);



/**
 *
 */
DVZ_EXPORT void dvz_axes_update(DvzAxes* axes);



/**
 *
 */
DVZ_EXPORT void dvz_axes_destroy(DvzAxes* axes);



EXTERN_C_OFF

#endif
