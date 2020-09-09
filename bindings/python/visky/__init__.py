from .wrap import viskylib, make_vertices, upload_data
from . import _constants as const


def demo_blank():
    viskylib.vky_demo_blank()


def markers(pos, color):
    app = viskylib.vky_create_app(const.BACKEND_GLFW, None)
    canvas = viskylib.vky_create_canvas(app, 100, 100)
    scene = viskylib.vky_create_scene(canvas, const.WHITE, 1, 1)
    panel = viskylib.vky_get_panel(scene, 0, 0)
    viskylib.vky_set_controller(panel, const.CONTROLLER_AXES_2D, None)

    # Raw markers.
    visual = viskylib.vky_visual(scene, const.VISUAL_MARKER_RAW, None)
    viskylib.vky_add_visual_to_panel(
        visual, panel, const.VIEWPORT_INNER, const.VISUAL_PRIORITY_NONE)

    upload_data(visual, make_vertices(pos, color))

    viskylib.vky_run_app(app)
    viskylib.vky_destroy_scene(scene)
    viskylib.vky_destroy_app(app)
