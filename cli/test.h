#define N_NUMBERS 20



/*************************************************************************************************/
/*  Test vklite                                                                                 */
/*************************************************************************************************/

// Resources used in tests below.
static VkyBufferRegion vbr;
static VkyGraphicsPipeline pipeline;
static VkyBuffer buffer;

static int no_destroy(VkyTestContext* context) { return 0; }

static int vklite_compute(VkyTestContext* context)
{
    // VkyGpu gpu = vky_create_device(0, NULL);
    // vky_prepare_gpu(&gpu, NULL);
    VkyGpu gpu = *context->app->gpu;

    VkDeviceSize size = N_NUMBERS * sizeof(float);
    buffer = vky_create_buffer(
        &gpu, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkyBufferRegion buffer_region = vky_allocate_buffer(&buffer, size);

    float numbers[N_NUMBERS];
    for (uint32_t i = 0; i < N_NUMBERS; i++)
    {
        numbers[i] = (float)i + 1;
    }
    vky_upload_buffer(buffer_region, 0, size, numbers);

    VkyResourceLayout resource_layout = vky_create_resource_layout(&gpu, 1);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    VkyComputePipeline cpipeline =
        vky_create_compute_pipeline(&gpu, "pow2.comp.spv", resource_layout);

    VkyBufferRegion* resources[] = {&buffer_region};
    vky_bind_resources(cpipeline.resource_layout, cpipeline.descriptor_sets, (void**)resources);

    vky_begin_compute(&gpu);
    vky_compute(&cpipeline, N_NUMBERS, 1, 1);
    vky_end_compute(&gpu, 0, NULL, NULL);
    vky_compute_submit(&gpu);
    vky_compute_wait(&gpu);

    vky_download_buffer(&buffer_region, numbers);

    for (uint32_t i = 0; i < N_NUMBERS; i++)
    {
        if (numbers[i] != 2 * (i + 1))
            return 1;
    }

    vky_destroy_compute_pipeline(&cpipeline);
    vky_destroy_buffer(&buffer);
    // vky_destroy_device(&gpu);

    return 0;
}

static void _blank_fill(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);
    vky_end_render_pass(cmd_buf, canvas);
}

static int vklite_blank(VkyTestContext* context)
{
    // log_set_level_env();
    context->canvas->cb_fill_command_buffer = _blank_fill;
    // vky_run_app(canvas->app);
    // vky_destroy_app(app);
    return 0;
}



static void _triangle_fill(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);
    vky_bind_vertex_buffer(cmd_buf, vbr, 0);
    vky_bind_graphics_pipeline(cmd_buf, &pipeline);
    vky_set_viewport(
        cmd_buf, 0, 0, canvas->size.framebuffer_width, canvas->size.framebuffer_height);
    vky_draw(cmd_buf, 0, 3);
    vky_end_render_pass(cmd_buf, canvas);
}

static int vklite_triangle(VkyTestContext* context)
{
    VkyCanvas* canvas = context->canvas;
    canvas->cb_fill_command_buffer = _triangle_fill;

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
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);

    // Create the graphics pipeline.
    pipeline = vky_create_graphics_pipeline(
        canvas,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // triangles, 3 vertices per primitive
        shaders, vertex_layout, resource_layout, (VkyGraphicsPipelineParams){true});

    // Create the vertex buffer.
    VkDeviceSize size = 3 * sizeof(VkyDefaultVertex); // 3 vertices
    buffer = vky_create_buffer(canvas->gpu, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
    vbr = vky_allocate_buffer(&buffer, size);

    // Make the data and upload it to the GPU.
    VkyDefaultVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}}, // vec3 pos, vec4 color
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vky_upload_buffer(vbr, 0, size, data);
    return 0;
}

static int vklite_triangle_destroy(VkyTestContext* context)
{
    vky_destroy_buffer(&buffer);
    vky_destroy_vertex_layout(&pipeline.vertex_layout);
    vky_destroy_resource_layout(&pipeline.resource_layout);
    vky_destroy_shaders(&pipeline.shaders);
    vky_destroy_graphics_pipeline(&pipeline);

    return 0;
}



static void _push_fill(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    // Begin the render pass.
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);

    // Bind the vertex buffer.
    vky_bind_vertex_buffer(cmd_buf, vbr, 0);

    // Bind the graphics pipeline.
    vky_bind_graphics_pipeline(cmd_buf, &pipeline);

    // Set the full viewport.
    vky_set_viewport(
        cmd_buf, 0, 0, canvas->size.framebuffer_width, canvas->size.framebuffer_height);

    // Push constants.
    vky_push_constants(cmd_buf, &pipeline, sizeof(vec2), (vec2){1, 1});

    // Draw 3 vertices = 1 triangle.
    vky_draw(cmd_buf, 0, 3);

    // End the render pass.
    vky_end_render_pass(cmd_buf, canvas);
}

static int vklite_push(VkyTestContext* context)
{
    VkyCanvas* canvas = context->canvas;

    // This callback function is called when the command buffers need to be recreated,
    // at initialization and resize.
    canvas->cb_fill_command_buffer = _push_fill;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "push.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "push.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyDefaultVertex));

    // GLSL: layout (location = 0) in vec3 pos;
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyDefaultVertex, pos));

    // GLSL: layout (location = 1) in vec4 color;
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VkyDefaultVertex, color));

    // Resource layout. None here, but one could add uniform buffers, textures...
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);

    // Create the graphics pipeline.
    pipeline = vky_create_graphics_pipeline(
        canvas,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // triangles, 3 vertices per primitive
        shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){
            true,        // depth test
            sizeof(vec2) // push constant size, 0 if no push constants
        });

    // Create the vertex buffer.
    VkDeviceSize size = 3 * sizeof(VkyDefaultVertex); // 3 vertices
    // Typically one uses one giant vertex buffer for the whole application and allocates parts of
    // the buffer to different visuals.
    buffer = vky_create_buffer(canvas->gpu, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
    vbr = vky_allocate_buffer(&buffer, size);

    // Make the data and upload it to the GPU.
    VkyDefaultVertex data[3] = {
        {{-1, +1, 0}, {0, 0, 0, 1}}, // vec3 pos, vec4 color
        {{+1, +1, 0}, {0, 0, 0, 1}}, // NOTE: will be filled by the push constant
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vky_upload_buffer(vbr, 0, size, data);

    return 0;
}

static int vklite_push_destroy(VkyTestContext* context)
{
    // Destroy the resources.
    vky_destroy_buffer(&buffer);
    vky_destroy_vertex_layout(&pipeline.vertex_layout);
    vky_destroy_resource_layout(&pipeline.resource_layout);
    vky_destroy_shaders(&pipeline.shaders);
    vky_destroy_graphics_pipeline(&pipeline);

    return 0;
}



/*************************************************************************************************/
/*  Visual props tests                                                                           */
/*************************************************************************************************/

static VkyVisual _blank_visual()
{
    VkyVisual v = {0};
    v.props = calloc(10, sizeof(VkyVisualProp));
    v.children = calloc(10, sizeof(VkyVisual*));
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS, 0);
    return v;
}

static void _destroy_visual(VkyVisual* v)
{
    FREE(v->children);
    FREE(v->props);
}

static int visuals_props_1(VkyTestContext* context)
{
    VkyVisual v = _blank_visual();

    VkyVisualProp* vp = NULL;
    VkyVisualProp* vpc = NULL;

    // Add a prop to the visual.
    vp = vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS_GPU, 4);

    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_COLOR, 0) == NULL);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 0) != NULL);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 1) == NULL);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS_GPU, 0) == vp);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS_GPU, 1) == NULL);

    // Add a child.
    v.children_count = 1;
    VkyVisual vc = _blank_visual();
    v.children[0] = &vc;

    // Add a prop to the child visual.
    vpc = vky_visual_prop_add(&vc, VKY_VISUAL_PROP_POS_GPU, 4);

    // Check that we get the correct prop.
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS_GPU, 0) == vp);
    AT(vky_visual_prop_get(&vc, VKY_VISUAL_PROP_POS_GPU, 0) == vpc);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS_GPU, 1) == NULL);

    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 0) != NULL);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 1) == NULL);
    AT(vky_visual_prop_get(&vc, VKY_VISUAL_PROP_POS, 0) != NULL);
    AT(vky_visual_prop_get(&vc, VKY_VISUAL_PROP_POS, 1) == NULL);

    _destroy_visual(&vc);
    _destroy_visual(&v);

    return 0;
}

static int visuals_props_2(VkyTestContext* context)
{
    VkyVisual v = _blank_visual();
    vky_visual_prop_spec(&v, 6, 0);
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS_GPU, 0); // 1 byte
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_COLOR, 1);   // 2 bytes
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_SIZE, 3);    // 3 bytes

    uint8_t val1[] = {10, 20, 30};
    cvec2 val2[] = {{11, 12}, {13, 14}, {15, 16}};
    // Only one cvec3 item here, but should be copied over automatically
    cvec3 val3[] = {{21, 22, 23}};

    vky_visual_data_set_size(&v, 3, 0, NULL, NULL);
    vky_visual_data(&v, VKY_VISUAL_PROP_POS_GPU, 0, 3, val1);
    AT(v.data.item_count == 3)
    vky_visual_data(&v, VKY_VISUAL_PROP_COLOR, 0, 3, val2);
    vky_visual_data(&v, VKY_VISUAL_PROP_SIZE, 0, 1, val3);
    AT(v.data.item_count == 3)

    uint8_t expected[] = {10, 11, 12, 21, 22, 23, 20, 13, 14, 21, 22, 23, 30, 15, 16, 21, 22, 23};
    AT(memcmp(v.data.items, expected, sizeof(expected)) == 0);
    return 0;
}

static int visuals_props_3(VkyTestContext* context)
{
    VkyVisual v = _blank_visual();
    vky_visual_prop_spec(&v, 3, 0);
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS_GPU, 0); // 1 byte
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_COLOR, 1);   // 2 bytes

    uint8_t val1[] = {10};
    cvec2 val2[] = {{11, 12}, {13, 14}};


    vky_visual_data_set_size(&v, 1, 0, NULL, NULL);
    AT(v.data.item_count == 1)

    vky_visual_data(&v, VKY_VISUAL_PROP_POS_GPU, 0, 1, val1);
    AT(memcmp(v.data.items, (uint8_t[]){10, 0, 0}, 3) == 0);

    vky_visual_data(&v, VKY_VISUAL_PROP_COLOR, 0, 1, val2);
    AT(memcmp(v.data.items, (uint8_t[]){10, 11, 12}, 3) == 0);


    // Test the case where a subsequent call to vky_visual_data() increases the number
    // of data items, and causes the library to enlarge the array and copy over the last item value
    vky_visual_data_set_size(&v, 2, 0, NULL, NULL);
    AT(v.data.item_count == 2)

    AT(memcmp(v.data.items, (uint8_t[]){10, 11, 12, 0, 0, 0}, 6) == 0);

    vky_visual_data(&v, VKY_VISUAL_PROP_COLOR, 0, 2, val2);
    AT(memcmp(v.data.items, (uint8_t[]){10, 11, 12, 0, 13, 14}, 6) == 0);

    return 0;
}

static int visuals_props_4(VkyTestContext* context)
{
    VkyVisual v = _blank_visual();
    vky_visual_prop_spec(&v, 3, 0);
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS_GPU, 0); // 1 byte

    uint8_t val1[] = {10, 11, 12};
    uint8_t val2[] = {20};

    vky_visual_data_set_size(&v, 3, 0, NULL, NULL);
    vky_visual_data(&v, VKY_VISUAL_PROP_POS_GPU, 0, 3, val1);
    AT(v.data.item_count == 3)

    // Test the case where a subsequent call to vky_visual_data() decreases the number
    // of data items.
    vky_visual_data_set_size(&v, 1, 0, NULL, NULL);
    vky_visual_data(&v, VKY_VISUAL_PROP_POS_GPU, 0, 1, val2);
    AT(v.data.item_count == 1)

    uint8_t expected[] = {20};
    AT(memcmp(v.data.items, expected, sizeof(expected)) == 0);

    return 0;
}

static int visuals_props_5(VkyTestContext* context)
{
    VkyVisual v = _blank_visual();
    vky_visual_prop_spec(&v, 4, 0);
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS_GPU, 0); // 1 byte
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_COLOR, 1);   // 1 byte
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_SIZE, 2);    // 2 bytes

    vky_visual_data_set_size(&v, 6, 3, (uint32_t[]){1, 2, 3}, NULL);
    AT(v.data.item_count == 6);

    uint8_t x = 10;
    vky_visual_data_partial(&v, VKY_VISUAL_PROP_POS_GPU, 0, 3, 2, 1, &x);

    x = 11;
    vky_visual_data_group(&v, VKY_VISUAL_PROP_POS_GPU, 0, 1, 1, &x);

    uint8_t y[] = {20, 21};
    vky_visual_data(&v, VKY_VISUAL_PROP_COLOR, 0, 2, y);

    vky_visual_data_group(&v, VKY_VISUAL_PROP_SIZE, 0, 2, 1, y);

    uint8_t expected[] = {0,  20, 0,  0,  11, 21, 0,  0,  11, 21, 0,  0,
                          10, 21, 20, 21, 10, 21, 20, 21, 0,  21, 20, 21};
    AT(memcmp(v.data.items, expected, 6 * 4) == 0);

    return 0;
}

static int visuals_props_6(VkyTestContext* context)
{
    // Root empty visual with a POS prop, used for renormalizing the POS_GPU props of the child.
    VkyVisual v = _blank_visual();

    // Add a prop to the visual.
    VkyVisualProp* vp = vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS, 0);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 0) != NULL);

    // Add a child.
    v.children_count = 1;
    VkyVisual vc = _blank_visual();
    vky_visual_prop_spec(&vc, sizeof(vec3), 0);
    v.children[0] = &vc;

    // Add a prop to the child visual.
    VkyVisualProp* vpc = vky_visual_prop_add(&vc, VKY_VISUAL_PROP_POS, 0);
    vky_visual_prop_add(&vc, VKY_VISUAL_PROP_POS_GPU, 0);

    // Set data.
    vky_visual_data_set_size(&v, 2, 0, NULL, NULL);
    dvec2 positions[2] = {{10, 11}, {20, 21}};
    vky_visual_data(&v, VKY_VISUAL_PROP_POS, 0, 2, positions);

    AT(v.data.item_count == 2);
    AT(v.data.items == NULL);
    AT(vc.data.item_count == 2);
    AT(vc.data.items != NULL);

    AT(vp->values != NULL);
    AT(vpc->values != NULL);

    return 0;
}



/*************************************************************************************************/
/*  Transform tests                                                                              */
/*************************************************************************************************/

static int _check_axes_range(VkyAxes* axes, double xmin, double ymin, double xmax, double ymax)
{
    // Check set range/get range round trip for both bool values of recompute_ticks, and twice
    // each time.
    for (uint8_t b = 0; b < 2; b++)
    {
        vky_axes_reset(axes);

        for (uint32_t i = 0; i < 2; i++)
        {

            VkyBox2D box = (VkyBox2D){{xmin, ymin}, {xmax, ymax}};
            vky_axes_set_range(axes, box, b);
            VkyBox2D box_ = vky_axes_get_range(axes);
            ABOX(box_, xmin, ymin, xmax, ymax);
        }
    }
    return 0;
}

static void _set_axes_1(VkyPanel* panel)
{
    VkyAxes2DParams params = vky_default_axes_2D_params();
    glm_vec4_scale(params.margins, 0.25, params.margins);
    params.xscale.vmin = 0;
    params.xscale.vmax = 1000;
    params.yscale.vmin = -12;
    params.yscale.vmax = +12;
    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, &params);
}

static int transform_1(VkyTestContext* context)
{
    VkyAxesTransform tr = {{2, .5}, {-1, 1}};
    dvec2 p0 = {0, 0};
    dvec2 p1 = {1, 2};
    dvec2 pout = {0, 0};
    dvec2 pout2 = {0, 0};

    vky_axes_transform_apply(&tr, p0, pout);
    AT(pout[0] == 2);
    AT(pout[1] == -.5);

    vky_axes_transform_apply(&tr, p1, pout);
    AT(pout[0] == 4);
    AT(pout[1] == .5);

    // Inverse.
    VkyAxesTransform tri = vky_axes_transform_inv(tr);
    vky_axes_transform_apply(&tri, pout, pout2);
    AT(pout2[0] == p1[0]);
    AT(pout2[1] == p1[1]);
    AT(pout2[0] == 1);
    AT(pout2[1] == 2);

    VkyAxesTransform trm = vky_axes_transform_mul(tr, tri);
    AT(trm.scale[0] == 1);
    AT(trm.scale[1] == 1);
    AT(trm.shift[0] == 0);
    AT(trm.shift[1] == 0);

    trm = vky_axes_transform_mul(tri, tr);
    AT(trm.scale[0] == 1);
    AT(trm.scale[1] == 1);
    AT(trm.shift[0] == 0);
    AT(trm.shift[1] == 0);

    // Interpolation.
    tr = vky_axes_transform_interp(
        (dvec2){0, 0}, (dvec2){-1, -1}, (dvec2){100, 1000}, (dvec2){1, 1});

    vky_axes_transform_apply(&tr, (dvec2){0, 0}, pout);
    AT(pout[0] == -1);
    AT(pout[1] == -1);

    vky_axes_transform_apply(&tr, (dvec2){100, 1000}, pout);
    AT(pout[0] == 1);
    AT(pout[1] == 1);

    return 0;
}

static int transform_2(VkyTestContext* context)
{
    ASSERT(context->scene != NULL);

    vky_reset_grid(context->scene, 2, 4);
    VkyPanel* panel = vky_get_panel(context->scene, 1, 2);

    VkyAxes2DParams params = vky_default_axes_2D_params();
    glm_vec4_scale(params.margins, 0.25, params.margins);
    params.xscale.vmin = 0;
    params.xscale.vmax = 1000;
    params.yscale.vmin = -12;
    params.yscale.vmax = +12;

    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, &params);
    // VkyAxes* axes = ((VkyControllerAxes2D*)panel->controller)->axes;
    VkyPanzoom* panzoom = ((VkyAxes*)panel->controller)->panzoom_inner;

    VkyAxesTransform tr = {0};
    dvec2 ll = {0, -12};
    dvec2 ur = {1000, +12};
    dvec2 p = {0, 0};
    dvec2 out = {0, 0};

    // Data to GPU.
    tr = vky_axes_transform(panel, VKY_CDS_DATA, VKY_CDS_GPU);
    vky_axes_transform_apply(&tr, ll, out);
    AT(out[0] == -1);
    AT(out[1] == -1);

    vky_axes_transform_apply(&tr, ur, out);
    AT(out[0] == +1);
    AT(out[1] == +1);

    p[0] = 500;
    p[1] = 0;
    vky_axes_transform_apply(&tr, p, out);
    AT(out[0] == 0);
    AT(out[1] == 0);

    // GPU to panzoom
    tr = vky_axes_transform(panel, VKY_CDS_GPU, VKY_CDS_PANZOOM);
    AT(tr.scale[0] == 1);
    AT(tr.scale[1] == 1);
    AT(tr.shift[0] == 0);
    AT(tr.shift[1] == 0);

    panzoom->camera_pos[0] = .5;
    panzoom->camera_pos[1] = .5;
    panzoom->zoom[0] = 2;
    panzoom->zoom[1] = 2;
    tr = vky_axes_transform(panel, VKY_CDS_GPU, VKY_CDS_PANZOOM);
    vky_axes_transform_apply(&tr, (dvec2){.5, .5}, out);
    AT(out[0] == 0);
    AT(out[1] == 0);
    vky_axes_transform_apply(&tr, (dvec2){0, 0}, out);
    AT(out[0] == -1);
    AT(out[1] == -1);
    vky_axes_transform_apply(&tr, (dvec2){1, 1}, out);
    AT(out[0] == +1);
    AT(out[1] == +1);

    // Panel to canvas
    tr = vky_axes_transform(panel, VKY_CDS_PANZOOM, VKY_CDS_CANVAS_NDC);
    vky_axes_transform_apply(&tr, (dvec2){-1, -1}, out);
    AIN(out[0], 0, 0.1);
    AIN(out[1], -1, -0.95);

    vky_axes_transform_apply(&tr, (dvec2){+1, +1}, out);
    AIN(out[0], 0.49, 0.5);
    AIN(out[1], -.02, 0);

    // Canvas NDC to PX
    tr = vky_axes_transform(panel, VKY_CDS_CANVAS_NDC, VKY_CDS_CANVAS_PX);
    vky_axes_transform_apply(&tr, (dvec2){-1, -1}, out);
    AT(out[0] == 0);
    AT(out[1] == HEIGHT);

    vky_axes_transform_apply(&tr, (dvec2){+1, +1}, out);
    AT(out[0] == WIDTH);
    AT(out[1] == 0);

    // Full transform.
    tr = vky_axes_transform(panel, VKY_CDS_DATA, VKY_CDS_CANVAS_PX);
    vky_axes_transform_apply(&tr, (dvec2){750, +6}, out);
    AIN(out[0], WIDTH * .5, WIDTH * .75);
    AIN(out[1], HEIGHT * .5, HEIGHT * 1);

    return 0;
}

static int panzoom_1(VkyTestContext* context)
{
    VkyPanel* panel = context->panel;
    ASSERT(panel != NULL);

    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);
    VkyPanzoom* panzoom = ((VkyPanzoom*)panel->controller);

    panzoom->camera_pos[0] = 0;
    panzoom->camera_pos[1] = 0.5;
    panzoom->zoom[0] = .5;
    panzoom->zoom[1] = 2;
    VkyBox2D box = vky_panzoom_get_box(panel, panzoom, VKY_VIEWPORT_INNER);
    AT(box.pos_ll[0] == -2);
    AT(box.pos_ll[1] == 0);
    AT(box.pos_ur[0] == +2);
    AT(box.pos_ur[1] == 1);

    box.pos_ll[0] = 0;
    box.pos_ur[0] = 1;
    vky_panzoom_set_box(panzoom, VKY_VIEWPORT_INNER, box);
    AT(panzoom->camera_pos[0] == .5);
    AT(panzoom->camera_pos[1] == .5);
    AT(panzoom->zoom[0] == 2);
    AT(panzoom->zoom[1] == 2);

    return 0;
}

static int axes_1(VkyTestContext* context)
{
    VkyPanel* panel = context->panel;
    ASSERT(panel != NULL);
    _set_axes_1(panel);
    VkyAxes* axes = (VkyAxes*)panel->controller;
    VkyPanzoom* panzoom = axes->panzoom_inner;

    panzoom->camera_pos[0] = .5;
    panzoom->camera_pos[1] = .5;
    panzoom->zoom[0] = 2;
    panzoom->zoom[1] = 2;

    VkyBox2D box = vky_axes_get_range(axes);
    AT(box.pos_ll[0] == 500);
    AT(box.pos_ll[1] == 0);
    AT(box.pos_ur[0] == 1000);
    AT(box.pos_ur[1] == 12);

    // Reset panzoom for next test
    panzoom->camera_pos[0] = 0;
    panzoom->camera_pos[1] = 0;
    panzoom->zoom[0] = 1;
    panzoom->zoom[1] = 1;

    return 0;
}

static int axes_2(VkyTestContext* context)
{
    VkyPanel* panel = context->panel;
    ASSERT(panel != NULL);
    _set_axes_1(panel);
    VkyAxes* axes = (VkyAxes*)panel->controller;

    int res = 0;

    res += _check_axes_range(axes, 0, -12, 1000, 12);
    res += _check_axes_range(axes, 0, -12, 500, 12);
    res += _check_axes_range(axes, 250, -12, 750, 12);
    res += _check_axes_range(axes, 450, -6, 550, 6);

    return res;
}
