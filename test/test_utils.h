#include <visky/transform.h>


/*************************************************************************************************/
/*  Utils tests                                                                                  */
/*************************************************************************************************/

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
        return 1;

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
