#include "../include/visky/visky.h"
#include "axes.h"
#include "axes_3D.h"

#define STB_IMAGE_IMPLEMENTATION
BEGIN_INCL_NO_WARN
#pragma GCC visibility push(default)
#include <stb_image.h>
#pragma GCC visibility pop
END_INCL_NO_WARN

// Defined as extern in colorsmaps.h
uint8_t VKY_COLOR_TEXTURE[CMAP_COUNT * CMAP_COUNT * 4];



/*************************************************************************************************/
/*  Picking                                                                                      */
/*************************************************************************************************/

VkyPick vky_pick(VkyScene* scene, vec2 canvas_coords)
{
    VkyPick pick = {0};
    VkyPanel* panel = vky_panel_from_mouse(scene, canvas_coords);

    pick.panel = panel;
    pick.pos_canvas_px[0] = canvas_coords[0];
    pick.pos_canvas_px[1] = canvas_coords[1];

    VkyAxesTransform tr = {0};

    tr = vky_axes_transform(panel, VKY_CDS_CANVAS_PX, VKY_CDS_CANVAS_NDC);
    vky_axes_transform_apply(&tr, pick.pos_canvas_px, pick.pos_canvas_ndc);

    tr = vky_axes_transform(panel, VKY_CDS_CANVAS_NDC, VKY_CDS_PANEL);
    vky_axes_transform_apply(&tr, pick.pos_canvas_ndc, pick.pos_panel);

    tr = vky_axes_transform(panel, VKY_CDS_PANEL, VKY_CDS_PANZOOM);
    vky_axes_transform_apply(&tr, pick.pos_panel, pick.pos_panzoom);

    tr = vky_axes_transform(panel, VKY_CDS_PANZOOM, VKY_CDS_GPU);
    vky_axes_transform_apply(&tr, pick.pos_panzoom, pick.pos_gpu);

    tr = vky_axes_transform(panel, VKY_CDS_GPU, VKY_CDS_DATA);
    vky_axes_transform_apply(&tr, pick.pos_gpu, pick.pos_data);

    return pick;
}

VkyPanel* vky_panel_from_mouse(VkyScene* scene, vec2 pos)
{
    uint32_t width = scene->canvas->size.framebuffer_width;
    uint32_t height = scene->canvas->size.framebuffer_height;
    float x = pos[0] / width;
    float y = pos[1] / height;
    int32_t row = 0, col = 0;
    ASSERT(scene->grid->xs[0] == 0);
    for (col = 0; col < (int32_t)scene->grid->col_count; col++)
    {
        if (scene->grid->xs[col] > x)
            break;
    }
    if (col > 0)
        col--;
    ASSERT(scene->grid->ys[0] == 0);
    for (row = 0; row < (int32_t)scene->grid->row_count; row++)
    {
        if (scene->grid->ys[row] > y)
            break;
    }
    if (row > 0)
        row--;

    ASSERT(0 <= col && col < (int32_t)scene->grid->col_count);
    ASSERT(0 <= row && row < (int32_t)scene->grid->row_count);
    return vky_get_panel(scene, (uint32_t)row, (uint32_t)col);
}



/*************************************************************************************************/
/*  MVP                                                                                          */
/*************************************************************************************************/

VkyMVP vky_create_mvp()
{
    VkyMVP mvp = {0};
    glm_mat4_copy(GLM_MAT4_IDENTITY, mvp.model);
    glm_mat4_copy(GLM_MAT4_IDENTITY, mvp.view);
    glm_mat4_copy(GLM_MAT4_IDENTITY, mvp.proj);
    return mvp;
}

void vky_mvp_set_model(mat4 model, VkyMVP* mvp) { glm_mat4_copy(model, mvp->model); }

void vky_mvp_set_view(mat4 view, VkyMVP* mvp) { glm_mat4_copy(view, mvp->view); }

void vky_mvp_set_view_3D(vec3 eye, vec3 center, VkyMVP* mvp)
{
    glm_lookat(eye, center, (vec3)VKY_DEFAULT_CAMERA_UP, mvp->view);
}

void vky_mvp_normal_matrix(VkyMVP* mvp, mat4 normal)
{
    glm_mat4_copy(mvp->view, normal);
    glm_mat4_mul(normal, mvp->model, normal);
    glm_mat4_inv(normal, normal);
    glm_mat4_transpose(normal);
}

void vky_mvp_set_proj(mat4 proj, VkyMVP* mvp) { glm_mat4_copy(proj, mvp->proj); }

void vky_mvp_set_proj_3D(VkyPanel* panel, VkyViewportType viewport_type, VkyMVP* mvp)
{
    // NOTE: use the inner viewport for the 3D projection matrix.
    VkyViewport viewport = vky_get_viewport(panel, viewport_type);
    float ratio = viewport.w / viewport.h;

    mat4 proj;
    glm_perspective(GLM_PI_4, ratio, VKY_PERSPECTIVE_NEAR_VAL, VKY_PERSPECTIVE_FAR_VAL, proj);
    glm_mat4_copy(proj, mvp->proj);
}

void vky_mvp_upload(VkyPanel* panel, VkyViewportType viewport_type, VkyMVP* mvp)
{
    VkyScene* scene = panel->scene;
    VkyCanvas* canvas = scene->canvas;
    ASSERT(canvas != NULL);
    VkyViewport viewport = vky_get_viewport(panel, viewport_type);
    uint32_t buffer_index = vky_get_panel_buffer_index(panel, viewport_type);

    // Update the uniform buffers.
    void* pointer = vky_get_dynamic_uniform_pointer(&scene->grid->dynamic_buffer, buffer_index);

    // Update the 3 MVP matrices.
    // mat4* p_model = (mat4*)pointer;
    ASSERT(pointer != NULL);
    int64_t offset = (int64_t)pointer;
    // ASSERT(p_model != NULL);

    glm_mat4_copy(mvp->model, (vec4*)offset); // *(p_model + 0));
    offset += (int32_t)sizeof(mat4);

    glm_mat4_copy(mvp->view, (vec4*)offset); // *(p_model + 1));
    offset += (int32_t)sizeof(mat4);

    glm_mat4_copy(mvp->proj, (vec4*)offset); // *(p_model + 2));
    offset += (int32_t)sizeof(mat4);

    // Update the vec4 vector.
    // vec4* p_viewport = (vec4*)(p_model + 3);
    // Viewport 4 coords.
    vec4 vec_viewport = {viewport.x, viewport.y, viewport.w, viewport.h};
    glm_vec4_copy(vec_viewport, (float*)offset); // *p_viewport);
    offset += (int32_t)sizeof(vec4);

    // Viewport margins.
    glm_vec4_copy(panel->margins, (float*)offset); // *(p_viewport + 1));
    offset += (int32_t)sizeof(vec4);

    // Update the Window size.
    vec4 win_size = {
        (float)canvas->size.framebuffer_width, (float)canvas->size.framebuffer_height, 0, 0};
    // Aspect ratio fixed?
    win_size[2] = mvp->aspect_ratio;
    glm_vec4_copy(win_size, (float*)offset); // *(p_viewport + 2));
    offset += (int32_t)sizeof(vec4);

    // Color information: 4 bytes, packed into a single uint32 in GLSL uniform common as GLSL does
    // not support uint8 variables.
    memcpy((void*)offset, panel->color_ctx, sizeof(cvec4));
    offset += (int32_t)sizeof(cvec4);
    ASSERT(sizeof(cvec4) == 4);

    ASSERT((offset - (int64_t)pointer) == VKY_MVP_BUFFER_SIZE);
}



/*************************************************************************************************/
/*  Controller                                                                                   */
/*************************************************************************************************/

static VkyVisual* vky_get_panel_first_visual(VkyPanel* panel)
{
    VkyGrid* grid = panel->scene->grid;
    ASSERT(grid != NULL);
    VkyVisualPanel vp;
    for (uint32_t i = 0; i < grid->visual_panel_count; i++)
    {
        vp = grid->visual_panels[i];
        if (vp.row == panel->row && vp.col == panel->col)
        {
            return vp.visual;
        }
    }
    return NULL;
}

static void set_colorbar(VkyPanel* panel, VkyAxes2DParams* params)
{
    ASSERT(params != NULL);
    // This function modifies the axes margins *before* the axes are set.

    switch (params->colorbar.position)
    {

    case VKY_COLORBAR_TOP:
        // TODO: use colorbar params for width, height, margin instead of constants
        params->margins[0] += VKY_COLORBAR_WIDTH + VKY_COLORBAR_VMARGIN;
        // TODO: set other params->colorbar params
        break;

    case VKY_COLORBAR_RIGHT:
        params->margins[1] += VKY_COLORBAR_WIDTH + VKY_COLORBAR_HMARGIN;

        params->colorbar.pos_tl[0] = +1;
        params->colorbar.pos_tl[1] = -1;

        params->colorbar.pad_tl[0] = VKY_COLORBAR_WIDTH + VKY_COLORBAR_HMARGIN;
        params->colorbar.pad_tl[1] = VKY_AXES_MARGIN_TOP;

        params->colorbar.pos_br[0] = +1;
        params->colorbar.pos_br[1] = +1;

        params->colorbar.pad_br[0] = VKY_COLORBAR_HMARGIN + VKY_AXES_MARGIN_RIGHT;
        params->colorbar.pad_br[1] = params->margins[2];

        params->colorbar.z = 0;

        break;

    case VKY_COLORBAR_BOTTOM:
        params->margins[2] += VKY_COLORBAR_WIDTH + VKY_COLORBAR_VMARGIN;
        // TODO: set other params->colorbar params
        break;

    case VKY_COLORBAR_LEFT:
        params->margins[3] += VKY_COLORBAR_WIDTH + VKY_COLORBAR_HMARGIN;
        // TODO: set other params->colorbar params
        break;

    default:
        break;
    }

    VkyVisualBundle* colorbar = vky_bundle_colorbar(panel->scene, params->colorbar);
    vky_add_visual_bundle_to_panel(colorbar, panel, VKY_VIEWPORT_OUTER, VKY_VISUAL_PRIORITY_NONE);
}

static void add_label(VkyPanel* panel, VkyAxis axis, VkyAxes2DParams* params)
{
    ASSERT(params != NULL);

    VkyVisual* label = vky_visual_text(panel->scene);
    vky_add_visual_to_panel(label, panel, VKY_VIEWPORT_OUTER, VKY_VISUAL_PRIORITY_NONE);
    uint32_t i = 0;
    VkyTextData text_data[2];
    memset(text_data, 0, sizeof(text_data));

    VkyTextData label_data = {0};
    switch (axis)
    {

    case VKY_AXIS_X:
        params->margins[2] += VKY_AXES_LABEL_VMARGIN;
        label_data = (VkyTextData){
            {0, +1, 0}, // label position
            {0, -params->margins[2] + VKY_AXES_FONT_SIZE + VKY_AXES_LABEL_VMARGIN * .9}, // shift
            params->xlabel.color,                                                        //
            params->xlabel.font_size,                                                    //
            {0, +1},                                                                     // anchor
            0, // angle (horizontal)
            strlen(params->xlabel.label),
            params->xlabel.label,
            true,
        };
        memcpy(text_data + i, &label_data, sizeof(VkyTextData));
        i++;
        break;

    case VKY_AXIS_Y:
        params->margins[3] += VKY_AXES_LABEL_HMARGIN;
        label_data = (VkyTextData){
            {-1, 0, 0}, // label position
            // shift
            {VKY_AXES_LABEL_HMARGIN, 0},
            params->ylabel.color,     //
            params->ylabel.font_size, //
            {0, -1},                  // anchor
            M_PI / 2,                 // angle (vertical)
            strlen(params->ylabel.label),
            params->ylabel.label,
            true,
        };
        memcpy(text_data + i, &label_data, sizeof(VkyTextData));
        i++;
        break;

    default:
        break;
    }

    vky_visual_upload(label, (VkyData){i, text_data});
}

void vky_set_controller(VkyPanel* panel, VkyControllerType controller_type, const void* params)
{
    /* NOTE: this function should ideally be called *after* all visuals have been added to the
     * scene so that any controller visuals are on top of the others */
    panel->controller_type = controller_type;
    void* controller = NULL;
    switch (controller_type)
    {

    case VKY_CONTROLLER_PANZOOM:
        controller = vky_panzoom_init();
        break;

    case VKY_CONTROLLER_AXES_2D:;
        // Create default axes params if needed.
        VkyAxes2DParams axparams = {0};
        if (params == NULL)
            axparams = vky_default_axes_2D_params();
        else
            memcpy(&axparams, params, sizeof(VkyAxes2DParams));

        // Add labels.
        if (strlen(axparams.xlabel.label) > 0)
            add_label(panel, VKY_AXIS_X, &axparams);
        if (strlen(axparams.ylabel.label) > 0)
            add_label(panel, VKY_AXIS_Y, &axparams);

        // Add colorbar.
        if (axparams.colorbar.position != VKY_COLORBAR_NONE)
        {
            set_colorbar(panel, &axparams);
        }

        controller = vky_axes_init(panel, axparams);

        vky_add_visual_to_panel(
            ((VkyAxes*)controller)->tick_visual, panel, VKY_VIEWPORT_OUTER,
            VKY_VISUAL_PRIORITY_LAST);
        vky_add_visual_to_panel(
            ((VkyAxes*)controller)->text_visual, panel, VKY_VIEWPORT_OUTER,
            VKY_VISUAL_PRIORITY_LAST);

        break;

    case VKY_CONTROLLER_ARCBALL:
        controller = vky_arcball_init(VKY_MVP_MODEL);
        break;

    case VKY_CONTROLLER_AXES_3D:
        controller = vky_arcball_init(VKY_MVP_MODEL);
        vky_axes_3D_init(panel);
        break;

    case VKY_CONTROLLER_VOLUME:
        controller = vky_arcball_init(VKY_MVP_VIEW);
        break;

    case VKY_CONTROLLER_FPS:
    case VKY_CONTROLLER_FLY:
    case VKY_CONTROLLER_AUTOROTATE:
        controller = vky_camera_init();
        break;

    default:
        break;
    }
    panel->controller = controller;
}

static void _update_controller(VkyPanel* panel)
{
    // Update the panel's controller from the event controller (mouse and keyboard).

    VkyPanzoom* panzoom = NULL;

    switch (panel->controller_type)
    {

    case VKY_CONTROLLER_PANZOOM:
        panzoom = (VkyPanzoom*)(panel->controller);

        // Reset lim_reached.
        panzoom->lim_reached[0] = false;
        panzoom->lim_reached[1] = false;

        vky_panzoom_update(panel, panzoom, VKY_VIEWPORT_INNER);
        break;

    case VKY_CONTROLLER_AXES_2D:;
        vky_axes_panzoom_update((VkyAxes*)panel->controller);
        break;

    case VKY_CONTROLLER_ARCBALL:
    case VKY_CONTROLLER_AXES_3D:
        // TODO: split between update and mvp
        vky_arcball_update(panel, (VkyArcball*)(panel->controller), VKY_VIEWPORT_INNER);
        break;

    case VKY_CONTROLLER_VOLUME:;
        // Arcball interaction.
        VkyArcball* arcball = (VkyArcball*)(panel->controller);
        vky_arcball_update(panel, arcball, VKY_VIEWPORT_INNER);

        // Get the volume visual.
        VkyVisual* visual = vky_get_panel_first_visual(panel);
        VkyVolumeParams* volume_params = (VkyVolumeParams*)(visual->params);

        // Compute the inverse transform (view and proj) to simulate the camera directly in the
        // fragment shader.
        mat4 mat;
        glm_mat4_mul(arcball->mvp.proj, arcball->mvp.view, mat);
        glm_mat4_inv(mat, volume_params->inv_proj_view);

        // Compute the normal matrix.
        vky_mvp_normal_matrix(&arcball->mvp, volume_params->normal_mat);

        // Update the params.
        vky_visual_params(visual, sizeof(VkyVolumeParams), volume_params);

        break;

    case VKY_CONTROLLER_FPS:
    case VKY_CONTROLLER_FLY:
    case VKY_CONTROLLER_AUTOROTATE:;
        VkyCamera* camera = (VkyCamera*)(panel->controller);
        vky_camera_update(panel, camera, VKY_VIEWPORT_INNER);
        break;

    default:
        break;
    }
}

#define PBOX(x)                                                                                   \
    printf("%f %f %f %f\n", (x).pos_ll[0], (x).pos_ll[1], (x).pos_ur[0], (x).pos_ur[1]);

static void _update_linked_panel(VkyPanel* p0, VkyPanel* p1, VkyPanelLinkMode mode)
{
    // update the linked panel p1 as a function of the active panel p0, using the specified link
    // mode.
    ASSERT(p0 != NULL);
    ASSERT(p1 != NULL);
    ASSERT(p0->controller_type == p1->controller_type);

    VkyBox2D box = {0};

    switch (p0->controller_type)
    {
    case VKY_CONTROLLER_AXES_2D:;
        VkyAxes* ax0 = (VkyAxes*)p0->controller;
        VkyAxes* ax1 = (VkyAxes*)p1->controller;
        bool update_x = (mode & VKY_PANEL_LINK_X) != 0;
        bool update_y = (mode & VKY_PANEL_LINK_Y) != 0;
        if (!update_x && !update_y)
            return;
        box = vky_axes_get_range(ax0);
        if (!update_x)
        {
            box.pos_ll[0] = box.pos_ur[0] = 0;
        }
        if (!update_y)
        {
            box.pos_ll[1] = box.pos_ur[1] = 0;
        }
        vky_axes_set_range(ax1, box, true);
        break;

    default:
        break;
    }
}

static void _update_mvp(VkyPanel* panel)
{
    // Upload the MVP data to the GPU using the panel's controller state.
    VkyPanzoom* panzoom = NULL;

    switch (panel->controller_type)
    {

    case VKY_CONTROLLER_PANZOOM:
        panzoom = (VkyPanzoom*)(panel->controller);
        vky_panzoom_mvp(panel, panzoom, VKY_VIEWPORT_INNER);
        break;

    case VKY_CONTROLLER_AXES_2D:;
        VkyAxes* axes = (VkyAxes*)panel->controller;
        vky_panzoom_mvp(panel, axes->panzoom_outer, VKY_VIEWPORT_OUTER);
        vky_panzoom_mvp(panel, axes->panzoom_inner, VKY_VIEWPORT_INNER);
        break;

    case VKY_CONTROLLER_ARCBALL:
    case VKY_CONTROLLER_AXES_3D:
        // TODO: split between update and mvp
        break;

    case VKY_CONTROLLER_VOLUME:;
        // TODO: split between update and mvp
        break;

    case VKY_CONTROLLER_FPS:
    case VKY_CONTROLLER_FLY:
    case VKY_CONTROLLER_AUTOROTATE:;
        break;

    default:
        break;
    }
}

static void _link_panels(VkyApp* app)
{
    // NOTE: using the app and not canvas as we might want to link panels between different
    // canvases.
    ASSERT(app != NULL);
    if (app->links == NULL)
    {
        return;
    }
    ASSERT(app->links != NULL);
    ASSERT(app->link_count > 0);

    VkyPanel* p0 = NULL;
    VkyPanel* p1 = NULL;
    VkyPanelLink* link = NULL;

    for (uint32_t i = 0; i < app->link_count; i++)
    {
        link = &((VkyPanelLink*)app->links)[i];
        ASSERT(link != NULL);
        ASSERT(link->p0 != NULL);
        ASSERT(link->p1 != NULL);
        p0 = link->p0;
        p1 = link->p1;
        if (link->mode == VKY_PANEL_LINK_NONE)
            continue;
        ASSERT(link->mode);

        // NOTE: we ensure p0 is the active panel and p1 is the non-active one.
        if (link->p0->status == VKY_PANEL_STATUS_ACTIVE ||
            link->p0->status == VKY_PANEL_STATUS_RESET)
        {
            p0 = link->p0;
            p1 = link->p1;
        }
        else if (
            link->p1->status == VKY_PANEL_STATUS_ACTIVE ||
            link->p1->status == VKY_PANEL_STATUS_RESET)
        {
            p0 = link->p1;
            p1 = link->p0;
        }
        else
        {
            continue;
        }

        // Set the status of the linked panel.
        p1->status = VKY_PANEL_STATUS_LINKED;

        if (p0 == p1)
            continue;
        if (p0->controller_type != p1->controller_type)
        {
            log_warn("linking panels of different controller types is not yet supported");
            continue;
        }
        _update_linked_panel(p0, p1, link->mode);
    }
}

static void _controller_callback(VkyCanvas* canvas)
{
    VkyScene* scene = canvas->scene;
    VkyGrid* grid = scene->grid;

    for (uint32_t i = 0; i < grid->panel_count; i++)
    {
        // Reset the status of every panel.
        grid->panels[i].status = VKY_PANEL_STATUS_NONE;
        // This function will set the active panel's status to ACTIVE.
        _update_controller(&grid->panels[i]);
    }

    // This function finds the panels that depend on the active panel, set them to LINKED
    // status, update their controller.
    _link_panels(canvas->app);

    // Update the MVP of all panels.
    for (uint32_t i = 0; i < grid->panel_count; i++)
    {
        _update_mvp(&grid->panels[i]);
    }
}

static void _controller_resize_callback(VkyCanvas* canvas)
{
    VkyGrid* grid = canvas->scene->grid;
    VkyPanel* panel = NULL;
    for (uint32_t i = 0; i < grid->panel_count; i++)
    {
        panel = &grid->panels[i];
        switch (panel->controller_type)
        {

        case VKY_CONTROLLER_PANZOOM:
            break;

        case VKY_CONTROLLER_AXES_2D:;
            VkyAxes* axes = (VkyAxes*)(panel->controller);
            vky_axes_rescale(axes);
            vky_axes_compute_ticks(axes);
            vky_axes_update_visuals(axes);
            break;

        default:
            break;
        }
    }
}

void* vky_get_axes(VkyPanel* panel)
{
    switch (panel->controller_type)
    {
    case VKY_CONTROLLER_AXES_2D:
        return panel->controller;
    default:
        log_warn("no axes found in this panel");
        return NULL;
    }
}

void vky_destroy_controller(VkyPanel* panel)
{
    ASSERT(panel != NULL);
    void* controller = panel->controller;
    switch (panel->controller_type)
    {

    default:
        break;
    }
    if (controller != NULL)
        free(controller);
    panel->controller = NULL;
}

void vky_link_panels(VkyPanel* p0, VkyPanel* p1, VkyPanelLinkMode mode)
{
    VkyApp* app = p0->scene->canvas->app;
    ASSERT(app != NULL);
    ASSERT(p1->scene->canvas->app == app);
    // Create the array of panel links if needed.
    if (app->links == NULL)
    {
        app->links = calloc(VKY_MAX_PANEL_LINKS, sizeof(VkyPanelLink));
    }
    VkyPanelLink* link = &((VkyPanelLink*)app->links)[app->link_count];
    ASSERT(link != NULL);
    ASSERT(p0 != NULL);
    ASSERT(p1 != NULL);

    link->p0 = p0;
    link->p1 = p1;
    link->mode = mode;

    app->link_count++;
}



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

VkyVisual* vky_create_visual(VkyScene* scene, VkyVisualType visual_type)
{
    VkyVisual visual = {0};
    visual.scene = scene;
    visual.visual_type = visual_type;
    visual.resources = calloc(VKY_MAX_VISUAL_RESOURCES, sizeof(void*));

    // NOTE: by convention, the first buffer is the indirect draw buffer.
    // HACK: we allocate the bigger indexed command so that we have enough space,
    // even if the visual doesn't use indexed drawing.
    log_trace("create indirect buffer for visual");
    ASSERT(scene->canvas->gpu->buffer_count > 0);
    visual.indirect_buffer =
        vky_allocate_buffer(&scene->canvas->gpu->buffers[0], sizeof(VkDrawIndexedIndirectCommand));

    scene->visuals[scene->visual_count] = visual;
    VkyVisual* out = &scene->visuals[scene->visual_count];
    ASSERT(out != NULL);
    scene->visual_count++;
    return out;
}

VkyVisual*
vky_visual(VkyScene* scene, VkyVisualType visual_type, const void* params, const void* obj)
{
    VkyVisual* visual = NULL;
    switch (visual_type)
    {

    case VKY_VISUAL_RECTANGLE:
        visual = vky_visual_rectangle(scene, (const VkyRectangleParams*)params);
        break;

    case VKY_VISUAL_RECTANGLE_AXIS:
        visual = vky_visual_rectangle_axis(scene);
        break;

    case VKY_VISUAL_AREA:
        visual = vky_visual_area(scene, (const VkyAreaParams*)params);
        break;

    case VKY_VISUAL_MESH:
        visual = vky_visual_mesh(scene, (const VkyMeshParams*)params, obj);
        break;

    case VKY_VISUAL_MESH_RAW:
        visual = vky_visual_mesh_raw(scene);
        break;

    case VKY_VISUAL_MARKER:
        visual = vky_visual_marker(scene, (const VkyMarkersParams*)params);
        break;

    case VKY_VISUAL_MARKER_RAW:
        visual = vky_visual_marker_raw(scene, (const VkyMarkersRawParams*)params);
        break;

    case VKY_VISUAL_SEGMENT:
        visual = vky_visual_segment(scene);
        break;

    case VKY_VISUAL_ARROW:
        visual = vky_visual_arrow(scene);
        break;

    case VKY_VISUAL_PATH:
        visual = vky_visual_path(scene, (const VkyPathParams*)params);
        break;

    case VKY_VISUAL_PATH_RAW:
        visual = vky_visual_path_raw(scene);
        break;

    case VKY_VISUAL_PATH_RAW_MULTI:
        visual = vky_visual_path_raw_multi(scene, (const VkyMultiRawPathParams*)params);
        break;

    case VKY_VISUAL_FAKE_SPHERE:
        visual = vky_visual_fake_sphere(scene, (const VkyFakeSphereParams*)params);
        break;

    case VKY_VISUAL_IMAGE:
        visual = vky_visual_image(scene, (const VkyTextureParams*)params);
        break;

    case VKY_VISUAL_VOLUME:
        visual = vky_visual_volume(scene, (const VkyTextureParams*)params, obj);
        break;

    case VKY_VISUAL_TEXT:
        visual = vky_visual_text(scene);
        break;

    default:
        log_error("unknown visual type");
        break;
    }
    ASSERT(visual != NULL);
    return visual;
}

VkyVisualBundle* vky_create_visual_bundle(VkyScene* scene)
{
    VkyVisualBundle vb = {0};
    vb.scene = scene;
    vb.visual_count = 0;
    vb.visuals = calloc(VKY_MAX_VISUALS_PER_BUNDLE, sizeof(VkyVisual));

    scene->visual_bundles[scene->visual_bundle_count] = vb;
    VkyVisualBundle* out = &scene->visual_bundles[scene->visual_bundle_count];
    ASSERT(out != NULL);
    scene->visual_bundle_count++;
    return out;
}

void vky_add_visual_to_bundle(VkyVisualBundle* vb, VkyVisual* visual)
{
    vb->visuals[vb->visual_count] = visual;
    vb->visual_count++;
}

void vky_visual_params(VkyVisual* visual, size_t params_size, const void* params)
{
    if (visual->params_buffer.buffer_size == 0)
    {
        log_trace("automatically create uniform buffer for visual");
        visual->params_buffer = vky_create_uniform_buffer(
            visual->scene->canvas->gpu, visual->scene->canvas->image_count, params_size);
    }
    // Copy the params data to a heap-allocated buffer so that it lives until the destruction
    // of the visual.
    if (visual->params == NULL)
    {
        visual->params = malloc(params_size);
    }
    ASSERT(visual->params != NULL);
    memcpy(visual->params, params, params_size);
    for (uint32_t image_index = 0; image_index < visual->scene->canvas->image_count; image_index++)
    {
        vky_upload_uniform_buffer(&visual->params_buffer, image_index, params);
    }
}

void vky_add_uniform_buffer_resource(VkyVisual* visual, VkyUniformBuffer* ubo)
{
    visual->resources[visual->resource_count] = ubo;
    visual->resource_count++;

    if (visual->resource_count == visual->pipeline.resource_layout.binding_count)
    {
        vky_bind_resources(
            visual->pipeline.resource_layout, visual->pipeline.descriptor_sets, visual->resources);
    }
}

VkyResourceLayout vky_common_resource_layout(VkyVisual* visual)
{
    /* Common resource layout: (0) dynamic uniform buffer for MVP and color mode, (1) color texture
     */
    /* NOTE: visual_params() must be called BEFORE this function */
    VkyCanvas* canvas = visual->scene->canvas;
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vky_add_resource_binding(&resource_layout, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    if (visual->params != NULL)
        vky_add_resource_binding(&resource_layout, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    return resource_layout;
}

void vky_add_common_resources(VkyVisual* visual)
{
    vky_add_uniform_buffer_resource(visual, &visual->scene->grid->dynamic_buffer);
    if (visual->scene->canvas->gpu->texture_count > 0)
        vky_add_texture_resource(visual, &visual->scene->canvas->gpu->textures[0]);
    if (visual->params != NULL)
        vky_add_uniform_buffer_resource(visual, &visual->params_buffer);
}

void vky_allocate_vertex_buffer(VkyVisual* visual, VkDeviceSize size)
{
    /* Allocate a vertex buffer for the visual using an existing buffer if there is one, or
     * creating a new one if needed. */
    log_trace("allocate vertex buffer with size %d", size);
    if (visual->vertex_buffer.buffer != NULL)
        return;
    VkyBuffer* buffer =
        vky_find_buffer(visual->scene->canvas->gpu, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    ASSERT(buffer != NULL);
    ASSERT(buffer->size - buffer->allocated_size >= size);
    visual->vertex_buffer = vky_allocate_buffer(buffer, size);
}

void vky_allocate_index_buffer(VkyVisual* visual, VkDeviceSize size)
{
    log_trace("allocate index buffer with size %d", size);
    /* Allocate a index buffer for the visual using an existing buffer if there is one, or
     * creating a new one if needed. */
    if (visual->index_buffer.buffer != NULL)
        return;
    VkyBuffer* buffer =
        vky_find_buffer(visual->scene->canvas->gpu, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    ASSERT(buffer != NULL);
    ASSERT(buffer->size - buffer->allocated_size >= size);
    visual->index_buffer = vky_allocate_buffer(buffer, size);
}

void vky_add_texture_resource(VkyVisual* visual, VkyTexture* texture)
{
    visual->resources[visual->resource_count] = texture;
    visual->resource_count++;

    if (visual->resource_count == visual->pipeline.resource_layout.binding_count)
    {
        vky_bind_resources(
            visual->pipeline.resource_layout, visual->pipeline.descriptor_sets, visual->resources);
    }
}

void vky_set_color_context(VkyPanel* panel, VkyColormap cmap, VkyColorMod cmod, uint8_t constant)
{
    // 32-colors palettes need special care as there is a discrepancy with the tex row.
    if (cmap >= CPAL032_OFS)
    {
        cmap = (VkyColormap)(CPAL032_OFS + ((uint8_t)cmap - CPAL032_OFS) / CPAL032_PER_ROW);
    }
    panel->color_ctx[0] = cmap;
    panel->color_ctx[1] = cmod;
    panel->color_ctx[2] = constant;
    panel->color_ctx[3] = 0; // NOTE: currently unused
}

void vky_visual_upload(VkyVisual* visual, VkyData data)
{
    // Bake the data and save the size and pointers of the baked vertex/index buffers into the
    // data struct.
    ASSERT(visual != NULL);

    // Determine whether the visual has a baking process.
    bool has_bake = visual->cb_bake_data != NULL;

    if (data.vertex_count == 0 || data.vertices == NULL)
    {
        // NOTE: if no vertices but no baking, we assume the items pass through to vertices
        if (!has_bake)
        {
            log_trace("pass through vertex data");
            data.vertex_count = data.item_count;
            data.vertices = data.items;
        }
        else
        {
            // log_trace("bake vertex data");
            // NOTE: the cb_bake_data callback function must malloc() the arrays with the
            // vertices and indices. They will be freed at the end of the present function.
            data = visual->data = visual->cb_bake_data(visual, data);
        }
    }
    ASSERT(data.vertex_count > 0);
    bool has_indices = data.index_count > 0;

    // Allocation of the vertex and index buffers if needed.
    VkDeviceSize vertex_size = visual->pipeline.vertex_layout.stride;
    ASSERT(vertex_size > 0);
    VkDeviceSize vbuf_size = data.vertex_count * vertex_size;

    // Create vertex buffer if the user didn't already do it.
    if (visual->vertex_buffer.size == 0)
    {
        ASSERT(data.vertex_count > 0);
        // Reuse an existing vertex buffer if one sufficiently large exists, or create a new
        // one.
        VkyBuffer* buffer = vky_find_buffer(
            visual->scene->canvas->gpu, vbuf_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        ASSERT(buffer != NULL);
        visual->vertex_buffer = vky_allocate_buffer(buffer, vbuf_size);
    }
    ASSERT(visual->vertex_buffer.buffer != NULL);

    // Create index buffer if the user didn't already do it.
    if (has_indices && visual->index_buffer.size == 0)
    {
        VkDeviceSize ibuf_size = data.index_count * sizeof(VkyIndex);
        // Reuse an existing vertex buffer if one sufficiently large exists, or create a new
        // one.
        VkyBuffer* buffer = vky_find_buffer(
            visual->scene->canvas->gpu, ibuf_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        ASSERT(buffer != NULL);
        visual->index_buffer = vky_allocate_buffer(buffer, ibuf_size);
    }
    ASSERT(!has_indices || visual->index_buffer.buffer != NULL);

    // Check that the buffers are large enough.
    ASSERT(visual->vertex_buffer.size >= vbuf_size);
    ASSERT(!has_indices || visual->index_buffer.size >= data.index_count * sizeof(VkyIndex));

    // Upload the data if data was provided.
    if (data.vertices != NULL)
    {
        vky_upload_buffer(visual->vertex_buffer, 0, vbuf_size, data.vertices);
    }
    if (has_indices && data.indices != NULL)
    {
        vky_upload_buffer(
            visual->index_buffer, 0, data.index_count * sizeof(VkyIndex), data.indices);
    }

    // Indirect draw call.
    if (!has_indices)
    {
        // TODO: customizable offsets
        ASSERT(data.vertex_count > 0);
        // log_trace("upload indirect draw data %d", data.vertex_count);
        VkDrawIndirectCommand ind = (VkDrawIndirectCommand){data.vertex_count, 1, 0, 0};
        visual->indirect_item_count = data.vertex_count;
        vky_upload_buffer(visual->indirect_buffer, 0, sizeof(ind), &ind);
    }
    else
    {
        ASSERT(data.index_count > 0);
        // log_trace("upload indirect indexed draw data %d", data.index_count);
        VkDrawIndexedIndirectCommand ind = {data.index_count, 1, 0, 0, 0};
        visual->indirect_item_count = data.index_count;
        vky_upload_buffer(visual->indirect_buffer, 0, sizeof(ind), &ind);
    }

    // Free the allocated buffers.
    if (has_bake)
    {
        if (data.vertices != NULL && !data.no_vertices_alloc)
        {
            log_trace("freeing data.vertices");
            free(data.vertices);
            data.vertices = NULL;
        }
        if (data.indices != NULL)
        {
            log_trace("freeing data.indices");
            free(data.indices);
            data.indices = NULL;
        }
    }

    // Update the visual's data structure.
    visual->data = data;
}

void vky_visual_upload_partial(VkyVisual* visual, uint32_t item_offset, VkyData data)
{
    // Bake the data and save the size and pointers of the baked vertex/index buffers into the
    // data struct.

    // Determine whether the visual has a baking process.
    // log_trace("upload partial");
    bool has_bake = visual->cb_bake_data != NULL;
    uint32_t vertex_count = 0;
    void* vertices = NULL;

    if (!has_bake)
    {
        // NOTE: if no vertices but no baking, we assume the items pass through to vertices
        vertex_count = data.item_count;
        vertices = data.items;
    }
    else
    {
        // NOTE: the cb_bake_data callback function must malloc() the arrays with the
        // vertices and indices. They will be freed at the end of the present function.
        data = visual->cb_bake_data(visual, data);
        vertex_count = data.vertex_count;
        vertices = data.vertices;
    }
    ASSERT(vertex_count > 0);
    ASSERT(vertices != NULL);

    // Find the offset and size in the vertex buffer.
    VkDeviceSize vertex_size = visual->pipeline.vertex_layout.stride;
    ASSERT(vertex_size > 0);
    VkDeviceSize vbuf_size = data.vertex_count * vertex_size;
    VkDeviceSize vertex_offset = item_offset * vertex_size;

    vky_upload_buffer(visual->vertex_buffer, vertex_offset, vbuf_size, data.vertices);
    // NOT IMPLEMENTED YET: update the index buffer with new indices.

    // Free the allocated buffers.
    if (has_bake)
    {
        free(vertices);
    }
}

void vky_toggle_visual_visibility(VkyVisual* visual, bool is_visible)
{
    uint32_t count = is_visible ? visual->indirect_item_count : 0;
    if (visual->index_buffer.size == 0)
    {
        // Non-indexed.
        // TODO: other params in indirect rendering.
        VkDrawIndirectCommand cmd = {count, 1, 0, 0};
        vky_upload_buffer(visual->indirect_buffer, 0, sizeof(cmd), &cmd);
    }
    else
    {
        // Indexed.
        VkDrawIndexedIndirectCommand cmd = {count, 1, 0, 0, 0};
        vky_upload_buffer(visual->indirect_buffer, 0, sizeof(cmd), &cmd);
    }
}

void vky_destroy_visual(VkyVisual* visual)
{
    log_trace("destroy visual");
    vky_destroy_uniform_buffer(&visual->params_buffer);
    vky_destroy_vertex_layout(&visual->pipeline.vertex_layout);
    vky_destroy_resource_layout(&visual->pipeline.resource_layout);
    vky_destroy_shaders(&visual->pipeline.shaders);
    vky_destroy_graphics_pipeline(&visual->pipeline);

    free(visual->resources);
    visual->resources = NULL;
    free(visual->params);
    visual->params = NULL;
}

void vky_destroy_visual_bundle(VkyVisualBundle* vb)
{
    log_trace("destroy visual bundle");

    if (vb->visuals != NULL)
        free(vb->visuals);
    vb->visuals = NULL;

    // Destroy the parametersS.
    if (vb->params != NULL)
        free(vb->params);
}

void vky_free_data(VkyData data)
{
    if (data.vertices != NULL)
        free(data.vertices);
    if (data.indices != NULL)
        free(data.indices);
    data.vertices = data.indices = NULL;
}



/*************************************************************************************************/
/*  Panels                                                                                       */
/*************************************************************************************************/

void vky_set_full_viewport(VkyCanvas* canvas)
{
    VkCommandBuffer command_buffer = canvas->command_buffers[canvas->current_command_buffer_index];
    vky_set_viewport(
        command_buffer, 0, 0, canvas->size.framebuffer_width, canvas->size.framebuffer_height);
}

void vky_reset_all_mvp(VkyScene* scene)
{
    VkyMVP mvp = {0};
    glm_mat4_copy(GLM_MAT4_IDENTITY, mvp.model);
    glm_mat4_copy(GLM_MAT4_IDENTITY, mvp.view);
    glm_mat4_copy(GLM_MAT4_IDENTITY, mvp.proj);

    for (uint32_t i = 0; i < scene->grid->panel_count; i++)
    {
        for (uint32_t v = 0; v < 2; v++)
        {
            // Default color context.
            vky_set_color_context(
                &scene->grid->panels[i], VKY_DEFAULT_COLORMAP, VKY_DEFAULT_COLOR_MOD,
                VKY_DEFAULT_COLOR_OPT);
            // TODO: rename to vky_common_upload() or something?
            vky_mvp_upload(&scene->grid->panels[i], v, &mvp);
        }
    }
    vky_mvp_finalize(scene);
}

void vky_mvp_finalize(VkyScene* scene)
{
    VkyCanvas* canvas = scene->canvas;
    ASSERT(canvas != NULL);

    VkyUniformBuffer* dubo = &scene->grid->dynamic_buffer;

    // Upload that to the GPU.
    for (uint32_t image_index = 0; image_index < canvas->image_count; image_index++)
    {
        vky_upload_dynamic_uniform_buffer(dubo, image_index);
    }
}

VkyViewport vky_remove_viewport_margins(VkyViewport viewport, vec4 margins)
{
    viewport.x += margins[3];
    viewport.y += margins[0];

    viewport.w -= margins[1];
    viewport.w -= margins[3];

    viewport.h -= margins[0];
    viewport.h -= margins[2];

    return viewport;
}

VkyViewport vky_add_viewport_margins(VkyViewport viewport, vec4 margins)
{
    viewport.x -= margins[3];
    viewport.y -= margins[0];

    viewport.w += margins[1];
    viewport.w += margins[3];

    viewport.h += margins[0];
    viewport.h += margins[2];

    return viewport;
}

VkyPanel* vky_get_panel(VkyScene* scene, uint32_t row, uint32_t col)
{
    uint32_t panel_index = row * scene->grid->col_count + col;
    return &scene->grid->panels[panel_index];
}

VkyPanelIndex vky_get_panel_index(VkyPanel* panel)
{
    return (VkyPanelIndex){panel->row, panel->col};
}

VkyViewport vky_get_viewport(VkyPanel* panel, VkyViewportType viewport_type)
{
    VkyScene* scene = panel->scene;
    VkyViewport viewport =
        panel->viewport; // NOTE: in relative NDC coordinates, need to multiply by the window size.
    float W = scene->canvas->size.framebuffer_width;
    float H = scene->canvas->size.framebuffer_height;
    viewport.x *= W;
    viewport.w *= W;
    viewport.y *= H;
    viewport.h *= H;
    // Remove the margins if getting the inner viewport.
    if (viewport_type == VKY_VIEWPORT_INNER)
    {
        viewport = vky_remove_viewport_margins(viewport, panel->margins);
    }
    return viewport;
}

uint32_t vky_get_panel_buffer_index(VkyPanel* panel, VkyViewportType viewport_type)
{
    uint32_t buffer_index = 0;
    if (viewport_type == VKY_VIEWPORT_INNER)
    {
        buffer_index = panel->inner_uniform_index;
        ASSERT(buffer_index % 2 == 0);
    }
    else if (viewport_type == VKY_VIEWPORT_OUTER)
    {
        buffer_index = panel->outer_uniform_index;
        ASSERT(buffer_index % 2 == 1);
    }
    return buffer_index;
}

void vky_add_visual_to_panel(
    VkyVisual* visual, VkyPanel* panel, VkyViewportType viewport_type, VkyVisualPriority priority)
{
    VkyGrid* grid = visual->scene->grid;
    uint32_t n = grid->visual_panel_count;
    grid->visual_panels[n] =
        (VkyVisualPanel){visual, panel->row, panel->col, viewport_type, priority};
    grid->visual_panel_count++;
}

void vky_add_visual_bundle_to_panel(
    VkyVisualBundle* visual_bundle, VkyPanel* panel, VkyViewportType viewport_type,
    VkyVisualPriority priority)
{
    // Add to the panel all visuals in the bundle.
    for (uint32_t i = 0; i < visual_bundle->visual_count; i++)
    {
        vky_add_visual_to_panel(visual_bundle->visuals[i], panel, viewport_type, priority);
    }
}

void vky_draw_visual(VkyVisual* visual, VkyPanel* panel, VkyViewportType viewport_type)
{
    log_trace("draw visual type %d", visual->visual_type);
    VkyScene* scene = visual->scene;
    VkyCanvas* canvas = scene->canvas;
    VkyViewport viewport = vky_get_viewport(panel, viewport_type);
    VkCommandBuffer command_buffer = canvas->command_buffers[canvas->current_command_buffer_index];
    uint32_t buffer_index = vky_get_panel_buffer_index(panel, viewport_type);

    // Bind the vertex buffer.
    if (visual->vertex_buffer.buffer == NULL)
    {
        log_error("no buffer, skipping draw");
        return;
    }
    vky_bind_vertex_buffer(command_buffer, visual->vertex_buffer, 0);

    // Bind the index buffer if there is one.
    if (visual->index_buffer.buffer != NULL)
    {
        vky_bind_index_buffer(command_buffer, visual->index_buffer, 0);
    }

    // Bind the pipeline.
    vky_bind_graphics_pipeline(command_buffer, &visual->pipeline);

    // Bind the dynamic uniform with the panel's MVP.
    vky_bind_dynamic_uniform(
        command_buffer, &visual->pipeline, &scene->grid->dynamic_buffer,
        canvas->current_command_buffer_index, buffer_index);

    // Set the panel viewport.
    vky_set_viewport(
        command_buffer, (int32_t)viewport.x, (int32_t)viewport.y, (uint32_t)viewport.w,
        (uint32_t)viewport.h);

    // Draw
    // TODO: option to disable indirect drawing if fixed # of vertices/indices
    if (visual->index_buffer.buffer != NULL)
    {
        // Indexed.
        ASSERT(visual->data.index_count > 0);
        vky_draw_indexed_indirect(command_buffer, visual->indirect_buffer);
    }
    else
    {
        // Non-indexed.
        ASSERT(visual->data.vertex_count > 0);
        vky_draw_indirect(command_buffer, visual->indirect_buffer);
    }
}

void vky_draw_all_visuals(VkyScene* scene)
{
    log_trace("draw all visuals");
    VkyGrid* grid = scene->grid;
    ASSERT(grid != NULL);
    VkyVisualPanel vp = {0};
    VkyPanel* panel = NULL;

    VkyVisualPriority priorities[3] = {
        VKY_VISUAL_PRIORITY_FIRST, VKY_VISUAL_PRIORITY_NONE, VKY_VISUAL_PRIORITY_LAST};
    for (uint32_t priority_idx = 0; priority_idx < 3; priority_idx++)
    {
        for (uint32_t i = 0; i < grid->visual_panel_count; i++)
        {
            vp = grid->visual_panels[i];
            if (vp.priority == priorities[priority_idx])
            {
                panel = vky_get_panel(scene, vp.row, vp.col);
                vky_draw_visual(vp.visual, panel, vp.viewport_type);
            }
        }
    }
}



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

static void _scene_fill_command_buffer(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    log_trace("scene fill command buffer");
    vky_begin_render_pass(cmd_buf, canvas, canvas->scene->clear_color);
    vky_draw_all_visuals(canvas->scene);
    vky_end_render_pass(cmd_buf, canvas);
}

VkyScene* vky_create_scene(
    VkyCanvas* canvas, VkyColorBytes clear_color, uint32_t row_count, uint32_t col_count)
{
    log_trace(
        "create scene with clear color (%d, %d, %d, %d)", clear_color.r, clear_color.g,
        clear_color.b, clear_color.a);
    VkyScene* scene = calloc(1, sizeof(VkyScene));

    scene->canvas = canvas;
    canvas->scene = scene;

    scene->clear_color = clear_color;

    scene->visual_count = 0;
    scene->visuals = calloc(VKY_MAX_VISUAL_COUNT, sizeof(VkyVisual));

    scene->visual_bundle_count = 0;
    scene->visual_bundles = calloc(VKY_MAX_VISUAL_BUNDLE_COUNT, sizeof(VkyVisualBundle));

    // Sanity check.
    log_trace("create grid with %d row(s) and %d col(s)", row_count, col_count);
    ASSERT(row_count <= 1000);
    ASSERT(col_count <= 1000);

    // Create the grid.
    scene->grid = vky_create_grid(scene, row_count, col_count);

    // Dynamic uniform buffer MVP.
    scene->grid->dynamic_buffer = vky_create_dynamic_uniform_buffer(
        canvas->gpu, canvas->image_count, 2 * VKY_MAX_CAMERA_COUNT, VKY_MVP_BUFFER_SIZE);

    // Regular grid, can be override by custom calls to vky_add_panel()
    vky_set_regular_grid(scene, GLM_VEC4_ZERO);
    vky_reset_all_mvp(scene);

    // Draw all visuals in the callback that recreates the command buffers.
    canvas->cb_fill_command_buffer = _scene_fill_command_buffer;
    canvas->need_refill = true;
    canvas->cb_resize = _controller_resize_callback;

    // Controller activity responding to mouse is defined in this callback function.
    vky_add_frame_callback(canvas, _controller_callback);

    // Show FPS
    if (VKY_FPS)
    {
        VkyGui* gui = vky_create_gui(canvas, (VkyGuiParams){VKY_GUI_FIXED_TR, "FPS"});
        vky_gui_fps(gui);
    }

    return scene;
}

void vky_clear_color(VkyScene* scene, VkyColorBytes clear_color)
{
    scene->clear_color = clear_color;
    scene->canvas->need_refill = true;
}

VkyGrid* vky_create_grid(VkyScene* scene, uint32_t row_count, uint32_t col_count)
{
    // Create the grid.
    ASSERT(row_count > 0);
    ASSERT(col_count > 0);

    VkyGrid* grid = calloc(1, sizeof(VkyGrid));
    grid->row_count = row_count;
    grid->col_count = col_count;

    // Setup the grid.
    grid->xs = calloc(VKY_MAX_GRID_VIEWPORTS, sizeof(float));
    grid->ys = calloc(VKY_MAX_GRID_VIEWPORTS, sizeof(float));
    grid->widths = calloc(VKY_MAX_GRID_VIEWPORTS, sizeof(float));
    grid->heights = calloc(VKY_MAX_GRID_VIEWPORTS, sizeof(float));

    // Regular sizes by default.
    for (uint32_t i = 0; i < row_count; i++)
    {
        grid->ys[i] = (float)i / row_count;
        grid->heights[i] = 1.0f / row_count;
    }
    for (uint32_t j = 0; j < col_count; j++)
    {
        grid->xs[j] = (float)j / col_count;
        grid->widths[j] = 1.0f / col_count;
    }

    // Create the panels.
    grid->panels = calloc(VKY_MAX_GRID_VIEWPORTS, sizeof(VkyPanel));

    // Keep track of where to draw each visual.
    grid->visual_panels = calloc(VKY_MAX_GRID_VIEWPORTS, sizeof(VkyVisualPanel));

    return grid;
}

void vky_set_regular_grid(VkyScene* scene, vec4 margins)
{
    ASSERT(scene->grid != NULL);
    for (uint32_t i = 0; i < scene->grid->row_count; i++)
    {
        for (uint32_t j = 0; j < scene->grid->col_count; j++)
        {
            vky_add_panel(scene, i, j, 1, 1, margins, VKY_CONTROLLER_NONE);
        }
    }
}

static void _set_panel_viewport(VkyPanel* panel)
{
    VkyGrid* grid = panel->scene->grid;

    float x = grid->xs[panel->col];
    float y = grid->ys[panel->row];
    float w = grid->widths[panel->col];
    float h = grid->heights[panel->row];
    uint32_t hspan = panel->hspan;
    uint32_t vspan = panel->vspan;

    VkyViewport viewport = {panel->scene->canvas, x, y, w * hspan, h * vspan};
    panel->viewport = viewport;
}

void vky_set_grid_widths(VkyScene* scene, const float* widths)
{
    VkyGrid* grid = scene->grid;
    uint32_t n = grid->row_count;
    float total = 0.0f;
    for (uint32_t i = 0; i < n; i++)
    {
        float width = widths[i];
        if (width == 0.0f)
            width = 1.0f / n;
        ASSERT(width > 0);
        grid->xs[i] = total;
        grid->widths[i] = width;
        total += width;
    }
    // Renormalize.
    for (uint32_t i = 0; i < n; i++)
    {
        grid->xs[i] /= total;
        grid->widths[i] /= total;
    }
    // Need to update the panel's viewport.
    for (uint32_t i = 0; i < grid->panel_count; i++)
    {
        _set_panel_viewport(&grid->panels[i]);
    }
}

void vky_set_grid_heights(VkyScene* scene, const float* heights)
{
    VkyGrid* grid = scene->grid;
    uint32_t n = grid->row_count;
    float total = 0.0f;
    for (uint32_t i = 0; i < n; i++)
    {
        float height = heights[i];
        if (height == 0.0f)
            height = 1.0f / n;
        ASSERT(height > 0);
        grid->ys[i] = total;
        grid->heights[i] = height;
        total += height;
    }
    // Renormalize.
    for (uint32_t i = 0; i < n; i++)
    {
        grid->ys[i] /= total;
        grid->heights[i] /= total;
    }
    // Need to update the panel's viewport.
    for (uint32_t i = 0; i < grid->panel_count; i++)
    {
        _set_panel_viewport(&grid->panels[i]);
    }
}

void vky_add_panel(
    VkyScene* scene, uint32_t row, uint32_t col, uint32_t vspan, uint32_t hspan, vec4 margins,
    VkyControllerType controller_type)
{
    VkyGrid* grid = scene->grid;

    // Create the panel struct.
    VkyPanel panel = {0};
    panel.row = row;
    panel.col = col;
    panel.vspan = vspan;
    panel.hspan = hspan;
    panel.scene = scene;
    glm_vec4_copy(margins, panel.margins);

    // Set the panel's viewport.
    _set_panel_viewport(&panel);

    // NOTE: there are 2 DUBO locations, the inner viewport is first, the outer viewport
    // second.
    panel.inner_uniform_index = 2 * grid->panel_count;
    panel.outer_uniform_index = panel.inner_uniform_index + 1;
    ASSERT(panel.outer_uniform_index > 0);

    // Create the panel controller.
    vky_set_controller(&panel, controller_type, NULL);

    // Add the panel to the scene grid.
    grid->panels[grid->panel_count] = panel;
    grid->panel_count++;
}

void vky_set_panel_aspect_ratio(VkyPanel* panel, float aspect_ratio)
{
    panel->aspect_ratio = aspect_ratio;
}

void vky_destroy_axes(VkyAxes* axes)
{
    if (axes != NULL)
    {
        if (axes->tick_data != NULL)
            free(axes->tick_data);
        if (axes->text_data != NULL)
            free(axes->text_data);
        if (axes->str_buffer != NULL)
            free(axes->str_buffer);
        if (axes->panzoom_outer != NULL)
            free(axes->panzoom_outer);
        if (axes->panzoom_inner != NULL)
            free(axes->panzoom_inner);
        free(axes);
    }
}

void vky_destroy_scene(VkyScene* scene)
{
    if (scene == NULL)
    {
        log_trace("skip destroy scene as it was probably already destroyed");
        return;
    }

    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        vky_destroy_visual(&scene->visuals[i]);
    }
    for (uint32_t i = 0; i < scene->visual_bundle_count; i++)
    {
        vky_destroy_visual_bundle(&scene->visual_bundles[i]);
    }
    vky_destroy_dynamic_uniform_buffer(&scene->grid->dynamic_buffer);

    free(scene->visuals);
    scene->visuals = NULL;

    free(scene->visual_bundles);
    scene->visual_bundles = NULL;

    for (uint32_t i = 0; i < scene->grid->panel_count; i++)
    {
        vky_destroy_controller(&scene->grid->panels[i]);
    }

    free(scene->grid->panels);
    scene->grid->panels = NULL;

    free(scene->grid->xs);
    free(scene->grid->ys);
    free(scene->grid->widths);
    free(scene->grid->heights);
    free(scene->grid->visual_panels);

    free(scene->grid);
    scene->grid = NULL;

    vky_destroy_guis();

    scene->canvas->scene = NULL;
    free(scene);
}
