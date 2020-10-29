#include <visky/demo_inc.h>
#include <visky/visky.h>



/*************************************************************************************************/
/*  Blank demo                                                                                   */
/*************************************************************************************************/

void vky_demo_blank()
{
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    vky_create_scene(canvas, (VkyColor){{128, 172, 172}, 255}, 1, 1);
    vky_run_app(app);
    vky_destroy_app(app);
}



/*************************************************************************************************/
/*  Raytracing demo                                                                              */
/*************************************************************************************************/

void vky_demo_raytracing()
{
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_FPS, NULL);
    vky_set_constant(VKY_PANZOOM_MIN_ZOOM_ID, 1);

    _demo_raytracing(panel);

    vky_run_app(app);
    vky_destroy_app(app);
}



/*************************************************************************************************/
/*  Param demo                                                                                   */
/*************************************************************************************************/

void vky_demo_param(vec4 clear_color)
{
    VkyColor color = {0};
    color.rgb[0] = TO_BYTE(clear_color[0]);
    color.rgb[1] = TO_BYTE(clear_color[1]);
    color.rgb[2] = TO_BYTE(clear_color[2]);
    color.alpha = TO_BYTE(clear_color[3]);

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    vky_create_scene(canvas, color, 1, 1);
    vky_run_app(app);
    vky_destroy_app(app);
}



/*************************************************************************************************/
/*  Scatter plot demo                                                                            */
/*************************************************************************************************/

void vky_demo_scatter(uint32_t point_count, const dvec2* points)
{
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, NULL);

    // Create the visual.
    VkyMarkersParams params = (VkyMarkersParams){{0, 0, 0, 1}, 1.0f, false};
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_MARKER, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    VkyMarkersVertex* data = calloc(point_count, sizeof(VkyMarkersVertex));
    for (uint32_t i = 0; i < point_count; i++)
    {
        data[i] = (VkyMarkersVertex){
            {(float)points[i][0], (float)points[i][1], 0},
            vky_color(VKY_CMAP_VIRIDIS, i, 0, point_count, 1),
            10.0f,
            VKY_MARKER_DISC,
            0};
    }
    visual->data.item_count = point_count;
    visual->data.items = data;
    vky_visual_data_raw(visual);
    FREE(data);

    vky_run_app(app);
    vky_destroy_app(app);
}
