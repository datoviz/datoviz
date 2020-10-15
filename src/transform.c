#include "transform.h"

#define GET_VEC3(buf) ((float*)((int64_t)(buf) + (int64_t)(item_size * i)))



void vky_transform_cartesian(VkyBox2D box, uint32_t item_count, const dvec2* pos_in, vec3* pos_out)
{
    double xmin = box.pos_ll[0];
    double ymin = box.pos_ll[1];
    double xmax = box.pos_ur[0];
    double ymax = box.pos_ur[1];

    double dx = xmin < xmax ? 1. / (xmax - xmin) : 1;
    double dy = ymin < ymax ? 1. / (ymax - ymin) : 1;

    double ax = 2 * dx;
    double ay = 2 * dy;
    double bx = -(xmin + xmax) * dx;
    double by = -(ymin + ymax) * dy;

    for (uint32_t i = 0; i < item_count; i++)
    {
        pos_out[i][0] = ax * pos_in[i][0] + bx;
        pos_out[i][1] = ay * pos_in[i][1] + by;
    }
}
