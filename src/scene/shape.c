/*************************************************************************************************/
/*  Shape                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/shape.h"
#include "scene/array.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Shape functions                                                                              */
/*************************************************************************************************/

void dvz_shape_destroy(DvzShape* shape)
{
    ANN(shape);
    FREE(shape->pos);
    FREE(shape->index);
    FREE(shape->color);
    FREE(shape->normal);
}



/*************************************************************************************************/
/*  2D shapes                                                                                    */
/*************************************************************************************************/

DvzShape dvz_shape_square(cvec4 color)
{
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_SQUARE;
    shape.vertex_count = 6;
    shape.color_count = 1;

    // Position.
    float x = .5;
    shape.pos = (vec3*)calloc(shape.vertex_count, sizeof(vec3));
    memcpy(
        shape.pos,
        (vec3[]){
            {-x, -x, 0},
            {+x, -x, 0},
            {+x, +x, 0},
            {+x, +x, 0},
            {-x, +x, 0},
            {-x, -x, 0},
        },
        shape.vertex_count * sizeof(vec3));

    // Color.
    shape.color = (cvec4*)calloc(shape.color_count, sizeof(cvec4));
    memcpy(shape.color, color, sizeof(cvec4));

    return shape;
}



DvzShape dvz_shape_disc(uint32_t count, cvec4 color)
{
    ASSERT(count > 0);

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_DISC;

    const uint32_t triangle_count = count;
    const uint32_t vertex_count = triangle_count + 1;
    const uint32_t index_count = 3 * triangle_count;

    // Position.
    vec3* pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    // NOTE: start at i=1 because the first vertex is the origin (0,0)
    for (uint32_t i = 1; i < vertex_count; i++)
    {
        pos[i][0] = .5 * cos(M_2PI * (float)i / (vertex_count - 2));
        pos[i][1] = .5 * sin(M_2PI * (float)i / (vertex_count - 2));
    }
    shape.pos = pos;

    // Color.
    shape.color = (cvec4*)calloc(shape.color_count, sizeof(cvec4));
    memcpy(shape.color, color, sizeof(cvec4));

    // Index.
    DvzIndex* index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    for (uint32_t i = 0; i < vertex_count - 1; i++)
    {
        ASSERT(3 * i + 2 < index_count);
        index[3 * i + 0] = 0;
        index[3 * i + 1] = i + 1;
        index[3 * i + 2] = 1 + (i + 1) % triangle_count;
    }
    shape.index = index;

    return shape;
}



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

DvzShape dvz_shape_cube(cvec4* colors)
{
    ANN(colors);
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CUBE;
    // TODO
    return shape;
}



DvzShape dvz_shape_sphere(uint32_t rows, uint32_t cols, cvec4 color)
{
    ASSERT(rows > 0);
    ASSERT(cols > 0);

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_SPHERE;
    // TODO
    return shape;
}



DvzShape dvz_shape_cone(uint32_t count, cvec4 color)
{
    ASSERT(count > 0);
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CONE;
    // TODO
    return shape;
}



DvzShape dvz_shape_cylinder(uint32_t count, cvec4 color)
{
    ASSERT(count > 0);
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CYLINDER;
    // TODO
    return shape;
}
