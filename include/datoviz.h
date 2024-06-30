/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC
#define DVZ_HEADER_PUBLIC



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "datoviz_enums.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define DATOVIZ_VERSION_MAJOR 0
#define DATOVIZ_VERSION_MINOR 2
#define DATOVIZ_VERSION_PATCH 0

#ifndef DVZ_EXPORT
#if CC_MSVC
#ifdef DVZ_SHARED
#define DVZ_EXPORT __declspec(dllexport)
#else
#define DVZ_EXPORT __declspec(dllimport)
#endif
#define DVZ_INLINE __forceinline
#else
#define DVZ_EXPORT __attribute__((visibility("default")))
#define DVZ_INLINE static inline __attribute((always_inline))
#endif
#endif



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzBatch DvzBatch;
typedef struct DvzScene DvzScene;
typedef struct DvzFigure DvzFigure;
typedef struct DvzPanel DvzPanel;
typedef struct DvzVisual DvzVisual;
typedef struct DvzTransform DvzTransform;
typedef struct DvzCamera DvzCamera;
typedef struct DvzArcball DvzArcball;
typedef struct DvzPanzoom DvzPanzoom;



/*************************************************************************************************/
/*************************************************************************************************/
/*  Scene API                                                                                    */
/*************************************************************************************************/
/*************************************************************************************************/

EXTERN_C_ON

/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzScene* dvz_scene(DvzBatch* batch);



/**
 *
 */
DVZ_EXPORT void dvz_scene_run(DvzScene* scene, DvzApp* app, uint64_t n_frames);



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
DVZ_EXPORT void
dvz_panel_margins(DvzPanel* panel, float top, float right, float bottom, float left);



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
DVZ_EXPORT DvzCamera* dvz_panel_camera(DvzPanel* panel);


/**
 *
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzScene* scene, DvzPanel* panel);


/**
 *
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzScene* scene, DvzPanel* panel);



/**
 *
 */
DVZ_EXPORT void dvz_panel_update(DvzPanel* panel);



/**
 *
 */
DVZ_EXPORT void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual);



/**
 *
 */
DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Visuals API                                                                                  */
/*************************************************************************************************/
/*************************************************************************************************/



/*************************************************************************************************/
/*  Basic visual                                                                                 */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_basic(DvzBatch* batch, DvzPrimitiveTopology topology, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_basic_position(DvzVisual* basic, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_basic_color(DvzVisual* basic, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_basic_alloc(DvzVisual* basic, uint32_t item_count);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Interactivity API                                                                            */
/*************************************************************************************************/
/*************************************************************************************************/



EXTERN_C_OFF

#endif
