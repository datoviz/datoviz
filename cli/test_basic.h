static int red_canvas(VkyCanvas* canvas)
{
    vky_clear_color(canvas->scene, (VkyColor){{255, 0, 0}, 255});
    return 0;
}

static int blue_canvas(VkyCanvas* canvas)
{
    vky_clear_color(canvas->scene, (VkyColor){{0, 0, 255}, 255});
    return 0;
}

static int hello(VkyCanvas* canvas)
{
    vky_clear_color(canvas->scene, VKY_CLEAR_COLOR_BLACK);
    VkyPanel* panel = vky_get_panel(canvas->scene, 0, 0);
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_TEXT, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    char* str = "Hello world!";
    uint32_t n = strlen(str);

    vec3 pos = {0, 0, 0};
    VkyColor color = {{255, 0, 0}, 255};
    float size = 30;

    vky_visual_data_set_size(visual, n, 1, (uint32_t[]){n}, NULL);
    vky_visual_data_group(visual, VKY_VISUAL_PROP_POS_GPU, 0, 0, 1, pos);
    vky_visual_data_group(visual, VKY_VISUAL_PROP_COLOR, 0, 0, 1, &color);
    vky_visual_data_group(visual, VKY_VISUAL_PROP_SIZE, 0, 0, 1, &size);
    vky_visual_data(visual, VKY_VISUAL_PROP_TEXT, 0, n, (void*)str);

    return 0;
}

static int triangle(VkyCanvas* canvas)
{
    vky_clear_color(canvas->scene, VKY_CLEAR_COLOR_BLACK);
    VkyPanel* panel = vky_get_panel(canvas->scene, 0, 0);

    // Create the visual.
    VkyVisual* visual = vky_create_visual(panel->scene, VKY_VISUAL_CUSTOM);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "default.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "default.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyDefaultVertex));

    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyDefaultVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VkyDefaultVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_uniform_buffer_resource(visual, &panel->scene->grid->dynamic_buffer);

    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Make the data and upload it to the GPU.
    VkyDefaultVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}}, // vec3 pos, vec4 color
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };

    visual->data.item_count = 3;
    visual->data.items = data;
    vky_visual_data_raw(visual);

    return 0;
}
