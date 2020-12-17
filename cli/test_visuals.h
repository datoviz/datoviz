#ifndef VKY_TEST_VISUALS_HEADER
#define VKY_TEST_VISUALS_HEADER

#include "../include/visky/visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Visual example                                                                               */
/*************************************************************************************************/

static void _marker_visual(VklVisual* visual)
{
    VklCanvas* canvas = visual->canvas;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_POINTS, 0));

    // Sources.
    {
        // Vertex buffer.
        vkl_visual_source( //
            visual, VKL_SOURCE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);



        // Binding #0: uniform buffer MVP
        vkl_visual_source( //
            visual, VKL_SOURCE_UNIFORM, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklMVP),
            VKL_SOURCE_FLAG_IMMEDIATE);

        // Binding #1: uniform buffer viewport
        vkl_visual_source(
            visual, VKL_SOURCE_UNIFORM, 1, VKL_PIPELINE_GRAPHICS, 0, 1, sizeof(VklViewport), 0);

        // Binding #2: color texture
        vkl_visual_source( //
            visual, VKL_SOURCE_TEXTURE_2D, 0, VKL_PIPELINE_GRAPHICS, 0, 2, sizeof(cvec4), 0);

        // Binding #3: uniform buffer params
        vkl_visual_source(
            visual, VKL_SOURCE_UNIFORM, 2, VKL_PIPELINE_GRAPHICS, 0, 3,
            sizeof(VklGraphicsPointsParams), 0);
    }

    // Props.
    {
        // Vertex pos.
        vkl_visual_prop(                                   //
            visual, VKL_PROP_POS, 0, VKL_SOURCE_VERTEX, 0, //
            0, VKL_DTYPE_VEC3, offsetof(VklVertex, pos));  //

        // Vertex color.
        vkl_visual_prop(                                     //
            visual, VKL_PROP_COLOR, 0, VKL_SOURCE_VERTEX, 0, //
            1, VKL_DTYPE_CVEC4, offsetof(VklVertex, color)); //


        // MVP
        // Model.
        vkl_visual_prop(
            visual, VKL_PROP_MODEL, 0, VKL_SOURCE_UNIFORM, 0, //
            0, VKL_DTYPE_MAT4, offsetof(VklMVP, model));

        // View.
        vkl_visual_prop(
            visual, VKL_PROP_VIEW, 0, VKL_SOURCE_UNIFORM, 0, //
            1, VKL_DTYPE_MAT4, offsetof(VklMVP, view));

        // Proj.
        vkl_visual_prop(
            visual, VKL_PROP_PROJ, 0, VKL_SOURCE_UNIFORM, 0, //
            2, VKL_DTYPE_MAT4, offsetof(VklMVP, proj));



        // Param: marker size.
        vkl_visual_prop(
            visual, VKL_PROP_MARKER_SIZE, 0, VKL_SOURCE_UNIFORM, 2, //
            0, VKL_DTYPE_FLOAT, offsetof(VklGraphicsPointsParams, point_size));


        // Colormap texture.
        vkl_visual_prop(
            visual, VKL_PROP_COLOR_TEXTURE, 0, VKL_SOURCE_TEXTURE_2D, 0, //
            0, VKL_DTYPE_CVEC4, 0);
    }
}

static void _visual_canvas_fill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    VklVisual* visual = (VklVisual*)ev.user_data;
    VklViewport viewport = vkl_viewport_full(canvas);


    // TODO: choose which of all canvas command buffers need to be filled with the visual
    // For now, update all of them.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        vkl_visual_fill_begin(canvas, ev.u.rf.cmds[i], ev.u.rf.img_idx);
        vkl_cmd_viewport(ev.u.rf.cmds[i], ev.u.rf.img_idx, viewport.viewport);
        vkl_visual_fill_event(
            visual, ev.u.rf.clear_color, ev.u.rf.cmds[i], ev.u.rf.img_idx, viewport, NULL);
        vkl_visual_fill_end(canvas, ev.u.rf.cmds[i], ev.u.rf.img_idx);
    }
}



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_visuals_1(TestContext* context);
int test_visuals_2(TestContext* context);
int test_visuals_3(TestContext* context);



#endif
