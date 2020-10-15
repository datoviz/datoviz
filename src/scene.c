#include "../include/visky/visky.h"
#include "axes.h"
#include "axes_3D.h"
#include "transform.h"


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

VkyPick vky_pick(VkyScene* scene, vec2 canvas_coords, VkyPanel* panel)
{
    VkyPick pick = {0};
    if (panel == NULL)
        panel = vky_panel_from_mouse(scene, canvas_coords);

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
        if (vp.panel == panel)
            return vp.visual;
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

    VkyVisual* colorbar = vky_visual_colorbar(panel->scene, params->colorbar);
    vky_add_visual_to_panel(colorbar, panel, VKY_VIEWPORT_OUTER, VKY_VISUAL_PRIORITY_NONE);
}

static void add_label(VkyPanel* panel, VkyAxis axis, VkyAxes2DParams* params)
{
    ASSERT(params != NULL);

    VkyVisual* label = vky_visual_text(panel->scene);
    vky_add_visual_to_panel(label, panel, VKY_VIEWPORT_OUTER, VKY_VISUAL_PRIORITY_NONE);
    char* str = axis == VKY_AXIS_X ? params->xlabel.label : params->ylabel.label;
    uint32_t str_len = strlen(str);

    VkyTextData* label_data = calloc(str_len, sizeof(VkyTextData));
    switch (axis)
    {

    case VKY_AXIS_X:
        params->margins[2] += VKY_AXES_LABEL_VMARGIN;
        for (uint32_t j = 0; j < str_len; j++)
        {
            label_data[j] = (VkyTextData){
                {0, +1, 0}, // label position
                {0,
                 -params->margins[2] + VKY_AXES_FONT_SIZE + VKY_AXES_LABEL_VMARGIN * .9}, // shift
                params->xlabel.color,                                                     //
                params->xlabel.font_size,                                                 //
                {0, +1},                                                                  // anchor
                0, // angle (horizontal)
                str[j],
                VKY_TRANSFORM_MODE_STATIC,
            };
        }
        break;

    case VKY_AXIS_Y:
        params->margins[3] += VKY_AXES_LABEL_HMARGIN;
        for (uint32_t j = 0; j < str_len; j++)
        {
            label_data[j] = (VkyTextData){
                {-1, 0, 0}, // label position
                // shift
                {VKY_AXES_LABEL_HMARGIN, 0},
                params->ylabel.color,     //
                params->ylabel.font_size, //
                {0, -1},                  // anchor
                M_PI / 2,                 // angle (vertical)
                str[j],
                VKY_TRANSFORM_MODE_STATIC,
            };
        }
        break;

    default:
        log_error("unknown VKY_AXIS=%d", axis);
        break;
    }

    label->data.item_count = str_len;
    label->data.items = label_data;
    vky_visual_data_raw(label);
    free(label_data);
}

void vky_set_controller(VkyPanel* panel, VkyControllerType controller_type, const void* params)
{
    if (panel->controller_type == controller_type)
    {
        log_debug("the controller type has been already set, skipping.");
        return;
    }
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
    visual.props = calloc(VKY_MAX_VISUAL_PROP_COUNT, sizeof(VkyVisualProp));
    visual.resources = calloc(VKY_MAX_VISUAL_RESOURCES, sizeof(void*));
    visual.children = calloc(VKY_MAX_VISUALS_CHILDREN, sizeof(void*));

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

void vky_visual_add_child(VkyVisual* parent, VkyVisual* child)
{
    child->parent = parent;
    parent->children[parent->children_count] = child;
    parent->children_count++;
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

void vky_set_color_context(VkyPanel* panel, VkyColormap cmap, uint8_t constant)
{
    // 32-colors palettes need special care as there is a discrepancy with the tex row.
    if (cmap >= CPAL032_OFS)
    {
        cmap = (VkyColormap)(CPAL032_OFS + ((uint8_t)cmap - CPAL032_OFS) / CPAL032_PER_ROW);
    }
    panel->color_ctx[0] = cmap;
    panel->color_ctx[1] = 0; // unused, legacy color mode
    panel->color_ctx[2] = constant;
    panel->color_ctx[3] = 0; // NOTE: currently unused
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
    // HACK: this test is verified exactly for the non-empty visuals.
    if (visual->pipeline.gpu != NULL)
    {
        vky_destroy_uniform_buffer(&visual->params_buffer);
        vky_destroy_vertex_layout(&visual->pipeline.vertex_layout);
        vky_destroy_resource_layout(&visual->pipeline.resource_layout);
        vky_destroy_shaders(&visual->pipeline.shaders);
        vky_destroy_graphics_pipeline(&visual->pipeline);
    }

    // This array is kept in memory for the duration of the visual.
    if (visual->data.items != NULL && visual->data.need_free_items)
    {
        log_trace("free visual's data.items");
        free(visual->data.items);
        visual->data.items = NULL;
    }

    if (visual->data.group_lengths != NULL)
    {
        log_trace("destroy the group lengths of the visual");
        free(visual->data.group_lengths);
        visual->data.group_lengths = NULL;
    }

    if (visual->data.group_starts != NULL)
    {
        log_trace("destroy the group starts of the visual");
        free(visual->data.group_starts);
        visual->data.group_starts = NULL;
    }

    if (visual->data.group_params != NULL)
    {
        log_trace("destroy the group params of the visual");
        free(visual->data.group_params);
        visual->data.group_params = NULL;
    }

    free(visual->props);
    visual->props = NULL;
    free(visual->resources);
    visual->resources = NULL;
    free(visual->children);
    visual->children = NULL;
    free(visual->params);
    visual->params = NULL;
}



/*************************************************************************************************/
/*  Visual props                                                                                 */
/*************************************************************************************************/

void vky_visual_prop_spec(VkyVisual* visual, size_t item_size, size_t group_param_size)
{
    visual->item_size = item_size;
    visual->group_param_size = group_param_size;
}

VkyVisualProp* vky_visual_prop_add(VkyVisual* visual, VkyVisualPropType prop_type, size_t offset)
{
    // POS prop is handled separately since it does not fit in the data.items array.
    VkyVisualProp vp = {0};
    vp.type = prop_type;
    vp.field_offset = offset;
    visual->props[visual->prop_count] = vp;

    // Update the previous prop size depending on the current prop offset.
    if (visual->prop_count > 0)
    {
        visual->props[visual->prop_count - 1].field_size =
            offset - visual->props[visual->prop_count - 1].field_offset;
    }

    // HACK: temporarily set the current prop field size so that it fills the entire struct
    // affter its offset, thereby assuming it is the last field of the struct. Even if that's not
    // the case, there should be another call to vky_visual_prop_add() anyway, which will update
    // accordingly the field_size above. However if it is indeed the last field in the
    // struct, its field_size will be correct too.
    visual->props[visual->prop_count].field_size =
        visual->item_size - visual->props[visual->prop_count].field_offset;

    visual->prop_count++;

    // Keep track of the number of POS_GPU props in the visual.
    if (prop_type == VKY_VISUAL_PROP_POS)
        visual->pos_prop_count++;

    return &visual->props[visual->prop_count - 1];
}

VkyVisualProp*
vky_visual_prop_get(VkyVisual* visual, VkyVisualPropType prop_type, uint32_t prop_index)
{
    uint32_t k = 0;

    // Search the #prop_index-th prop with the requested type.
    for (uint32_t i = 0; i < visual->prop_count; i++)
    {
        if (visual->props[i].type == prop_type)
        {
            if (k == prop_index)
                return &visual->props[i];
            else
                k++;
        }
    }
    ASSERT(prop_index >= k);

    // Search among the children.
    VkyVisualProp* vp = NULL;
    for (uint32_t i = 0; i < visual->children_count; i++)
    {
        ASSERT(prop_index >= k);
        vp = vky_visual_prop_get(visual->children[i], prop_type, prop_index - k);
        if (vp != NULL)
            return vp;
        else
        {
            // HACK: we must add the number of props in the child visual with the requested prop
            // type
            uint32_t l = 0;
            for (uint32_t j = 0; j < visual->children[i]->prop_count; j++)
                if (visual->children[i]->props[j].type == prop_type)
                    l++;
            k += l;
        }
    }

    return NULL;
}



/*************************************************************************************************/
/*  Visual data                                                                                  */
/*************************************************************************************************/

static void _visual_prop_has_been_set(VkyVisual* visual, VkyVisualProp* vp)
{
    if (visual == NULL)
        return;

    vp->is_set = true;
    // Tag the visual for data upload at the next frame.
    visual->need_data_upload = true;
    if (visual->scene)
        visual->scene->canvas->need_data_upload = true;

    // Special handling of POS2D prop: all children should be invalidated.
    if (vp != NULL && vp->type == VKY_VISUAL_PROP_POS)
    {
        visual->need_pos_rescale = true;
        for (uint32_t i = 0; i < visual->children_count; i++)
            _visual_prop_has_been_set(visual->children[i], vp);
    }
}


static void* _reallocate(void* old, size_t old_size, size_t new_size)
{
    ASSERT(new_size > 0);
    void* out = NULL;
    if (new_size > 0)
        out = calloc(new_size, 1);
    if (out != NULL && old_size > 0)
        memcpy(out, old, old_size);
    if (old != NULL)
        free(old);
    return out;
}


static void _reallocate_pos_props(VkyVisual* visual, uint32_t item_count)
{
    // Reallocate all pos props.
    VkyVisualProp* vp = NULL;
    for (uint32_t i = 0; i < visual->pos_prop_count; i++)
    {
        vp = vky_visual_prop_get(visual, VKY_VISUAL_PROP_POS, i);
        vp->values = _reallocate(
            vp->values, visual->data.item_count * sizeof(dvec2), item_count * sizeof(dvec2));
    }
}


static void _renormalize_pos(VkyVisual* visual, VkyPanel* panel)
{
    // Go through the different POS props of the visual.
    for (uint32_t i = 0; i < visual->pos_prop_count; i++)
    {
        VkyVisualProp* vp_pos_gpu = vky_visual_prop_get(visual, VKY_VISUAL_PROP_POS_GPU, i);
        VkyVisualProp* vp_pos = vky_visual_prop_get(visual, VKY_VISUAL_PROP_POS, i);

        if (vp_pos_gpu == NULL || vp_pos == NULL || !vp_pos->is_set)
        {
            log_warn("POS or POS_GPU prop was null or not set for visual %d", visual->visual_type);
            return;
        }

        // Now we renormalize.
        ASSERT(vp_pos_gpu != NULL && vp_pos != NULL);
        ASSERT(vp_pos->is_set);
        ASSERT(panel != NULL);
        vec3* pos_out = calloc(visual->data.item_count, sizeof(vec3));

        switch (panel->controller_type)
        {
        case VKY_CONTROLLER_AXES_2D:;
            VkyAxes* axes = (VkyAxes*)panel->controller;

            vky_transform_cartesian(
                vky_axes_get_range(axes), visual->data.item_count, vp_pos_gpu->values, pos_out);
            // (void*)((int64_t)visual->data.items + (int64_t)vp_pos_gpu->field_offset));

            // TODO: other controllers
            // - log scale
            // - polar
            // - web mercator

            break;

        default:
            log_error(
                "normalization with controller type %d not implemented yet",
                panel->controller_type);
            // success = false;
            break;
        }

        // Set the POS_GPU prop to the normalized values.
        ASSERT(vp_pos_gpu->field_size == sizeof(vec3));
        vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, i, visual->data.item_count, pos_out);

        // Set it also for the children.
        for (uint32_t j = 0; j < visual->children_count; j++)
        {
            vky_visual_data(
                visual->children[j], VKY_VISUAL_PROP_POS_GPU, i, visual->data.item_count, pos_out);
        }

        free(pos_out);
    }
}


static void _copy_prop_values(
    VkyVisual* visual, VkyVisualProp* vp,     //
    uint32_t first_item, uint32_t item_count, // part of the VkyData.items array to update
    uint32_t value_count, const void* values) // input array
{
    ASSERT(item_count > 0);
    ASSERT(value_count > 0);
    ASSERT(values != NULL);
    ASSERT(first_item < visual->data.item_count);
    ASSERT(item_count <= visual->data.item_count);
    ASSERT(first_item + item_count <= visual->data.item_count);
    ASSERT(value_count <= item_count);
    ASSERT(visual->data.items != NULL);

    int64_t offset = (int64_t)vp->field_offset;
    int64_t item_size = (int64_t)vp->field_size;
    int64_t out_stride = (int64_t)visual->item_size;

    ASSERT(offset >= 0);
    ASSERT(item_size > 0);
    ASSERT(out_stride > 0);

    int64_t in_byte = (int64_t)values;
    int64_t out_byte = (int64_t)visual->data.items + first_item * out_stride + offset;

    for (uint32_t i = 0; i < item_count; i++)
    {
        memcpy((void*)out_byte, (const void*)in_byte, (size_t)item_size);
        // NOTE: if there are less values in the prop than items, it will copy the last prop
        // value on all remaining items.
        if (i < value_count - 1)
            in_byte += item_size;
        out_byte += out_stride;
    }
}


static void _copy_pos_prop_values(
    VkyVisual* visual, VkyVisualProp* vp,     //
    uint32_t first_item, uint32_t item_count, // part of the VkyData.items array to update
    uint32_t value_count, const void* values)
{
    // TODO
}


static void _bake(VkyVisual* visual)
{
    ASSERT(visual != NULL);
    VkyData* data = &visual->data;
    ASSERT(visual->cb_bake_data != NULL);
    ASSERT(data->item_count > 0);
    ASSERT(data->items != NULL);

    // Create trivial group if there are no groups.
    if (data->group_count == 0)
        data->group_count = 1;
    if (data->group_lengths == 0)
    {
        data->group_lengths = calloc(1, sizeof(uint32_t));
        data->group_lengths[0] = data->item_count;
    }
    if (data->group_starts == 0)
    {
        data->group_starts = calloc(1, sizeof(uint32_t));
    }

    // This call fills the vertices and possibly indices.
    visual->cb_bake_data(visual);
    ASSERT(data->vertex_count > 0);
    ASSERT(data->vertices != NULL);
}


void vky_visual_data_raw(VkyVisual* visual)
{
    // Bake the data and save the size and pointers of the baked vertex/index buffers into the
    // data struct.
    ASSERT(visual != NULL);
    VkyData* data = &visual->data;

    // Determine whether the visual has a baking process.
    bool has_bake = visual->cb_bake_data != NULL;
    bool has_items = data->item_count > 0 && data->items != NULL;
    bool has_vertices = data->vertex_count > 0 && data->vertices != NULL;
    bool has_indices = data->index_count > 0 && data->indices != NULL;

    // Bake the data from VkyData.items if there are no vertices yet.
    if (has_items)
    {
        if (has_bake)
        {
            _bake(visual);
        }
        else
        {
            // If there is no baking function, we assume the data.items array goes straight
            // to the GPU.
            data->vertex_count = data->item_count;
            data->vertices = data->items;
            // NOTE: since items will be freed anyway, we should not free the vertices.
            visual->data.need_free_vertices = false;
        }
    }

    // At this point, the vertices have been allocated and set.
    has_vertices = data->vertex_count > 0 && data->vertices != NULL;
    ASSERT(has_vertices);

    // Need to free the indices if they were created during the baking.
    has_indices = data->index_count > 0 && data->indices != NULL;

    // Allocation of the vertex and index buffers if needed.
    VkDeviceSize vertex_size = visual->pipeline.vertex_layout.stride;
    ASSERT(vertex_size > 0);
    VkDeviceSize vbuf_size = data->vertex_count * vertex_size;

    // Create vertex buffer if the user didn't already do it.
    if (visual->vertex_buffer.size == 0)
    {
        ASSERT(data->vertex_count > 0);
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
        VkDeviceSize ibuf_size = data->index_count * sizeof(VkyIndex);
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
    ASSERT(!has_indices || visual->index_buffer.size >= data->index_count * sizeof(VkyIndex));

    // Upload the vertices.
    vky_upload_buffer(visual->vertex_buffer, 0, vbuf_size, data->vertices);

    // Upload the indices, if any.
    if (has_indices)
        vky_upload_buffer(
            visual->index_buffer, 0, data->index_count * sizeof(VkyIndex), data->indices);

    // Indirect draw call.
    if (!has_indices)
    {
        // TODO: customizable offsets
        ASSERT(data->vertex_count > 0);
        VkDrawIndirectCommand ind = (VkDrawIndirectCommand){data->vertex_count, 1, 0, 0};
        visual->indirect_item_count = data->vertex_count;
        vky_upload_buffer(visual->indirect_buffer, 0, sizeof(ind), &ind);
    }
    else
    {
        ASSERT(data->index_count > 0);
        // log_trace("upload indirect indexed draw data %d", data->index_count);
        VkDrawIndexedIndirectCommand ind = {data->index_count, 1, 0, 0, 0};
        visual->indirect_item_count = data->index_count;
        vky_upload_buffer(visual->indirect_buffer, 0, sizeof(ind), &ind);
    }

    // Free the allocated buffers.
    if (visual->data.need_free_vertices)
    {
        log_trace("freeing data->vertices");
        free(data->vertices);
        data->vertices = NULL;
        data->need_free_vertices = false;
    }
    if (visual->data.need_free_indices)
    {
        log_trace("freeing data->indices");
        free(data->indices);
        data->indices = NULL;
        data->need_free_indices = false;
    }
}


void vky_visual_data_set_size(
    VkyVisual* visual, uint32_t item_count, uint32_t group_count, //
    const uint32_t* group_lengths, const void* group_params)
{
    if (group_count == 0)
        group_count = 1;
    ASSERT(group_count > 0);
    ASSERT(item_count > 0);

    VkyData* data = &visual->data;


    // Allocate and fill VkyData.group_lengths.
    data->group_count = group_count;

    // Reallocate the group lengths array if needed.
    data->group_lengths =
        (uint32_t*)_reallocate(data->group_lengths, 0, group_count * sizeof(uint32_t));
    ASSERT(data->group_lengths != NULL);
    if (group_lengths != NULL)
        memcpy(data->group_lengths, group_lengths, group_count * sizeof(uint32_t));
    if (group_count <= 1)
        data->group_lengths[0] = item_count;

    // Copy the group params, if any.
    if (group_params != NULL)
    {
        ASSERT(visual->group_param_size > 0);
        data->group_params =
            _reallocate(data->group_params, 0, group_count * visual->group_param_size);
        memcpy(data->group_params, group_params, group_count * visual->group_param_size);
    }

    // Reallocate the group starts.
    data->group_starts =
        (uint32_t*)_reallocate(data->group_starts, 0, group_count * sizeof(uint32_t));

    // Count the total number of items across all groups.
    uint32_t group_item_count = 0;
    for (uint32_t i = 0; i < group_count; i++)
    {
        data->group_starts[i] = group_item_count;
        group_item_count += data->group_lengths[i];
        ASSERT(data->group_lengths[i] > 0);
        if (i > 0)
            ASSERT(data->group_starts[i] > data->group_starts[i - 1]);
    }
    ASSERT(group_item_count == item_count);
    ASSERT(
        data->group_starts[group_count - 1] + data->group_lengths[group_count - 1] == item_count);


    // Allocate the VkyData.items array.
    // 3 cases depending on whether the current data.items exists or not, and if so, is larger or
    // smaller.
    if (item_count < data->item_count)
    {
        // Do not reallocate here, just reduce data->item_count accordingly.
        data->item_count = item_count;
    }
    else if (item_count > data->item_count)
    {
        // Need to reallocate and copy the old array into the new one
        data->items = _reallocate(
            data->items, data->item_count * visual->item_size, item_count * visual->item_size);
        data->item_count = item_count;
        data->need_free_items = true;

        // Allocate the position arrays corresponding to the POS props.
        _reallocate_pos_props(visual, item_count);
    }
    else
    {
        // Nothing to do if the requested data.items has the same size as the existing one.
        ASSERT(item_count == data->item_count);
    }

    // Resize the children visuals too.
    for (uint32_t i = 0; i < visual->children_count; i++)
    {
        vky_visual_data_set_size(
            visual->children[i], item_count, group_count, group_lengths, group_params);
    }
}


void vky_visual_data(
    VkyVisual* visual, VkyVisualPropType prop_type, uint32_t prop_index, //
    uint32_t value_count, const void* values)
{
    vky_visual_data_partial(
        visual, prop_type, prop_index, 0, visual->data.item_count, value_count, values);
}


void vky_visual_data_partial(
    VkyVisual* visual, VkyVisualPropType prop_type, uint32_t prop_index, //
    uint32_t first_item, uint32_t item_count, uint32_t value_count, const void* values)
{
    ASSERT(value_count > 0);
    ASSERT(item_count > 0);
    ASSERT(value_count <= item_count);

    if (visual->data.items == NULL)
    {
        log_error("you need to call vky_visual_data_set_size() before calling vky_visual_data()");
        return;
    }

    // Get the visual prop object.
    VkyVisualProp* vp = vky_visual_prop_get(visual, prop_type, prop_index);
    if (vp == NULL)
    {
        log_error("could not find visual prop %d", prop_type);
        return;
    }

    // Special handling of the POS prop (we don't copy it in data.items)
    if (prop_type == VKY_VISUAL_PROP_POS)
    {
        _copy_pos_prop_values(visual, vp, first_item, item_count, value_count, values);
        return;
    }

    ASSERT(visual->data.items != NULL);
    ASSERT(visual->data.item_count > 0);
    ASSERT(first_item + item_count <= visual->data.item_count);

    if (values != NULL)
        _copy_prop_values(visual, vp, first_item, item_count, value_count, values);

    _visual_prop_has_been_set(visual, vp);
}


void vky_visual_data_group(
    VkyVisual* visual, VkyVisualPropType prop_type, uint32_t prop_index, uint32_t group_idx,
    uint32_t value_count, const void* value)
{
    ASSERT(value_count > 0);
    ASSERT(group_idx < visual->data.group_count);
    // Need to call vky_visual_data_set_size() first.
    ASSERT(visual->data.group_lengths != NULL);
    ASSERT(visual->data.group_starts != NULL);

    uint32_t group_size = visual->data.group_lengths[group_idx];
    uint32_t first_item = visual->data.group_starts[group_idx];

    VkyVisualProp* vp = vky_visual_prop_get(visual, prop_type, prop_index);

    // Repeat the value for the current group.
    ASSERT(group_size > 0);
    ASSERT(vp->field_size > 0);

    vky_visual_data_partial(
        visual, prop_type, prop_index, first_item, group_size, value_count, value);
}


void vky_visual_data_resource(
    VkyVisual* visual, VkyVisualPropType prop_type, uint32_t prop_index, void* resource)
{
    VkyVisualProp* vp = vky_visual_prop_get(visual, prop_type, prop_index);
    vp->resource = resource;
}


void vky_visual_data_callback(
    VkyVisual* visual, VkyVisualPropType prop_type, uint32_t prop_index,
    VkyVisualPropCallback callback)
{
    VkyVisualProp* vp = vky_visual_prop_get(visual, prop_type, prop_index);
    vp->callback = callback;
}


void vky_visual_data_upload(VkyVisual* visual, VkyPanel* panel)
{
    VkyVisualProp* vp = NULL;

    // call the callbacks and ensure the prop's values are all set.
    for (uint32_t i = 0; i < visual->prop_count; i++)
    {
        vp = &visual->props[i];
        if (vp->callback != NULL)
            vp->callback(vp, visual, panel);
    }

    // Renormalize the POS_GPU prop if POS is specified.
    if (visual->need_pos_rescale)
        _renormalize_pos(visual, panel);

    // The VkyData.items array is kept up to date in vky_visual_data(), called by the user.
    // Here, we send it to the visual's bake callback, which creates the vertices and indices
    // and sends them to the GPU.
    vky_visual_data_raw(visual);

    // Call the visual's children recursively.
    for (uint32_t i = 0; i < visual->children_count; i++)
    {
        vky_visual_data_upload(visual->children[i], panel);
    }
}


void vky_free_data(VkyData data)
{
    if (data.items != NULL)
        free(data.items);
    if (data.vertices != NULL)
        free(data.vertices);
    if (data.indices != NULL)
        free(data.indices);
    data.items = data.vertices = data.indices = NULL;
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
            vky_set_color_context(&scene->grid->panels[i], VKY_DEFAULT_COLORMAP, 255);
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
    if (row >= scene->grid->row_count)
    {
        log_error("row %d doesn't exist", row);
        row = scene->grid->row_count - 1;
    }
    if (col >= scene->grid->col_count)
    {
        log_error("col %d doesn't exist", col);
        col = scene->grid->col_count - 1;
    }
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
    grid->visual_panels[n] = (VkyVisualPanel){visual, panel, viewport_type, priority};
    grid->visual_panel_count++;
}

static void vky_draw_children(VkyVisual* visual, VkyPanel* panel, VkyViewportType viewport_type)
{
    for (uint32_t i = 0; i < visual->children_count; i++)
    {
        if (visual->children[i] != NULL)
            vky_draw_visual(visual->children[i], panel, viewport_type);
    }
}

void vky_draw_visual(VkyVisual* visual, VkyPanel* panel, VkyViewportType viewport_type)
{
    // Non-empty visuals.
    if (visual->pipeline.gpu != NULL)
    {
        log_trace("draw visual %d", visual->visual_type);
        VkyScene* scene = visual->scene;
        VkyCanvas* canvas = scene->canvas;
        VkyViewport viewport = vky_get_viewport(panel, viewport_type);
        VkCommandBuffer command_buffer =
            canvas->command_buffers[canvas->current_command_buffer_index];
        uint32_t buffer_index = vky_get_panel_buffer_index(panel, viewport_type);

        // Bind the vertex buffer.
        if (visual->vertex_buffer.buffer == NULL)
        {
            log_debug("no buffer, skipping draw");
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
    vky_draw_children(visual, panel, viewport_type);
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
                panel = vp.panel;
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

VkyScene*
vky_create_scene(VkyCanvas* canvas, VkyColor clear_color, uint32_t row_count, uint32_t col_count)
{
    VkyScene* scene = calloc(1, sizeof(VkyScene));

    scene->canvas = canvas;
    canvas->scene = scene;

    scene->clear_color = clear_color;

    scene->visual_count = 0;
    scene->visuals = calloc(VKY_MAX_VISUAL_COUNT, sizeof(VkyVisual));

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

void vky_clear_color(VkyScene* scene, VkyColor clear_color)
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
    vky_destroy_dynamic_uniform_buffer(&scene->grid->dynamic_buffer);

    free(scene->visuals);
    scene->visuals = NULL;

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
