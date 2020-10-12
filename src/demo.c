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

static void _raytracing_callback(VkyCanvas* canvas)
{
    double t = canvas->local_time;
    VkyCamera* camera = (VkyCamera*)canvas->scene->grid->panels[0].controller;
    vec3 axis = {0, -1, 0};
    glm_rotate_make(camera->mvp.model, 2 * t, axis);
}

void raytracing_demo(VkyPanel* panel)
{
    VkyCanvas* canvas = panel->scene->canvas;
    VkyScene* scene = panel->scene;

    // Create the visual.
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_CUSTOM);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "raytracing.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "raytracing.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(vec3));
    vky_add_vertex_attribute(&vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

    // Resource layout.
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_uniform_buffer_resource(visual, &scene->grid->dynamic_buffer);

    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    vec3 data[6] = {
        {-1, -1, 0}, {+1, -1, 0}, {-1, +1, 0}, {-1, +1, 0}, {+1, -1, 0}, {+1, +1, 0},
    };
    vky_visual_data_raw(visual, (VkyData){6, data});

    vky_add_frame_callback(canvas, _raytracing_callback);
}

void vky_demo_raytracing()
{
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_FPS, NULL);
    vky_set_constant(VKY_PANZOOM_MIN_ZOOM_ID, 1);

    raytracing_demo(panel);

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

void vky_demo_scatter(size_t point_count, const dvec2* points)
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
    vky_visual_data_raw(visual, (VkyData){point_count, data});
    free(data);

    vky_run_app(app);
    vky_destroy_app(app);
}
