/*************************************************************************************************/
/* Scene                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SCENE
#define DVZ_HEADER_SCENE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_enums.h"
#include "../_log.h"
#include "../_math.h"
#include "mvp.h"
#include "viewport.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzScene DvzScene;
typedef struct DvzFigure DvzFigure;
typedef struct DvzPanel DvzPanel;

// Forward declarations.
typedef struct DvzApp DvzApp;
typedef struct DvzArcball DvzArcball;
typedef struct DvzCamera DvzCamera;
typedef struct DvzList DvzList;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzRequester DvzRequester;
typedef struct DvzTransform DvzTransform;
typedef struct DvzView DvzView;
typedef struct DvzViewset DvzViewset;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzScene
{
    DvzRequester* rqr;
    DvzList* figures;
};



struct DvzFigure
{
    DvzScene* scene;
    DvzList* panels;
    vec2 shape, shape_init; // NOTE: in screen coordinates
    int flags;

    DvzViewset* viewset;
    DvzId canvas_id;
};



struct DvzPanel
{
    DvzFigure* figure;
    DvzView* view;                // has a list of visuals
    vec2 offset_init, shape_init; // initial viewport size
    DvzTransform* transform;
    DvzCamera* camera;
    DvzPanzoom* panzoom;
    DvzArcball* arcball;
    bool transform_to_destroy; // HACK: avoid double destruction with transform sharing
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzScene* dvz_scene(DvzRequester* rqr);


/**
 *
 */
DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_figure_resize(DvzFigure* fig, uint32_t width, uint32_t height);



/**
 *
 */
DvzFigure* dvz_scene_figure(DvzScene* scene, DvzId id);



/**
 *
 */
DVZ_EXPORT void dvz_figure_destroy(DvzFigure* figure);



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzPanel* dvz_panel(DvzFigure* fig, float x, float y, float w, float h);



/**
 *
 */
DVZ_EXPORT DvzPanel* dvz_panel_default(DvzFigure* fig);



/**
 *
 */
DVZ_EXPORT void dvz_panel_transform(DvzPanel* panel, DvzTransform* tr);



/**
 *
 */
DVZ_EXPORT void dvz_panel_resize(DvzPanel* panel, float x, float y, float width, float height);



/**
 *
 */
DVZ_EXPORT bool dvz_panel_contains(DvzPanel* panel, vec2 pos);



/**
 *
 */
DVZ_EXPORT DvzPanel* dvz_panel_at(DvzFigure* figure, vec2 pos);



/**
 *
 */
DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);



/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzCamera* dvz_panel_camera(DvzPanel* panel);



/*************************************************************************************************/
/*  Controllers                                                                                  */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzApp* app, DvzPanel* panel);


/**
 *
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzApp* app, DvzPanel* panel);



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual);



/*************************************************************************************************/
/*  Run                                                                                          */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_scene_run(DvzScene* scene, DvzApp* app, uint64_t n_frames);



EXTERN_C_OFF

#endif
