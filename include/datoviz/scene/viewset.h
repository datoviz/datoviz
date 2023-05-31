/*************************************************************************************************/
/* Viewset                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VIEWSET
#define DVZ_HEADER_VIEWSET



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_enums.h"
#include "../_obj.h"
#include "mvp.h"
#include "viewport.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzView DvzView;
typedef struct DvzViewset DvzViewset;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzVisual DvzVisual;
typedef struct DvzList DvzList;
typedef struct DvzTransform DvzTransform;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzView
{
    DvzViewset* viewset;
    vec2 offset, shape;
    DvzList* visuals;
};



struct DvzViewset
{
    DvzRequester* rqr;
    DvzId canvas_id;
    DvzList* views;
    // DvzView* root;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Viewset                                                                                      */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzViewset* dvz_viewset(DvzRequester* rqr, DvzId canvas_id);



/**
 *
 */
DVZ_EXPORT void dvz_viewset_clear(DvzViewset* viewset);



/**
 *
 */
DVZ_EXPORT void dvz_viewset_build(DvzViewset* viewset);



/**
 *
 */
DVZ_EXPORT void dvz_viewset_destroy(DvzViewset* viewset);



/*************************************************************************************************/
/*  View                                                                                         */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzView* dvz_view(DvzViewset* viewset, vec2 offset, vec2 shape);



/**
 *
 */
DVZ_EXPORT void dvz_view_clear(DvzView* view);



/**
 *
 */
DVZ_EXPORT void dvz_view_resize(DvzView* view, vec2 offset, vec2 shape);



/**
 *
 */
DVZ_EXPORT void dvz_view_destroy(DvzView* view);



/**
 *
 */
DVZ_EXPORT void dvz_view_add(DvzView* view, DvzVisual* visual);



EXTERN_C_OFF

#endif
