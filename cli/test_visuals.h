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
            visual, VKL_SOURCE_TYPE_VERTEX, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);



        // Binding #0: uniform buffer MVP
        vkl_visual_source( //
            visual, VKL_SOURCE_TYPE_MVP, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklMVP),
            VKL_SOURCE_FLAG_IMMEDIATE);

        // Binding #1: uniform buffer viewport
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_VIEWPORT, VKL_PIPELINE_GRAPHICS, 0, 1, sizeof(VklViewport), 0);

        // // Binding #2: color texture
        // vkl_visual_source( //
        //     visual, VKL_SOURCE_TEXTURE_2D, 0, VKL_PIPELINE_GRAPHICS, 0, 2, sizeof(cvec4), 0);

        // Binding #2: uniform buffer params
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_PARAM, VKL_PIPELINE_GRAPHICS, 0, 2,
            sizeof(VklGraphicsPointParams), 0);
    }

    // Props.
    {
        // Vertex pos.
        vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_VEC3, VKL_SOURCE_TYPE_VERTEX, 0);
        vkl_visual_prop_copy(
            visual, VKL_PROP_POS, 0, 0, offsetof(VklVertex, pos), VKL_ARRAY_COPY_SINGLE, 1);

        // Vertex color.
        vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
        vkl_visual_prop_copy(
            visual, VKL_PROP_COLOR, 0, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_SINGLE, 1);

        // MVP
        // Model.
        vkl_visual_prop(visual, VKL_PROP_MODEL, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
        vkl_visual_prop_copy(
            visual, VKL_PROP_MODEL, 0, 0, offsetof(VklMVP, model), VKL_ARRAY_COPY_SINGLE, 1);

        // View.
        vkl_visual_prop(visual, VKL_PROP_VIEW, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
        vkl_visual_prop_copy(
            visual, VKL_PROP_VIEW, 0, 1, offsetof(VklMVP, view), VKL_ARRAY_COPY_SINGLE, 1);

        // Proj.
        vkl_visual_prop(visual, VKL_PROP_PROJ, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
        vkl_visual_prop_copy(
            visual, VKL_PROP_PROJ, 0, 2, offsetof(VklMVP, proj), VKL_ARRAY_COPY_SINGLE, 1);



        // Param: marker size.
        vkl_visual_prop(
            visual, VKL_PROP_MARKER_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_PARAM, 0);
        vkl_visual_prop_copy(
            visual, VKL_PROP_MARKER_SIZE, 0, 0, offsetof(VklGraphicsPointParams, point_size),
            VKL_ARRAY_COPY_SINGLE, 1);


        // // Colormap texture.
        // vkl_visual_prop(
        //     visual, VKL_PROP_COLOR_TEXTURE, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TEXTURE_2D, 0);
        // vkl_visual_prop_copy(visual, VKL_PROP_COLOR_TEXTURE, 0, 0, 0, VKL_ARRAY_COPY_SINGLE, 1);
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
