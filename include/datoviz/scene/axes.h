/*************************************************************************************************/
/* Axes                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_AXES
#define DVZ_HEADER_AXES



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "datoviz_math.h"



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
DvzAxes* dvz_axes(DvzPanel* panel, int flags);



/**
 *
 */
void dvz_axes_xref(DvzAxes* axes, dvec2 range);



/**
 *
 */
void dvz_axes_yref(DvzAxes* axes, dvec2 range);



/**
 *
 */
void dvz_axes_xget(DvzAxes* axes, dvec2 range_data, vec2 range_ndc);



/**
 *
 */
void dvz_axes_yget(DvzAxes* axes, dvec2 range_data, vec2 range_ndc);



/**
 *
 */
bool dvz_axes_xset(DvzAxes* axes, dvec2 range_data, vec2 range_ndc);



/**
 *
 */
bool dvz_axes_yset(DvzAxes* axes, dvec2 range_data, vec2 range_ndc);



/**
 *
 */
void dvz_axes_resize(DvzAxes* axes);
//, uint32_t viewport_width, uint32_t viewport_height);



/**
 *
 */
void dvz_axes_update(DvzAxes* axes);



/**
 *
 */
void dvz_axes_destroy(DvzAxes* axes);



EXTERN_C_OFF

#endif
