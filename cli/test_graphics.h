#ifndef VKY_TEST_GRAPHICS_HEADER
#define VKY_TEST_GRAPHICS_HEADER

#include "utils.h"



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_dynamic(TestContext* context);
int test_graphics_3D(TestContext* context);

// Basic graphics.
int test_graphics_points(TestContext* context);
int test_graphics_lines(TestContext* context);
int test_graphics_line_strip(TestContext* context);
int test_graphics_triangles(TestContext* context);
int test_graphics_triangle_strip(TestContext* context);
int test_graphics_triangle_fan(TestContext* context);
int test_graphics_marker(TestContext* context);
int test_graphics_segment(TestContext* context);
int test_graphics_text(TestContext* context);
int test_graphics_image(TestContext* context);
int test_graphics_mesh(TestContext* context);



#endif
