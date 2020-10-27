// each visual has a test for which the screenshot will be used in the doc

static int visual_marker(VkyCanvas* canvas)
{
    vky_create_scene(canvas, (VkyColor){{255, 0, 0}, 255}, 1, 1);
    return 0;
}
