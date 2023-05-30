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

typedef struct DvzInstance DvzInstance;
typedef struct DvzView DvzView;
typedef struct DvzViewset DvzViewset;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzVisual DvzVisual;
typedef struct DvzList DvzList;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInstance
{
    DvzView* view;
    DvzVisual* visual;

    uint32_t first; // first item (offset)
    uint32_t count; // number of items to draw

    // Instancing.
    uint32_t first_instance;
    uint32_t instance_count;

    bool is_visible;
};



struct DvzView
{
    DvzViewset* viewset;
    vec2 offset, shape;
    DvzList* instances;
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
DVZ_EXPORT void dvz_instance_visible(DvzInstance* instance, bool is_visible);

/**
 *
 */
DVZ_EXPORT void dvz_view_resize(DvzView* view, vec2 offset, vec2 shape);

/**
 *
 */
DVZ_EXPORT void dvz_view_destroy(DvzView* view);



/*************************************************************************************************/
/*  Instance                                                                                     */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzInstance* dvz_view_instance(
    DvzView* view, DvzVisual* visual,                  //
    uint32_t first, uint32_t count,                    // items
    uint32_t first_instance, uint32_t instance_count); // instances

/**
 *
 */
DVZ_EXPORT void dvz_instance_destroy(DvzInstance* instance);



EXTERN_C_OFF

#endif
