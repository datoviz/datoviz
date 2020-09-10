from .wrap import viskylib as vl, make_vertices, upload_data
from . import _constants as const


def demo_blank():
    vl.vky_demo_blank()


def markers(pos, color):
    vl.log_set_level_env()

    app = vl.vky_create_app(const.BACKEND_GLFW, None)
    canvas = vl.vky_create_canvas(app, 100, 100)
    scene = vl.vky_create_scene(canvas, const.WHITE, 1, 1)
    panel = vl.vky_get_panel(scene, 0, 0)
    vl.vky_set_controller(panel, const.CONTROLLER_AXES_2D, None)

    # Raw markers.
    visual = vl.vky_visual(scene, const.VISUAL_MARKER_RAW, None)
    vl.vky_add_visual_to_panel(
        visual, panel, const.VIEWPORT_INNER, const.VISUAL_PRIORITY_NONE)

    upload_data(visual, make_vertices(pos, color))

    vl.vky_run_app(app)
    vl.vky_destroy_scene(scene)
    vl.vky_destroy_app(app)
