#include "../include/visky/basic.h"
#include "../include/visky/visuals.h"


/*************************************************************************************************/
/*  Static variables                                                                             */
/*************************************************************************************************/

static VkyApp* app = NULL;
static VkyCanvas* canvas = NULL;
static VkyScene* scene = NULL;
static VkyPanel* panel = NULL;



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VkyPanel* _make_panel()
{
    if (app == NULL)
    {
        log_set_level_env();
        app = vky_create_app(VKY_DEFAULT_BACKEND);
    }
    if (canvas == NULL)
        canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    if (scene == NULL)
        scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);
    if (panel == NULL)
        panel = vky_get_panel(scene, 0, 0);
    return panel;
}



/*************************************************************************************************/
/*  Basic API                                                                                    */
/*************************************************************************************************/

VkyPanel* vky_basic_panel() { return panel; }



VkyVisual* vky_basic_visual(
    VkyControllerType controller_type, VkyVisualType visual_type, //
    uint32_t item_count, vec3* pos, VkyColor* color)
{
    _make_panel();
    vky_set_controller(panel, controller_type, NULL);
    VkyVisual* visual = vky_visual(scene, visual_type, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    if (item_count > 0)
        vky_visual_data_set_size(visual, item_count, 0, NULL, NULL);
    if (pos != NULL)
        vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 0, item_count, pos);
    if (color != NULL)
        vky_visual_data(visual, VKY_VISUAL_PROP_COLOR, 0, item_count, color);

    return visual;
}



VkyVisual* vky_basic_scatter(uint32_t item_count, vec3* pos, VkyColor* color, float* size)
{
    VkyVisual* visual =
        vky_basic_visual(VKY_CONTROLLER_AXES_2D, VKY_VISUAL_MARKER, item_count, pos, color);

    VkyMarkersParams params = {0};
    params.edge_color[3] = 1;
    params.edge_width = 1;
    vky_visual_params(visual, sizeof(VkyMarkersParams), &params);

    vky_visual_data(visual, VKY_VISUAL_PROP_SIZE, 0, item_count, size);

    VkyMarkerType marker = VKY_MARKER_DISC;
    vky_visual_data(visual, VKY_VISUAL_PROP_SHAPE, 0, 1, &marker);

    return visual;
}



VkyVisual* vky_basic_plot(uint32_t item_count, vec3* pos, VkyColor* color)
{
    VkyVisual* visual =
        vky_basic_visual(VKY_CONTROLLER_AXES_2D, VKY_VISUAL_PATH, item_count, pos, color);

    VkyPathParams params = {0};
    params.cap_type = VKY_CAP_ROUND;
    params.miter_limit = 4;
    params.round_join = VKY_JOIN_ROUND;
    params.linewidth = 5;
    vky_visual_params(visual, sizeof(VkyPathParams), &params);

    return visual;
}



VkyVisual* vky_basic_segments(uint32_t item_count, vec3* pos, VkyColor* color)
{
    return vky_basic_visual(VKY_CONTROLLER_AXES_2D, VKY_VISUAL_SEGMENT, item_count, pos, color);
}



VkyVisual* vky_basic_imshow(uint32_t width, uint32_t height, const uint8_t* pixels)
{
    _make_panel();
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    VkyTextureParams params = vky_default_texture_params(width, height, 1);
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_IMAGE, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    vky_visual_data_set_size(visual, 1, 0, NULL, NULL);
    vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 0, 1, (vec3){-1, -1, 0});
    vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 1, 1, (vec3){+1, +1, 0});
    vky_visual_data(visual, VKY_VISUAL_PROP_TEXTURE_COORDS, 0, 1, (vec2){0, 0});
    vky_visual_data(visual, VKY_VISUAL_PROP_TEXTURE_COORDS, 1, 1, (vec2){1, 1});

    vky_visual_image_upload(visual, (const uint8_t*)pixels);

    return visual;
}



VkyVisual* vky_basic_mesh(uint32_t item_count, vec3* pos, VkyColor* color)
{
    return vky_basic_visual(VKY_CONTROLLER_ARCBALL, VKY_VISUAL_MESH_RAW, item_count, pos, color);
}



void vky_basic_run()
{
    vky_run_app(app);
    vky_destroy_app(app);
}
