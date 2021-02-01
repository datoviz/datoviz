/*************************************************************************************************/
/*  Example of a custom visual.                                                                  */
/*************************************************************************************************/

#include <datoviz/datoviz.h>


static void _bake_callback(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // Get prop arrays, to be read.
    DvzArray* arr_pos = dvz_prop_array(visual, DVZ_PROP_POS, 0);
    DvzArray* arr_length = dvz_prop_array(visual, DVZ_PROP_LENGTH, 0);
    DvzArray* arr_color = dvz_prop_array(visual, DVZ_PROP_COLOR, 0);

    // Get the vertex buffer array, to be written.
    DvzArray* arr_vertex = dvz_source_array(visual, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Rectangle triangulation.
    uint32_t rectangle_count = arr_pos->item_count;
    dvz_array_resize(arr_vertex, 6 * rectangle_count);

    // Input and output arrays.
    dvec3* pos = NULL;    //(dvec3*)arr_pos->data;
    float* length = NULL; //(float*)arr_length->data;
    cvec4* color = NULL;  //(cvec4*)arr_color->data;
    DvzVertex* vertex = (DvzVertex*)arr_vertex->data;

    // Triangulate the squares.
    float hl = 0;
    for (uint32_t i = 0; i < rectangle_count; i++)
    {
        pos = dvz_array_item(arr_pos, i);
        color = dvz_array_item(arr_color, i);
        length = dvz_array_item(arr_length, i);

        hl = (*length) / 2;

        vertex[6 * i + 0].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 0].pos[1] = pos[0][1] - hl;

        vertex[6 * i + 1].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 1].pos[1] = pos[0][1] - hl;

        vertex[6 * i + 2].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 2].pos[1] = pos[0][1] + hl;

        vertex[6 * i + 3].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 3].pos[1] = pos[0][1] + hl;

        vertex[6 * i + 4].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 4].pos[1] = pos[0][1] + hl;

        vertex[6 * i + 5].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 5].pos[1] = pos[0][1] - hl;

        for (uint32_t j = 0; j < 6; j++)
            memcpy(vertex[6 * i + j].color, color, sizeof(cvec4));
    }
}


int main(int argc, char** argv)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);
    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    // Create a blank visual.
    DvzVisual* visual = dvz_scene_visual_blank(scene, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);

    // Custom visual.
    {
        // Add an existing graphics pipeline to it.
        dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE, 0));

        // Add the vertex buffer source.
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);

        // Add some props.
        dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);

        // Custom baking functions.
        dvz_visual_callback_bake(visual, _bake_callback);
    }

    // Add the custom visual to the main panel.
    dvz_scene_visual_custom(panel, visual);

    // Three rectangles.
    dvz_visual_data(visual, DVZ_PROP_POS, 0, 3, (dvec3[]){{-.5, 0, 0}, {0, 0, 0}, {+.5, 0, 0}});
    // Three different colors.
    dvz_visual_data(
        visual, DVZ_PROP_COLOR, 0, 3,
        (cvec4[]){{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}});
    // The same length (automatic "broadcasting" when the prop size is low)
    dvz_visual_data(visual, DVZ_PROP_LENGTH, 0, 1, (float[]){.25});

    dvz_app_run(app, 0);

    dvz_app_destroy(app);
    return 0;
}
