#include "test_transforms.h"
#include "../include/visky/panel.h"
#include "../include/visky/transforms.h"
#include "../src/transforms_utils.h"

#define EPS 1e-6



/*************************************************************************************************/
/* Transform tests                                                                               */
/*************************************************************************************************/

int test_transforms_1(TestContext* context)
{
    const uint32_t n = 10000;
    const double eps = 1e-3;

    // Compute the data bounds of an array of dvec3.
    VklArray pos_in = vkl_array(n, VKL_DTYPE_DOUBLE);
    double* positions = (double*)pos_in.data;
    for (uint32_t i = 0; i < n; i++)
        positions[i] = -5 + 10 * vkl_rand_float();

    VklBox box = _box_bounding(&pos_in);
    AT(fabs(box.p0[0] + 5) < eps);
    AT(fabs(box.p1[0] - 5) < eps);

    box = _box_cube(box);
    // AT(fabs(box.p0[0] + 2.5) < eps);
    // AT(fabs(box.p1[0] - 7.5) < eps);
    // AT(fabs(box.p0[1] - 3.5) < eps);
    // AT(fabs(box.p1[1] - 13.5) < eps);
    AT(fabs(box.p0[0] + 5) < eps);
    AT(fabs(box.p1[0] - 5) < eps);


    // // Normalize the data.
    // VklArray pos_out = vkl_array(n, VKL_DTYPE_DOUBLE);
    // _transform_linear(box, &pos_in, VKL_BOX_NDC, &pos_out);
    // positions = (double*)pos_out.data;
    // double* pos = NULL;
    // double v = 0;
    // for (uint32_t i = 0; i < n; i++)
    // {
    //     pos = vkl_array_item(&pos_out, i);
    //     //     v = (*pos)[0];
    //     //     AT(-1 <= v && v <= +1);
    //     //     v = (*pos)[1];
    //     //     AT(-1 <= v && v <= +1);
    //     v = (*pos);
    //     AT(-1 - eps <= v && v <= +1 + eps);
    // }

    vkl_array_destroy(&pos_in);
    // vkl_array_destroy(&pos_out);

    return 0;
}



int test_transforms_2(TestContext* context)
{
    VklBox box0 = {{0, 0, -1}, {10, 10, 1}};
    VklTransform tr = _transform_interp(box0, VKL_BOX_NDC);

    {
        dvec3 in = {0, 0, -1};
        dvec3 out = {0};
        _transform_apply(&tr, in, out);
        for (uint32_t i = 0; i < 3; i++)
            AT(out[i] == -1);
    }

    {
        dvec3 in = {10, 10, 1};
        dvec3 out = {0};
        _transform_apply(&tr, in, out);
        for (uint32_t i = 0; i < 3; i++)
            AT(out[i] == +1);
    }

    tr = _transform_inv(&tr);

    {
        dvec3 in = {-1, -1, -1};
        dvec3 out = {0};
        _transform_apply(&tr, in, out);
        AT(out[0] == 0);
        AT(out[1] == 0);
        AT(out[2] == -1);
    }

    {
        dvec3 in = {1, 1, 1};
        dvec3 out = {0};
        _transform_apply(&tr, in, out);
        AT(out[0] == 10);
        AT(out[1] == 10);
        AT(out[2] == 1);
    }

    return 0;
}



int test_transforms_3(TestContext* context)
{
    VklMVP mvp = {0};

    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);

    glm_rotate(mvp.model, M_PI, (vec3){0, 1, 0});

    VklTransform tr = _transform_mvp(&mvp);

    dvec3 in = {0.5, 0.5, 0};
    dvec3 out = {0};
    _transform_apply(&tr, in, out);
    AC(out[0], -.5, EPS);
    AC(out[1], -.5, EPS);
    AC(out[2], +.5, EPS);

    return 0;
}



int test_transforms_4(TestContext* context)
{
    VklBox box0 = {{0, 0, -1}, {10, 10, 1}};
    VklBox box1 = {{0, 0, 0}, {1, 2, 3}};
    VklTransform tr = _transform_interp(box0, VKL_BOX_NDC);

    VklTransformChain tc = _transforms();
    _transforms_append(&tc, tr);
    tr = _transform_interp(VKL_BOX_NDC, box1);
    _transforms_append(&tc, tr);

    {
        dvec3 in = {10, 10, 1}, out = {0};
        _transforms_apply(&tc, in, out);
        AT(out[0] == 1);
        AT(out[1] == 2);
        AT(out[2] == 3);
    }

    {
        dvec3 in = {1, 2, 3}, out = {0};
        tc = _transforms_inv(&tc);
        _transforms_apply(&tc, in, out);
        AT(out[0] == 10);
        AT(out[1] == 10);
        AT(out[2] == 1);
    }

    return 0;
}



int test_transforms_5(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    VklGrid grid = vkl_grid(canvas, 2, 4);
    VklPanel* panel = vkl_panel(&grid, 1, 2);

    vkl_app_run(app, 3);

    panel->data_coords.box.p0[0] = 0;
    panel->data_coords.box.p0[1] = 0;
    panel->data_coords.box.p0[2] = -1;

    panel->data_coords.box.p1[0] = 10;
    panel->data_coords.box.p1[1] = 20;
    panel->data_coords.box.p1[2] = 1;

    dvec3 in, out;
    in[0] = 5;
    in[1] = 10;
    in[2] = 0;

    vkl_transform(panel, VKL_CDS_DATA, in, VKL_CDS_SCENE, out);
    AC(out[0], 0, EPS);
    AC(out[1], 0, EPS);
    AC(out[2], 0, EPS);

    vkl_transform(panel, VKL_CDS_DATA, in, VKL_CDS_VULKAN, out);
    AC(out[0], 0, EPS);
    AC(out[1], 0, EPS);
    AC(out[2], 0.5, EPS);

    in[0] = 0;
    in[1] = 20;
    in[2] = 0;
    uvec2 size = {0};

    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
    vkl_transform(panel, VKL_CDS_DATA, in, VKL_CDS_FRAMEBUFFER, out);
    AC(out[0], size[0] / 2., EPS);
    AC(out[1], size[1] / 2., EPS);
    AC(out[2], 0.5, EPS);

    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    vkl_transform(panel, VKL_CDS_DATA, in, VKL_CDS_WINDOW, out);
    AC(out[0], size[0] / 2., EPS);
    AC(out[1], size[1] / 2., EPS);
    AC(out[2], 0.5, EPS);

    TEST_END
}
