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
#include "datoviz_math.h"
#include "datoviz_types.h"
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
typedef struct DvzBatch DvzBatch;
typedef struct DvzTransform DvzTransform;
typedef struct DvzView DvzView;
typedef struct DvzViewset DvzViewset;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Default object IDs used for empty textures bound to unused texture descriptors.
#define DVZ_SCENE_DEFAULT_TEX_ID     1
#define DVZ_SCENE_DEFAULT_SAMPLER_ID 2



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzScene
{
    DvzApp* app;
    DvzBatch* batch;
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
    DvzTransform* static_transform;
    DvzCamera* camera;
    DvzPanzoom* panzoom;
    DvzArcball* arcball;
    bool transform_to_destroy; // HACK: avoid double destruction with transform sharing
};



#endif
