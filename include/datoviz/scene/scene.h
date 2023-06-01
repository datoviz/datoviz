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
typedef struct DvzRequester DvzRequester;
typedef struct DvzList DvzList;
typedef struct DvzView DvzView;
typedef struct DvzVisual DvzVisual;
typedef struct DvzTransform DvzTransform;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzArcball DvzArcball;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzScene // singleton in a given app
{
    DvzApp* app;
    DvzList* figures;
};



struct DvzFigure
{
    DvzScene* scene;
    DvzList* panels;
};



struct DvzPanel
{
    DvzFigure* figure;
    DvzView* view; // has a list of visuals
    DvzTransform* transform;
    DvzPanzoom* panzoom;
    DvzArcball* arcball;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzScene* dvz_scene(DvzApp* app);


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
DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);



/*************************************************************************************************/
/*  Controllers                                                                                  */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel);


/**
 *
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzPanel* panel);



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_panel_pixel(DvzPanel* panel, int flags);



EXTERN_C_OFF

#endif
