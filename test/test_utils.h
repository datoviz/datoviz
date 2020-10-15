#include "../src/axes.h"
#include <visky/visky.h>


#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
        return 1;


/*************************************************************************************************/
/*  Visual tests                                                                                 */
/*************************************************************************************************/

static VkyVisual _blank_visual()
{
    VkyVisual v = {0};
    v.props = calloc(10, sizeof(VkyVisualProp));
    v.children = calloc(10, sizeof(VkyVisual*));
    return v;
}

static void _destroy_visual(VkyVisual* v)
{
    free(v->children);
    free(v->props);
}

static int test_visuals_props_1()
{
    VkyVisual v = _blank_visual();

    VkyVisualProp* vp = NULL;
    VkyVisualProp* vpc = NULL;

    // Add a prop to the visual.
    vp = vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS, 4);

    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_ALPHA, 0) == NULL);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 0) == vp);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 1) == NULL);

    // Add a child.
    v.children_count = 1;
    VkyVisual vc = _blank_visual();
    v.children[0] = &vc;

    // Add a prop to the child visual.
    vpc = vky_visual_prop_add(&vc, VKY_VISUAL_PROP_POS, 4);

    // Check that we get the correct prop, belonging either to the parent visual or the child.
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 0) == vp);
    AT(vky_visual_prop_get(&vc, VKY_VISUAL_PROP_POS, 0) == vpc);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 1) == vpc);
    AT(vky_visual_prop_get(&v, VKY_VISUAL_PROP_POS, 2) == NULL);

    _destroy_visual(&vc);
    _destroy_visual(&v);
    return 0;
}

static int test_visuals_props_2()
{
    VkyVisual v = _blank_visual();
    vky_visual_prop_spec(&v, 6, 0);
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS, 0);   // 1 byte
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_COLOR, 1); // 2 bytes
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_SIZE, 3);  // 3 bytes

    uint8_t val1[] = {10, 20, 30};
    cvec2 val2[] = {{11, 12}, {13, 14}, {15, 16}};
    // Only one cvec3 item here, but should be copied over automatically
    cvec3 val3[] = {{21, 22, 23}};

    vky_visual_data_set_size(&v, 3, 0, NULL, NULL);
    vky_visual_data(&v, VKY_VISUAL_PROP_POS, 0, 3, val1);
    AT(v.data.item_count == 3)
    vky_visual_data(&v, VKY_VISUAL_PROP_COLOR, 0, 3, val2);
    vky_visual_data(&v, VKY_VISUAL_PROP_SIZE, 0, 1, val3);
    AT(v.data.item_count == 3)

    uint8_t expected[] = {10, 11, 12, 21, 22, 23, 20, 13, 14, 21, 22, 23, 30, 15, 16, 21, 22, 23};
    AT(memcmp(v.data.items, expected, sizeof(expected)) == 0);
    return 0;
}

static int test_visuals_props_3()
{
    VkyVisual v = _blank_visual();
    vky_visual_prop_spec(&v, 3, 0);
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS, 0);   // 1 byte
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_COLOR, 1); // 2 bytes

    uint8_t val1[] = {10};
    cvec2 val2[] = {{11, 12}, {13, 14}};


    vky_visual_data_set_size(&v, 1, 0, NULL, NULL);
    AT(v.data.item_count == 1)

    vky_visual_data(&v, VKY_VISUAL_PROP_POS, 0, 1, val1);
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

    // for (uint32_t i = 0; i < v.data.item_count * v.item_size; i++)
    // {
    //     DBG(((uint8_t*)v.data.items)[i]);
    // }

    return 0;
}

static int test_visuals_props_4()
{
    VkyVisual v = _blank_visual();
    vky_visual_prop_spec(&v, 3, 0);
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS, 0); // 1 byte

    uint8_t val1[] = {10, 11, 12};
    uint8_t val2[] = {20};

    vky_visual_data_set_size(&v, 3, 0, NULL, NULL);
    vky_visual_data(&v, VKY_VISUAL_PROP_POS, 0, 3, val1);
    AT(v.data.item_count == 3)

    // Test the case where a subsequent call to vky_visual_data() decreases the number
    // of data items.
    vky_visual_data_set_size(&v, 1, 0, NULL, NULL);
    vky_visual_data(&v, VKY_VISUAL_PROP_POS, 0, 1, val2);
    AT(v.data.item_count == 1)

    uint8_t expected[] = {20};
    AT(memcmp(v.data.items, expected, sizeof(expected)) == 0);

    return 0;
}

static int test_visuals_props_5()
{
    VkyVisual v = _blank_visual();
    vky_visual_prop_spec(&v, 4, 0);
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_POS, 0);   // 1 byte
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_COLOR, 1); // 1 byte
    vky_visual_prop_add(&v, VKY_VISUAL_PROP_SIZE, 2);  // 2 bytes

    vky_visual_data_set_size(&v, 6, 3, (uint32_t[]){1, 2, 3}, NULL);
    AT(v.data.item_count == 6);

    uint8_t x = 10;
    vky_visual_data_partial(&v, VKY_VISUAL_PROP_POS, 0, 3, 2, 1, &x);

    x = 11;
    vky_visual_data_group(&v, VKY_VISUAL_PROP_POS, 0, 1, &x);

    uint8_t y[] = {20, 21};
    vky_visual_data(&v, VKY_VISUAL_PROP_COLOR, 0, 2, y);

    vky_visual_data_group(&v, VKY_VISUAL_PROP_SIZE, 0, 2, y);

    uint8_t expected[] = {0,  20, 0,  0,  11, 21, 0,  0,  11, 21, 0,  0,
                          10, 21, 20, 21, 10, 21, 20, 21, 0,  21, 20, 21};
    AT(memcmp(v.data.items, expected, 6 * 4) == 0);

    return 0;
}


/*************************************************************************************************/
/*  Transform tests                                                                              */
/*************************************************************************************************/

#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))
#define ABOX(x, a, b, c, d)                                                                       \
    AT(((x).pos_ll[0] == (a)) && ((x).pos_ll[1] == (b)) && ((x).pos_ur[0] == (c)) &&              \
       ((x).pos_ur[1] == (d)))
#define PBOX(x)                                                                                   \
    printf("%f %f %f %f\n", (x).pos_ll[0], (x).pos_ll[1], (x).pos_ur[0], (x).pos_ur[1]);

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



static int test_utils_panzoom_1()
{
    VkyApp* app = vky_create_app(VKY_BACKEND_OFFSCREEN, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, W, H);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);

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

    vky_destroy_app(app);
    return 0;
}



static int test_utils_axes_1(VkyPanel* panel)
{
    // VkyPanzoom* panzoom = ((VkyControllerAxes2D*)panel->controller)->axes->panzoom_inner;
    // VkyAxes* axes = ((VkyControllerAxes2D*)panel->controller)->axes;
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
            // PBOX(box);
            // PBOX(box_);
            // printf("\n");
            ABOX(box_, xmin, ymin, xmax, ymax);
        }
    }
    return 0;
}



static int test_utils_axes_2(VkyPanel* panel)
{
    // VkyPanzoom* panzoom = ((VkyControllerAxes2D*)panel->controller)->panzoom;
    VkyAxes* axes = (VkyAxes*)panel->controller;

    int res = 0;

    res += _check_axes_range(axes, 0, -12, 1000, 12);
    res += _check_axes_range(axes, 0, -12, 500, 12);
    res += _check_axes_range(axes, 250, -12, 750, 12);
    res += _check_axes_range(axes, 450, -6, 550, 6);

    return res;
}
