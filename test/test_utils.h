#include "../src/axes.h"
#include <visky/visky.h>


/*************************************************************************************************/
/*  Utils tests                                                                                  */
/*************************************************************************************************/

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
        return 1;

#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))

#define W 1200
#define H 800


static int test_utils_transform_1()
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



static int test_utils_transform_2()
{
    VkyApp* app = vky_create_app(VKY_BACKEND_OFFSCREEN, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, W, H);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 2, 4);
    VkyPanel* panel = vky_get_panel(scene, 1, 2);

    VkyAxes2DParams params = vky_default_axes_2D_params();
    glm_vec4_scale(params.margins, 0.25, params.margins);
    params.xscale.vmin = 0;
    params.xscale.vmax = 1000;
    params.yscale.vmin = -12;
    params.yscale.vmax = +12;

    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, &params);
    // VkyAxes* axes = ((VkyControllerAxes2D*)panel->controller)->axes;
    VkyPanzoom* panzoom = ((VkyControllerAxes2D*)panel->controller)->panzoom;

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
    AIN(out[0], 0, 0.05);
    AIN(out[1], -1, -0.95);

    vky_axes_transform_apply(&tr, (dvec2){+1, +1}, out);
    AIN(out[0], 0.49, 0.5);
    AIN(out[1], -.02, 0);

    // Canvas NDC to PX
    tr = vky_axes_transform(panel, VKY_CDS_CANVAS_NDC, VKY_CDS_CANVAS_PX);
    vky_axes_transform_apply(&tr, (dvec2){-1, -1}, out);
    AT(out[0] == 0);
    AT(out[1] == H);

    vky_axes_transform_apply(&tr, (dvec2){+1, +1}, out);
    AT(out[0] == W);
    AT(out[1] == 0);

    // Full transform.
    tr = vky_axes_transform(panel, VKY_CDS_DATA, VKY_CDS_CANVAS_PX);
    vky_axes_transform_apply(&tr, (dvec2){750, +6}, out);
    AIN(out[0], W * .5, W * .75);
    AIN(out[1], H * .5, H * 1);


    vky_destroy_app(app);
    return 0;
}



static int test_utils_transform_3()
{
    VkyApp* app = vky_create_app(VKY_BACKEND_OFFSCREEN, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, W, H);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);

    VkyAxes2DParams params = vky_default_axes_2D_params();

    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, &params);
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
    vky_panzoom_set_box(panel, panzoom, VKY_VIEWPORT_INNER, box);
    AT(panzoom->camera_pos[0] == .5);
    AT(panzoom->camera_pos[1] == .5);
    AT(panzoom->zoom[0] == 2);
    AT(panzoom->zoom[1] == 2);

    vky_destroy_app(app);
    return 0;
}
