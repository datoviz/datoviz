#ifndef DVZ_TEST_VISUALS_HEADER
#define DVZ_TEST_VISUALS_HEADER

#include "../include/datoviz/visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Visual example                                                                               */
/*************************************************************************************************/

static void _marker_visual(DvzVisual* visual)
{
    DvzCanvas* canvas = visual->canvas;
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_POINT, 0));

    // Sources.
    {
        // Vertex buffer.
        dvz_visual_source( //
            visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);



        // Binding #0: uniform buffer MVP
        dvz_visual_source( //
            visual, DVZ_SOURCE_TYPE_MVP, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzMVP),
            DVZ_SOURCE_FLAG_MAPPABLE);

        // Binding #1: uniform buffer viewport
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, DVZ_PIPELINE_GRAPHICS, 0, 1, sizeof(DvzViewport),
            0);

        // Binding #2: uniform buffer params
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, 2,
            sizeof(DvzGraphicsPointParams), 0);
    }

    // Props.
    {
        // Vertex pos.
        prop =
            dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop_cast(
            prop, 0, offsetof(DvzVertex, pos),
            DVZ_DTYPE_VEC3, // NOTE: cast to float
            DVZ_ARRAY_COPY_SINGLE, 1);

        // Vertex color.
        prop =
            dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_SINGLE, 1);

        // MVP
        // Model.
        prop = dvz_visual_prop(visual, DVZ_PROP_MODEL, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
        dvz_visual_prop_copy(prop, 0, offsetof(DvzMVP, model), DVZ_ARRAY_COPY_SINGLE, 1);

        // View.
        prop = dvz_visual_prop(visual, DVZ_PROP_VIEW, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
        dvz_visual_prop_copy(prop, 1, offsetof(DvzMVP, view), DVZ_ARRAY_COPY_SINGLE, 1);

        // Proj.
        prop = dvz_visual_prop(visual, DVZ_PROP_PROJ, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
        dvz_visual_prop_copy(prop, 2, offsetof(DvzMVP, proj), DVZ_ARRAY_COPY_SINGLE, 1);



        // Param: marker size.
        prop = dvz_visual_prop(
            visual, DVZ_PROP_MARKER_SIZE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
        dvz_visual_prop_copy(
            prop, 0, offsetof(DvzGraphicsPointParams, point_size), DVZ_ARRAY_COPY_SINGLE, 1);
    }
}

static void _visual_canvas_fill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzVisual* visual = (DvzVisual*)ev.user_data;

    // TODO: choose which of all canvas command buffers need to be filled with the visual
    // For now, update all of them.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        dvz_visual_fill_begin(canvas, ev.u.rf.cmds[i], ev.u.rf.img_idx);
        dvz_cmd_viewport(ev.u.rf.cmds[i], ev.u.rf.img_idx, canvas->viewport.viewport);
        dvz_visual_fill_event(
            visual, ev.u.rf.clear_color, ev.u.rf.cmds[i], ev.u.rf.img_idx, canvas->viewport, NULL);
        dvz_visual_fill_end(canvas, ev.u.rf.cmds[i], ev.u.rf.img_idx);
    }
}



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_visuals_1(TestContext* context);
int test_visuals_2(TestContext* context);
int test_visuals_3(TestContext* context);
int test_visuals_4(TestContext* context);
int test_visuals_5(TestContext* context);



#endif
