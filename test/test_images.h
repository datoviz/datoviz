/*********************************************************************************************/
/*  Tests 11                                                                                 */
/*********************************************************************************************/

// Set the panel controller.
VkyAxes2DParams axparams = vky_default_axes_2D_params();

// x label
strcpy(axparams.xlabel.label, "x axis");
axparams.xlabel.axis = VKY_AXIS_X;
axparams.xlabel.color.alpha = TO_BYTE(VKY_AXES_LABEL_COLOR_A);
axparams.xlabel.font_size = 12;

// y label
strcpy(axparams.ylabel.label, "y axis");
axparams.ylabel.axis = VKY_AXIS_Y;
axparams.ylabel.color.alpha = TO_BYTE(VKY_AXES_LABEL_COLOR_A);
axparams.ylabel.font_size = 12;

axparams.colorbar.position = VKY_COLORBAR_RIGHT;
axparams.colorbar.cmap = VKY_CMAP_VIRIDIS;


/**** Blank **********************************************************************************/

TEST_11(blank, VKY_CONTROLLER_NONE, NULL, 1)
TEST_11(hello, VKY_CONTROLLER_NONE, NULL, 1)
TEST_11(triangle, VKY_CONTROLLER_NONE, NULL, 1)


/**** plot2d *********************************************************************************/

TEST_11(mesh_raw, VKY_CONTROLLER_PANZOOM, NULL, 1)
TEST_11(scatter, VKY_CONTROLLER_AXES_2D, NULL, 1)
axparams.user.colors[0][3] = 0;
TEST_11(imshow, VKY_CONTROLLER_AXES_2D, &axparams, 1)
axparams.user.colors[0][3] = 1;
TEST_11(arrows, VKY_CONTROLLER_AXES_2D, NULL, 1)
TEST_11(paths, VKY_CONTROLLER_AXES_2D, NULL, 1)
TEST_11(hist, VKY_CONTROLLER_AXES_2D, NULL, 1)
TEST_11(area, VKY_CONTROLLER_AXES_2D, NULL, 1)
TEST_11(axrect, VKY_CONTROLLER_AXES_2D, NULL, 1)
TEST_11(raster, VKY_CONTROLLER_AXES_2D, NULL, 1)
TEST_11(segments, VKY_CONTROLLER_AXES_2D, NULL, 1)
TEST_11(graph, VKY_CONTROLLER_AXES_2D, NULL, 1)
TEST_11(text, VKY_CONTROLLER_PANZOOM, NULL, 1)
TEST_11(image, VKY_CONTROLLER_PANZOOM, NULL, 1)
TEST_11(polygon, VKY_CONTROLLER_PANZOOM, NULL, 1)
TEST_11(pslg_1, VKY_CONTROLLER_PANZOOM, NULL, 1)
TEST_11(pslg_2, VKY_CONTROLLER_PANZOOM, NULL, 1)
TEST_11(france, VKY_CONTROLLER_PANZOOM, NULL, 1)


/**** plot3d *********************************************************************************/

TEST_11(surface, VKY_CONTROLLER_ARCBALL, NULL, 1)
TEST_11(spheres, VKY_CONTROLLER_AUTOROTATE, NULL, FPS)
TEST_11(brain, VKY_CONTROLLER_ARCBALL, NULL, 1)
TEST_11(volume, VKY_CONTROLLER_VOLUME, NULL, 1)


/**** custom *********************************************************************************/

TEST_11(mandelbrot, VKY_CONTROLLER_PANZOOM, NULL, 1)
TEST_11(raytracing, VKY_CONTROLLER_FPS, NULL, 1)


/**** compute ********************************************************************************/

TEST_11(compute_image, VKY_CONTROLLER_PANZOOM, NULL, FPS)



/*********************************************************************************************/
/*  Tests 12                                                                                 */
/*********************************************************************************************/

TEST_12(blank, hello, VKY_CONTROLLER_NONE, NULL, VKY_CONTROLLER_NONE, NULL, 1)
TEST_12(triangle, triangle, VKY_CONTROLLER_NONE, NULL, VKY_CONTROLLER_NONE, NULL, 1)
TEST_12(scatter, arrows, VKY_CONTROLLER_AXES_2D, NULL, VKY_CONTROLLER_AXES_2D, NULL, 1)
