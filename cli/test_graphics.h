#ifndef DVZ_TEST_GRAPHICS_HEADER
#define DVZ_TEST_GRAPHICS_HEADER

#include "utils.h"



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_dynamic(TestContext* context);
int test_graphics_3D(TestContext* context);
int test_graphics_depth(TestContext* context);

// Basic graphics.
int test_graphics_point(TestContext* context);
int test_graphics_line(TestContext* context);
int test_graphics_line_strip(TestContext* context);
int test_graphics_triangle(TestContext* context);
int test_graphics_triangle_strip(TestContext* context);
int test_graphics_triangle_fan(TestContext* context);

// 2D graphics.
int test_graphics_marker_1(TestContext* context);
int test_graphics_marker_screenshots(TestContext* context);
int test_graphics_segment(TestContext* context);
int test_graphics_path(TestContext* context);
int test_graphics_text(TestContext* context);
int test_graphics_image_1(TestContext* context);
int test_graphics_image_cmap(TestContext* context);

// 3D graphics.
int test_graphics_volume_slice(TestContext* context);
int test_graphics_volume_1(TestContext* context);
int test_graphics_mesh_1(TestContext* context);
int test_graphics_mesh_2(TestContext* context);



#endif
