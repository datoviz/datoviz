#include "../include/visky/panel.h"
#include "../include/visky/scene.h"
#include "../include/visky/transforms.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static VklBox _box_cds(VklPanel* panel, VklCDS cds)
{
    ASSERT(panel != NULL);
    ASSERT(panel->scene != NULL);
    VklCanvas* canvas = panel->scene->canvas;
    ASSERT(canvas != NULL);

    VklBox box = {0};
    uvec2 size = {0};

    switch (cds)
    {

    case VKL_CDS_DATA:
        return panel->data_coords.box;
        break;

    case VKL_CDS_SCENE:
        return (VklBox){{-1, -1, -1}, {+1, +1, +1}};
        break;

    case VKL_CDS_VULKAN:
        return (VklBox){{-1, -1, 0}, {+1, +1, +1}};
        break;

    case VKL_CDS_FRAMEBUFFER:
        vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
        return (VklBox){{0, 0, 0}, {(double)size[0], (double)size[1], 0}};
        break;

    case VKL_CDS_WINDOW:
        vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
        return (VklBox){{0, 0, 0}, {(double)size[0], (double)size[1], 0}};
        break;

    default:
        log_error("unknown coordinate system");
        break;
    }

    return box;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void vkl_transform(VklDataCoords coords, VklArray* pos_in, VklArray* pos_out)
{
    ASSERT(pos_in != NULL);
    ASSERT(pos_out != NULL);

    log_debug("data normalization on %d position elements", pos_in->item_count);

    switch (coords.transform)
    {

    case VKL_TRANSFORM_CARTESIAN:
        _transform_linear(coords.box, pos_in, VKL_BOX_NDC, pos_out);
        break;

        // TODO: other transforms
        // TODO: support for semilog/loglog plots

    default:
        break;
    }
}



void vkl_cds(VklPanel* panel, VklCDS source, dvec3 in, VklCDS target, dvec3 out)
{
    ASSERT(panel != NULL);
    ASSERT(panel->scene != NULL);

    VklCanvas* canvas = panel->scene->canvas;
    ASSERT(canvas != NULL);

    VklDataCoords* coords = &panel->data_coords;

    VklController* controller = panel->controller;

    ASSERT(controller->interact_count > 0);
    VklMVP* mvp = &controller->interacts[0].mvp;

    VklViewport viewport = panel->viewport;

    if (source == target)
        return;
    if (source == VKL_CDS_NONE || target == VKL_CDS_NONE)
    {
        log_error("undefined coordinate system");
        return;
    }

    bool source_cpu = source == VKL_CDS_DATA || source == VKL_CDS_SCENE;
    bool target_cpu = target == VKL_CDS_DATA || target == VKL_CDS_SCENE;

    VklBox box_in = _box_cds(panel, source);
    VklBox box_out = _box_cds(panel, target);

    if (source_cpu == target_cpu)
    {
        VklArray arr_in = vkl_array_point(in);
        VklArray arr_out = vkl_array_point(out);
        _transform_linear(box_in, &arr_in, box_out, &arr_out);
        memcpy(out, arr_out.data, sizeof(dvec3));
        vkl_array_destroy(&arr_in);
        vkl_array_destroy(&arr_out);
        return;
    }
    else if (source_cpu && !target_cpu)
    {
        if (source != VKL_CDS_SCENE)
        {
            dvec3 temp = {0};
            vkl_cds(panel, source, in, VKL_CDS_SCENE, temp);
            vkl_cds(panel, VKL_CDS_SCENE, temp, target, out);
            return;
        }
        ASSERT(source == VKL_CDS_SCENE);

        if (target != VKL_CDS_VULKAN)
        {
            dvec3 temp = {0};
            vkl_cds(panel, source, in, VKL_CDS_VULKAN, temp);
            vkl_cds(panel, VKL_CDS_VULKAN, temp, target, out);
            return;
        }
        ASSERT(source == VKL_CDS_VULKAN);

        // MVP matrices
        mat4 mat = {0};
        glm_mat4_mulN((mat4*[]){&mvp->proj, &mvp->view, &mvp->model}, 3, mat);
        vec4 pos = {in[0], in[1], in[2], 1};
        vec4 pos_out = {0};
        glm_mat4_mulv(mat, pos, pos_out);
        ASSERT(pos_out[3] > 0);

        // Vulkan transformation
        out[0] = pos_out[0];
        out[1] = -pos_out[1];                    // NOTE: Vulkan swaps the direction of the y axis
        out[2] = .5 * (pos_out[2] + pos_out[3]); // from [-1,1] to [0,1]
    }
    else if (!source_cpu && target_cpu)
    {
        if (source != VKL_CDS_VULKAN)
        {
            dvec3 temp = {0};
            vkl_cds(panel, source, in, VKL_CDS_VULKAN, temp);
            vkl_cds(panel, VKL_CDS_VULKAN, temp, target, out);
            return;
        }
        ASSERT(source == VKL_CDS_VULKAN);

        if (target != VKL_CDS_SCENE)
        {
            dvec3 temp = {0};
            vkl_cds(panel, source, in, VKL_CDS_VULKAN, temp);
            vkl_cds(panel, VKL_CDS_VULKAN, temp, target, out);
            return;
        }
        ASSERT(source == VKL_CDS_SCENE);

        // MVP matrices
        mat4 mat = {0};
        glm_mat4_mulN((mat4*[]){&mvp->proj, &mvp->view, &mvp->model}, 3, mat);
        mat4 mat_inv = {0};
        glm_mat4_inv(mat, mat_inv);
        vec4 pos = {in[0], in[1], in[2], 1};
        vec4 pos_out = {0};
        glm_mat4_mulv(mat_inv, pos, pos_out);
        ASSERT(pos_out[3] > 0);

        // Vulkan transformation
        out[0] = pos_out[0];
        out[1] = -pos_out[1];                 // NOTE: Vulkan swaps the direction of the y axis
        out[2] = 2 * pos_out[2] - pos_out[3]; // from [0,1] to [-1,1]
    }
    else
    {
        log_error("unknown coordinate systems");
    }

    // else if (source > target)
    // {
    //     vkl_cds(panel, out, in, target, source);
    //     return vkl_transform_inv(vkl_transform_old(panel, target, source));
    // }
    // else if (target - source >= 2)
    // {
    //     for (uint32_t k = source; k <= target - 1; k++)
    //     {
    //         tr = vkl_transform_mul(tr, vkl_transform_old(panel, (VklCDSOld)k, (VklCDSOld)(k +
    //         1)));
    //     }
    // }
    // else if (target - source == 1)
    // {
    //     switch (source)
    //     {

    //     case VKL_CDS_DATA:
    //         // linear normalization based on axes range
    //         ASSERT(target == VKL_CDS_GPU);
    //         {
    //             tr = vkl_transform_interp(ll, NDC0, ur, NDC1);
    //         }
    //         break;

    //     case VKL_CDS_GPU:
    //         // apply panzoom
    //         ASSERT(target == VKL_CDS_PANZOOM);
    //         {
    //             ASSERT(panzoom != NULL);
    //             ASSERT(panzoom->zoom[0] != 0);
    //             ASSERT(panzoom->zoom[1] != 0);
    //             dvec2 p = {panzoom->camera_pos[0], panzoom->camera_pos[1]};
    //             dvec2 s = {panzoom->zoom[0], panzoom->zoom[1]};
    //             tr.scale[0] = s[0];
    //             tr.scale[1] = s[1];
    //             tr.shift[0] = p[0]; // / s[0];
    //             tr.shift[1] = p[1]; // / s[1];
    //         }
    //         break;

    //     case VKL_CDS_PANZOOM:
    //         // using inner viewport
    //         ASSERT(target == VKL_CDS_PANEL);
    //         {
    //             // Margins.
    //             // double cw = panel->scene->canvas->size.framebuffer_width;
    //             // double ch = panel->scene->canvas->size.framebuffer_height;
    //             // uvec2 size = {0};
    //             // vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
    //             double cw = viewport.viewport.width;
    //             double ch = viewport.viewport.height;
    //             double mt = 2 * viewport.margins[0] / ch;
    //             double mr = 2 * viewport.margins[1] / cw;
    //             double mb = 2 * viewport.margins[2] / ch;
    //             double ml = 2 * viewport.margins[3] / cw;

    //             tr = vkl_transform_interp(
    //                 NDC0, (dvec2){-1 + ml, -1 + mb}, NDC1, (dvec2){+1 - mr, +1 - mt});
    //         }
    //         break;

    //     case VKL_CDS_PANEL:
    //         // multiply by canvas size
    //         ASSERT(target == VKL_CDS_CANVAS_NDC);
    //         {
    //             // From outer to inner viewport.
    //             ll[0] = -1 + 2 * viewport.viewport.x;
    //             ll[1] = +1 - 2 * (viewport.viewport.y + viewport.viewport.height);
    //             ur[0] = -1 + 2 * (viewport.viewport.x + viewport.viewport.width);
    //             ur[1] = +1 - 2 * viewport.viewport.y;

    //             tr = vkl_transform_interp(NDC0, ll, NDC1, ur);
    //         }
    //         break;

    //     case VKL_CDS_CANVAS_NDC:
    //         // multiply by canvas size
    //         ASSERT(target == VKL_CDS_CANVAS_PX);
    //         {
    //             uvec2 size = {0};
    //             vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    //             tr = vkl_transform_interp(NDC0, (dvec2){0, size[1]}, NDC1, (dvec2){size[0], 0});
    //         }
    //         break;

    //     default:
    //         log_error("unknown coordinate systems");
    //         break;
    //     }
    // }
}
