#ifndef VKL_SCENE_HEADER
#define VKL_SCENE_HEADER

#include "builtin_visuals.h"
#include "interact.h"
#include "panel.h"
#include "visuals.h"
#include "../external/exwilk.h"
//#include "../../src/axes.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_VISUALS_PER_CONTROLLER 64



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Controller type
typedef enum
{
    VKL_CONTROLLER_NONE,
    VKL_CONTROLLER_PANZOOM,
    VKL_CONTROLLER_AXES_2D,
    VKL_CONTROLLER_ARCBALL,
    VKL_CONTROLLER_CAMERA,
    VKL_CONTROLLER_AXES_3D,
} VklControllerType;



// Coordinate system
typedef enum
{
    VKL_CDS_DATA = 1,       // data coordinate system
    VKL_CDS_GPU = 2,        // data coordinates normalized to -1,+1 and sent to the GPU
    VKL_CDS_PANZOOM = 3,    // normalized coords within the panel inner's viewport (w/ panzoom)
    VKL_CDS_PANEL = 4,      // NDC coordinates within the outer panel viewport
    VKL_CDS_CANVAS_NDC = 5, // normalized coords within the canvas
    VKL_CDS_CANVAS_PX = 6,  // same but in pixels, origin at the upper left
} VklCDS;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklScene VklScene;
typedef struct VklController VklController;
typedef struct VklTransform VklTransform;
typedef struct VklAxes2D VklAxes2D;
// typedef struct VklAxesTicks VklAxesTicks;
typedef union VklControllerUnion VklControllerUnion;

typedef void (*VklControllerCallback)(VklController* controller, VklEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// struct VklAxesTicks
// {
//     double dmin, dmax;        // range values
//     double lmin, lmax, lstep; // tick range and interval
//     VklTickFormat format;     // best format
//     uint32_t value_count;     // final number of labels
//     uint32_t value_count_req; // number of values requested
//     double* values;           // from lmin to lmax by lstep
//     char* labels;             // hold all tick labels
// };



struct VklAxes2D
{
    VklAxesTicks ticks[2];
    dvec2 panzoom_range[2]; // current panzoom range in data coordinates
    dvec2 tick_range[2]; // extended range (in data coordinates) used to compute the current ticks
};



union VklControllerUnion
{
    VklAxes2D axes_2D;
};



struct VklController
{
    VklObject obj;
    VklPanel* panel;
    int flags;

    VklControllerType type;

    uint32_t visual_count;
    VklVisual* visuals[VKL_MAX_VISUALS_PER_CONTROLLER];

    uint32_t interact_count;
    VklInteract interacts[VKL_MAX_VISUALS_PER_CONTROLLER];

    VklDataCoords coords;
    // may call vkl_visual_update() on all visuals in the panel
    VklControllerCallback callback;
    VklControllerUnion u;
};



struct VklScene
{
    VklObject obj;
    VklCanvas* canvas;

    // The grid contains the panels.
    VklGrid grid;

    // Visuals.
    VklContainer visuals;

    // Controllers.
    VklContainer controllers;
};



/*************************************************************************************************/
/*  Transform definitions                                                                        */
/*************************************************************************************************/

struct VklTransform
{
    dvec2 scale, shift;
};



/*************************************************************************************************/
/*  Transform functions                                                                          */
/*************************************************************************************************/

VklTransform vkl_transform(VklPanel* panel, VklCDS source, VklCDS target);

VklTransform vkl_transform_inv(VklTransform);

VklTransform vkl_transform_mul(VklTransform, VklTransform);

VklTransform vkl_transform_interp(dvec2 pin, dvec2 pout, dvec2 qin, dvec2 qout);

void vkl_transform_apply(VklTransform*, dvec2 in, dvec2 out);



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklScene* vkl_scene(VklCanvas* canvas, uint32_t n_rows, uint32_t n_cols);

VKY_EXPORT void vkl_scene_destroy(VklScene* scene);



/*************************************************************************************************/
/*  Controller                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VklController vkl_controller(VklPanel* panel);

VKY_EXPORT void vkl_controller_visual(VklController* controller, VklVisual* visual);

VKY_EXPORT void vkl_controller_interact(VklController* controller, VklInteractType type);

VKY_EXPORT void vkl_controller_callback(VklController* controller, VklControllerCallback callback);

VKY_EXPORT void vkl_controller_update(VklController* controller);

VKY_EXPORT void vkl_controller_destroy(VklController* controller);



VKY_EXPORT VklController
vkl_controller_builtin(VklPanel* panel, VklControllerType type, int flags);



/*************************************************************************************************/
/*  High-level functions                                                                         */
/*************************************************************************************************/

// VKY_EXPORT VklController* vkl_scene_controller(VklPanel* panel, VklControllerType type, int
// flags);

VKY_EXPORT VklVisual* vkl_scene_visual(VklPanel* panel, VklVisualType type, int flags);

VKY_EXPORT VklPanel*
vkl_scene_panel(VklScene* scene, uint32_t row, uint32_t col, VklControllerType type, int flags);

// VKY_EXPORT void vkl_visual_toggle(VklVisual* visual, VklVisualVisibility visibility);



static void _default_controller_callback(VklController* controller, VklEvent ev)
{
    VklScene* scene = controller->panel->scene;
    VklCanvas* canvas = scene->canvas;

    // Controller interactivity.
    VklInteract* interact = NULL;

    // Use all interact of the controllers.
    for (uint32_t i = 0; i < controller->interact_count; i++)
    {
        interact = &controller->interacts[i];
        // float delay = canvas->clock.elapsed - interact->last_update;

        // Update the interact using the current panel's viewport.
        VklViewport viewport = controller->panel->viewport;
        vkl_interact_update(interact, viewport, &canvas->mouse, &canvas->keyboard);
        // NOTE: the CPU->GPU transfer occurs at every frame, in another callback below
    }
}



/*************************************************************************************************/
/*  Axes functions                                                                               */
/*************************************************************************************************/

// Create the ticks context.
static VklAxesContext _axes_context(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);

    ASSERT(controller->panel != NULL);
    ASSERT(controller->panel->grid != NULL);

    // Canvas size, used in tick computation.
    VklCanvas* canvas = controller->panel->grid->canvas;
    ASSERT(canvas!= NULL);
    float dpi_scaling = controller->panel->viewport.dpi_scaling;
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);

    // Make axes context.
    VklAxesContext ctx = {0};
    ctx.coord = coord;
    ctx.size_viewport = size[coord];
    ctx.size_glyph =
        coord == VKL_AXES_COORD_X ? VKL_FONT_ATLAS.glyph_width : VKL_FONT_ATLAS.glyph_height;
    ctx.size_glyph *= dpi_scaling;

    return ctx;
}

// Recompute the tick locations as a function of the current axis range in data coordinates.
static void _axes_ticks(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    // Find the ticks given the range.
    double vmin = axes->panzoom_range[coord][0];
    double vmax = axes->panzoom_range[coord][1];
    double vlen = vmax - vmin;
    ASSERT(vlen > 0);

    // Extended range for tolerancy during panzoom.
    double vmin0 = vmin - vlen;
    double vmax0 = vmax + vlen;
    ASSERT(vmin0 < vmax0);

    // Prepare context for tick computation.
    VklAxesContext ctx = _axes_context(controller, coord);

    // Determine the tick number and positions.
    axes->ticks[coord] = vkl_ticks(vmin0, vmax0, ctx);
}

// Update the axes visual's data as a function of the computed ticks.
static void _axes_upload(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(controller->visual_count == 2);

    VklVisual* visual = controller->visuals[coord];

    VklAxesTicks* axticks = &axes->ticks[coord];
    uint32_t N = axticks->value_count;
    // Convert ticks from double to float.
    float* ticks = calloc(N, sizeof(float));
    for (uint32_t i = 0; i < N; i++)
        ticks[i] = axticks->values[i];

    // Prepare text values.
    char** text = calloc(N, sizeof(char*));
    for (uint32_t i = 0; i < N; i++)
        text[i] = &axticks->labels[i * MAX_GLYPHS_PER_TICK];

    // TODO: more minor ticks between the major ticks.
    float lim[] = {-1};
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_LIM, 1, lim);
    vkl_visual_data(visual, VKL_PROP_TEXT, 0, N, text);

    FREE(ticks);
    FREE(text);
}

// Initialize the ticks positions and visual.
static void _axes_ticks_init(VklController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        // TODO: initial data coordinates from the panel
        axes->panzoom_range[coord][0] = -1;
        axes->panzoom_range[coord][1] = +1;
        axes->tick_range[coord][0] = -1;
        axes->tick_range[coord][1] = +1;

        // Check the initial range.
        ASSERT(axes->tick_range[coord][0] < axes->tick_range[coord][1]);

        // Compute the ticks for these ranges.
        _axes_ticks(controller, coord);

        // Upload the data.
        _axes_upload(controller, coord);
    }
}

// Determine the coords that need to be updated during panzoom because of overlapping labels.
static void _axes_collision(VklController* controller, bool* update)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(update != NULL);

    // Determine whether the ticks are overlapping, if so we should recompute the ticks (zooming)
    // Same if there are less than N visible labels (dezooming)
    for (uint32_t i = 0; i < 2; i++)
    {
        VklAxisCoord coord = (VklAxisCoord)i;
        VklAxesTicks* ticks = &axes->ticks[i];
        VklAxesContext ctx = _axes_context(controller, coord);
        float scale = controller->interacts[0].u.p.zoom[i];
        ctx.size_viewport *= scale;

        // Check whether there are overlapping labels (dezooming).
        double o = overlap(ticks->format, ticks->lmin, ticks->lmax, ticks->lstep, ctx);

        // Check whether there are too few labels (zooming).
        double d = density(
            ticks->value_count, ticks->value_count_req, //
            ticks->dmin, ticks->dmax, ticks->lmin, ticks->lmax);

        // Check whether the current view is outside the computed ticks (panning);
        bool outside =
            axes->panzoom_range[i][0] <= ticks->lmin || axes->panzoom_range[i][1] >= ticks->lmax;

        // Recompute the ticks on the current axis?
        update[i] = o <= 0 || d <= .75 || outside;
    }
}

static void _axes_range(VklController* controller, VklAxisCoord coord)
{
    // Update axes->range struct as a function of the current panzoom

    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    // set axes->range depending on coord
    VklTransform tr = {0};
    dvec2 ll = {-1, -1};
    dvec2 ur = {+1, +1};
    dvec2 pos_ll = {0};
    dvec2 pos_ur = {0};
    tr = vkl_transform(controller->panel, VKL_CDS_PANZOOM, VKL_CDS_GPU);
    // TODO: transform to data coordinates instead of GPU coordinates.
    vkl_transform_apply(&tr, ll, pos_ll);
    vkl_transform_apply(&tr, ur, pos_ur);
    axes->panzoom_range[coord][0] = pos_ll[coord];
    axes->panzoom_range[coord][1] = pos_ur[coord];
    // log_info("%.3f %.3f", axes->range[coord][0], axes->range[coord][1]);
}

static void _axes_callback(VklController* controller, VklEvent ev)
{
    ASSERT(controller != NULL);
    _default_controller_callback(controller, ev);
    return;
    if (!controller->interacts[0].is_active)
        return;
    VklCanvas* canvas = controller->panel->grid->canvas;

    // Check label collision
    // DEBUG
    // bool update[2] = {true, true}; // whether X and Y axes must be updated or not
    bool update[2] = {false, false}; // whether X and Y axes must be updated or not
    _axes_collision(controller, update);

    log_debug("%d %d", update[0], update[1]);

    // TODO DEBUG
    return;

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        if (!update[coord])
            continue;
        _axes_range(controller, coord);
        _axes_ticks(controller, coord);
        _axes_upload(controller, coord);
        canvas->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    }
}

static void _add_axes(VklController* controller)
{
    ASSERT(controller != NULL);
    VklPanel* panel = controller->panel;
    ASSERT(panel != NULL);
    panel->controller = controller;
    VklContext* ctx = panel->grid->canvas->gpu->context;
    ASSERT(ctx != NULL);

    vkl_panel_margins(panel, (vec4){25, 25, 100, 100});

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        VklVisual* visual = vkl_scene_visual(panel, VKL_VISUAL_AXES_2D, (int)coord);
        vkl_controller_visual(controller, visual);
        visual->priority = VKL_MAX_VISUAL_PRIORITY;

        visual->clip[0] = VKL_VIEWPORT_OUTER;
        visual->clip[1] = coord == 0 ? VKL_VIEWPORT_OUTER_BOTTOM : VKL_VIEWPORT_OUTER_LEFT;

        visual->transform[0] = visual->transform[1] =
            coord == 0 ? VKL_TRANSFORM_AXIS_X : VKL_TRANSFORM_AXIS_Y;

        // Text params.
        VklFontAtlas* atlas = vkl_font_atlas(ctx);
        ASSERT(strlen(atlas->font_str) > 0);
        vkl_visual_texture(visual, VKL_SOURCE_TYPE_FONT_ATLAS, 1, atlas->texture);

        VklGraphicsTextParams params = {0};
        params.grid_size[0] = (int32_t)atlas->rows;
        params.grid_size[1] = (int32_t)atlas->cols;
        params.tex_size[0] = (int32_t)atlas->width;
        params.tex_size[1] = (int32_t)atlas->height;
        vkl_visual_data_buffer(visual, VKL_SOURCE_TYPE_PARAM, 1, 0, 1, 1, &params);
    }
    // Add the axes data.
    _axes_ticks_init(controller);
}

static void _axes_destroy(VklController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    for (uint32_t i = 0; i < 2; i++)
    {
        vkl_ticks_destroy(&axes->ticks[i]);
    }
}



#endif
