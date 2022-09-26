/*************************************************************************************************/
/* Scene                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SCENE
#define DVZ_HEADER_SCENE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_enums.h"
#include "_log.h"
#include "_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzScene DvzScene;
typedef struct DvzFigure DvzFigure;
typedef struct DvzVisual DvzVisual;
typedef struct DvzPanel DvzPanel;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzList DvzList;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Panel type.
typedef enum
{
    DVZ_PANEL_TYPE_NONE,
    DVZ_PANEL_TYPE_PANZOOM,
    DVZ_PANEL_TYPE_AXES_2D,
} DvzPanelType;



// Prop types.
typedef enum
{
    DVZ_PROP_NONE,
    DVZ_PROP_POS,
    DVZ_PROP_COLOR,
    DVZ_PROP_ALPHA,
    DVZ_PROP_COLORMAP,
    DVZ_PROP_MARKER_SIZE,
    DVZ_PROP_MARKER_TYPE,
    DVZ_PROP_ANGLE,
    DVZ_PROP_TEXT,
    DVZ_PROP_TEXT_SIZE,
    DVZ_PROP_GLYPH,
    DVZ_PROP_ANCHOR,
    DVZ_PROP_LINE_WIDTH,
    DVZ_PROP_MITER_LIMIT,
    DVZ_PROP_CAP_TYPE,
    DVZ_PROP_JOIN_TYPE,
    DVZ_PROP_TOPOLOGY,
    DVZ_PROP_LENGTH,
    DVZ_PROP_RANGE,
    DVZ_PROP_MARGIN,
    DVZ_PROP_NORMAL,
    DVZ_PROP_TEXCOORDS,
    DVZ_PROP_TEXCOEFS,
    DVZ_PROP_IMAGE,
    DVZ_PROP_VOLUME,
    DVZ_PROP_TRANSFER_X,
    DVZ_PROP_TRANSFER_Y,
    DVZ_PROP_LIGHT_POS,
    DVZ_PROP_LIGHT_PARAMS,
    DVZ_PROP_CLIP,
    DVZ_PROP_MODEL,
    DVZ_PROP_VIEW,
    DVZ_PROP_PROJ,
    DVZ_PROP_VIEWPORT,
    DVZ_PROP_TIME,
    DVZ_PROP_INDEX,
    DVZ_PROP_SCALE,
    DVZ_PROP_TRANSFORM,
} DvzPropType;



// Visual types.
typedef enum
{
    DVZ_VISUAL_NONE,

    // Basic visuals.
    DVZ_VISUAL_POINT,
    DVZ_VISUAL_LINE,
    DVZ_VISUAL_LINE_STRIP,
    DVZ_VISUAL_TRIANGLE,
    DVZ_VISUAL_TRIANGLE_STRIP,
    DVZ_VISUAL_TRIANGLE_FAN,

    DVZ_VISUAL_RECTANGLE,

    DVZ_VISUAL_MARKER,
    DVZ_VISUAL_SEGMENT,
    DVZ_VISUAL_ARROW,
    DVZ_VISUAL_PATH,

    DVZ_VISUAL_TEXT,
    DVZ_VISUAL_IMAGE,
    DVZ_VISUAL_IMAGE_CMAP,

    DVZ_VISUAL_DISC,
    DVZ_VISUAL_SECTOR,
    DVZ_VISUAL_MESH,
    DVZ_VISUAL_POLYGON,
    DVZ_VISUAL_PSLG,
    DVZ_VISUAL_HISTOGRAM,
    DVZ_VISUAL_AREA,
    DVZ_VISUAL_CANDLE,

    DVZ_VISUAL_GRAPH,

    DVZ_VISUAL_SURFACE,
    DVZ_VISUAL_VOLUME_SLICE,
    DVZ_VISUAL_VOLUME,

    DVZ_VISUAL_FAKE_SPHERE,
    DVZ_VISUAL_AXES_2D,
    DVZ_VISUAL_AXES_3D,
    DVZ_VISUAL_COLORMAP,

    DVZ_VISUAL_COUNT,

    DVZ_VISUAL_CUSTOM,
} DvzVisualType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzScene // singleton in a given app
{
    DvzRequester* rqr;
    DvzList* visuals;
};



struct DvzFigure
{
    DvzId id;
    DvzScene* scene;
    uint32_t rows, cols;
    float* row_heights;
    float* col_widths;
    uint32_t width, height;
    DvzList* panels;
};



struct DvzPanel
{
    DvzId id;
    DvzFigure* figure;
    DvzPanelType type;
    uint32_t row, col;
    vec2 offset;
};



struct DvzVisual
{
    DvzId id;
    DvzScene* scene;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzScene* dvz_scene(void);

DVZ_EXPORT DvzFigure* dvz_figure(
    DvzScene* scene, uint32_t width, uint32_t height, uint32_t n_rows, uint32_t n_cols, int flags);

DVZ_EXPORT DvzPanel*
dvz_panel(DvzFigure* fig, uint32_t row, uint32_t col, DvzPanelType type, int flags);

DVZ_EXPORT DvzVisual* dvz_visual(DvzScene* scene, DvzVisualType vtype, int flags);

DVZ_EXPORT void
dvz_visual_data(DvzVisual* visual, DvzPropType ptype, uint64_t index, uint64_t count, void* data);

DVZ_EXPORT void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual, int pos);

DVZ_EXPORT void dvz_visual_destroy(DvzVisual* visual);

DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);

DVZ_EXPORT void dvz_figure_destroy(DvzFigure* figure);

DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);



EXTERN_C_OFF

#endif
