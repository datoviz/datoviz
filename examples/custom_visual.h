/*************************************************************************************************/
/*  Example of a custom visual.                                                                  */
/*************************************************************************************************/

// // NOTE: ignore this.
// #ifndef SCREENSHOT
// #define SCREENSHOT
// #endif
// #ifndef NFRAMES
// #define NFRAMES 0
// #endif



// We include the library header file.
#include <datoviz/datoviz.h>



// Baking callback function.
static void _custom_visual_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // First, we obtain the array instances holding the prop data as specified by the user.
    DvzArray* arr_pos = dvz_prop_array(visual, DVZ_PROP_POS, 0);
    DvzArray* arr_color = dvz_prop_array(visual, DVZ_PROP_COLOR, 0);
    DvzArray* arr_length = dvz_prop_array(visual, DVZ_PROP_LENGTH, 0);

    // We also get the array of the vertex buffer, which we'll need to fill with the triangulation.
    DvzArray* arr_vertex = dvz_source_array(visual, DVZ_SOURCE_TYPE_VERTEX, 0);

    // The number of rows in the 1D position array (set by the user) is the number of squares
    // requested by the user.
    uint32_t square_count = arr_pos->item_count;

    // We resize the vertex buffer array so that it holds six vertices per square (two triangles).
    dvz_array_resize(arr_vertex, 6 * square_count);

    // Pointers to the input data.
    dvec3* pos = NULL;
    cvec4* color = NULL;
    float* length = NULL;

    // Pointer to the output vertex.
    DvzVertex* vertex = (DvzVertex*)arr_vertex->data;

    // Here, we triangulate each square by computing the position of each square corner.
    float hl = 0;
    for (uint32_t i = 0; i < square_count; i++)
    {
        // We get a pointer to the current item in each prop array.
        pos = dvz_array_item(arr_pos, i);
        color = dvz_array_item(arr_color, i);
        length = dvz_array_item(arr_length, i);

        // This is the half of the square size.
        hl = (*length) / 2;

        // First triangle:

        // Bottom-left corner.
        vertex[6 * i + 0].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 0].pos[1] = pos[0][1] - hl;

        // Bottom-right corner.
        vertex[6 * i + 1].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 1].pos[1] = pos[0][1] - hl;

        // Top-right corner.
        vertex[6 * i + 2].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 2].pos[1] = pos[0][1] + hl;

        // Second triangle:

        // Top-right corner again.
        vertex[6 * i + 3].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 3].pos[1] = pos[0][1] + hl;

        // Top-left corner.
        vertex[6 * i + 4].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 4].pos[1] = pos[0][1] + hl;

        // Bottom-left corner (again).
        vertex[6 * i + 5].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 5].pos[1] = pos[0][1] - hl;

        // We copy the square color to each of the six vertices making the current square.
        // This is a choice made in this example, and it is up to the custom visual creator
        // to define how the user data, passed via props, will be used to fill in the vertices.
        for (uint32_t j = 0; j < 6; j++)
            memcpy(vertex[6 * i + j].color, color, sizeof(cvec4));
    }
}



static int demo_custom_visual()
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);
    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    // Create a blank visual.
    DvzVisual* visual = dvz_blank_visual(scene, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);

    // Custom visual.
    {
        // Add an existing graphics pipeline to it.
        dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE, 0));

        // Add the vertex buffer source.
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);

        // Add some props.
        dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);

        // Custom baking functions.
        dvz_visual_callback_bake(visual, _custom_visual_bake);
    }

    // Add the custom visual to the main panel.
    dvz_custom_visual(panel, visual);

    // Set the visual data.
    {
        // Three squares.
        dvz_visual_data(
            visual, DVZ_PROP_POS, 0, 3, (dvec3[]){{-.5, 0, 0}, {0, 0, 0}, {+.5, 0, 0}});

        // Three different colors.
        dvz_visual_data(
            visual, DVZ_PROP_COLOR, 0, 3,
            (cvec4[]){{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}});

        // The same length (automatic "broadcasting" when the prop size is low)
        dvz_visual_data(visual, DVZ_PROP_LENGTH, 0, 1, (float[]){.25});
    }

    // SCREENSHOT
    dvz_app_run(app, 0);

    dvz_app_destroy(app);
    return 0;
}
