"""Shared helpers for Python gallery examples."""

from __future__ import annotations

import ctypes
import time

import datoviz as dvz
import numpy as np


WIDTH = 1280
HEIGHT = 720

BG = dvz.DvzColor(13, 18, 25, 255)
TEXT = dvz.DvzColor(217, 226, 236, 255)
CYAN = dvz.DvzColor(34, 211, 238, 255)
BLUE = dvz.DvzColor(96, 165, 250, 255)
GREEN = dvz.DvzColor(74, 222, 128, 255)
YELLOW = dvz.DvzColor(250, 204, 21, 255)
RED = dvz.DvzColor(248, 113, 113, 255)
WHITE = dvz.DvzColor(255, 255, 255, 255)


def color_array(*colors):
    return np.array([[c.r, c.g, c.b, c.a] for c in colors], dtype=np.uint8)


def continuous_scale(scene, name: bytes = b"python_gallery_scale"):
    colors = (dvz.DvzColor * 4)(BG, CYAN, GREEN, YELLOW)
    colormap = dvz.dvz_colormap_custom(scene, name, colors, 4)
    if not colormap:
        raise RuntimeError("dvz_colormap_custom() failed")

    desc = dvz.dvz_scale_desc()
    desc.kind = dvz.DVZ_SCALE_CONTINUOUS
    scale = dvz.dvz_scale(scene, ctypes.byref(desc))
    if not scale:
        raise RuntimeError("dvz_scale() failed")
    dvz.dvz_scale_set_domain(scale, 0.0, 1.0)
    dvz.dvz_scale_set_colormap(scale, colormap)
    return scale


def categorical_scale(scene, categories, label: bytes = b"categories"):
    desc = dvz.dvz_scale_desc()
    desc.kind = dvz.DVZ_SCALE_CATEGORICAL
    desc.label = label
    scale = dvz.dvz_scale(scene, ctypes.byref(desc))
    if not scale:
        raise RuntimeError("dvz_scale() failed")

    c_categories = (dvz.DvzScaleCategory * len(categories))()
    for i, (category_id, name, color) in enumerate(categories):
        c_categories[i].category_id = int(category_id)
        c_categories[i].order = i
        c_categories[i].label = name.encode() if isinstance(name, str) else name
        c_categories[i].color = color
    if dvz.dvz_scale_set_categories(scale, c_categories, len(c_categories)) != 0:
        raise RuntimeError("dvz_scale_set_categories() failed")
    return scale


def image_quad():
    positions = np.array(
        [
            [-0.88, -0.66, 0.0],
            [-0.88, +0.66, 0.0],
            [+0.88, -0.66, 0.0],
            [+0.88, +0.66, 0.0],
        ],
        dtype=np.float32,
    )
    texcoords = np.array(
        [[0.0, 0.0], [0.0, 1.0], [1.0, 0.0], [1.0, 1.0]],
        dtype=np.float32,
    )
    return positions, texcoords


def add_image(scene, panel, field, scale=None, alpha=False):
    image = dvz.dvz_image(scene, 0)
    if not image:
        raise RuntimeError("dvz_image() failed")

    positions, texcoords = image_quad()
    dvz.dvz_visual_set_data(image, "position", positions)
    dvz.dvz_visual_set_data(image, "texcoords", texcoords)
    if scale is not None:
        dvz.dvz_visual_set_scale(image, b"color", scale)
    if dvz.dvz_visual_set_field(image, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field() failed")
    dvz.dvz_visual_set_depth_test(image, False)
    if alpha:
        dvz.dvz_visual_set_alpha_mode(image, dvz.DVZ_ALPHA_BLENDED)
    add_visual(panel, image)
    return image


def scene_panel(width: int = WIDTH, height: int = HEIGHT):
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, width, height, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        raise RuntimeError("dvz_panel_full() failed")
    dvz.dvz_panel_set_background_color(panel, BG)
    return scene, figure, panel


def panel_rect(figure, x: float, y: float, width: float, height: float):
    desc = dvz.dvz_panel_desc()
    desc.x = x
    desc.y = y
    desc.width = width
    desc.height = height
    panel = dvz.dvz_panel(figure, ctypes.byref(desc))
    if not panel:
        raise RuntimeError("dvz_panel() failed")
    dvz.dvz_panel_set_background_color(panel, BG)
    return panel


def run(scene, figure, title: str):
    dvz.run(scene, figure, WIDTH, HEIGHT, title)


def run_with_view(scene, figure, title: str, configure_view):
    app = dvz.dvz_app(scene)
    if not app:
        raise RuntimeError("dvz_app() failed")
    try:
        view = dvz.dvz_view_window(app, figure, WIDTH, HEIGHT, title.encode())
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        configure_view(view)
        dvz.dvz_app_run(app, 0)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def run_with_frame_callback(scene, figure, title: str, on_frame, configure_view=None):
    app = dvz.dvz_app(scene)
    if not app:
        raise RuntimeError("dvz_app() failed")
    callback_set = False
    try:
        view = dvz.dvz_view_window(app, figure, WIDTH, HEIGHT, title.encode())
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        if configure_view is not None:
            configure_view(view)

        state = {"index": 0, "start": time.monotonic()}

        def frame_callback(view_arg, _user_data):
            frame_index = state["index"]
            elapsed = time.monotonic() - state["start"]
            on_frame(view_arg, frame_index, elapsed)
            state["index"] = frame_index + 1
            dvz.dvz_view_request_frame(view_arg)

        if dvz.dvz_view_set_frame_callback(view, frame_callback, None) != 0:
            raise RuntimeError("dvz_view_set_frame_callback() failed")
        callback_set = True
        dvz.dvz_app_run(app, 0)
    finally:
        if callback_set:
            dvz.dvz_view_set_frame_callback(view, None, None)
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def run_with_input_callbacks(
    scene,
    figure,
    title: str,
    *,
    on_pointer=None,
    on_frame=None,
    configure_view=None,
):
    app = dvz.dvz_app(scene)
    if not app:
        raise RuntimeError("dvz_app() failed")
    view = None
    router = None
    input_subscription = 0
    frame_callback_set = False
    try:
        view = dvz.dvz_view_window(app, figure, WIDTH, HEIGHT, title.encode())
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        if configure_view is not None:
            configure_view(view)

        if on_pointer is not None:
            router = dvz.dvz_view_input(view)
            if not router:
                raise RuntimeError("dvz_view_input() failed")

            def input_callback(_router, event_ptr, _user_data):
                event = event_ptr.contents
                if event.type != dvz.DVZ_INPUT_EVENT_POINTER:
                    return
                on_pointer(event.content.pointer)
                dvz.dvz_view_request_frame(view)

            input_subscription = dvz.dvz_input_subscribe_event(router, input_callback, None)
            if input_subscription == 0:
                raise RuntimeError("dvz_input_subscribe_event() failed")

        if on_frame is not None:
            state = {"index": 0, "start": time.monotonic()}

            def frame_callback(view_arg, _user_data):
                frame_index = state["index"]
                elapsed = time.monotonic() - state["start"]
                on_frame(view_arg, frame_index, elapsed)
                state["index"] = frame_index + 1
                dvz.dvz_view_request_frame(view_arg)

            if dvz.dvz_view_set_frame_callback(view, frame_callback, None) != 0:
                raise RuntimeError("dvz_view_set_frame_callback() failed")
            frame_callback_set = True

        dvz.dvz_app_run(app, 0)
    finally:
        if input_subscription and router:
            dvz.dvz_input_unsubscribe(router, input_subscription)
        if frame_callback_set and view:
            dvz.dvz_view_set_frame_callback(view, None, None)
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def figure_to_panel_px(panel, x: float, y: float):
    src = (ctypes.c_double * 2)(float(x), float(y))
    dst = (ctypes.c_double * 2)()
    ok = dvz.dvz_panel_transform_point(
        panel,
        dvz.DVZ_PANEL_COORD_FIGURE_PX,
        dvz.DVZ_PANEL_COORD_PANEL_PX,
        src,
        dst,
    )
    if not ok:
        return None
    return float(dst[0]), float(dst[1])


def panel_px_to_data(panel, x: float, y: float):
    src = (ctypes.c_double * 2)(float(x), float(y))
    dst = (ctypes.c_double * 2)()
    ok = dvz.dvz_panel_position_to_data(panel, dvz.DVZ_PANEL_COORD_PANEL_PX, src, dst)
    if not ok:
        return None
    return float(dst[0]), float(dst[1])


def queue_panel_query(panel, x: float, y: float, request_id: int, target, *, hit_policy=None):
    request = dvz.dvz_query_request()
    request.request_id = request_id
    request.target = target
    if hit_policy is not None:
        request.hit_policy = hit_policy
    return dvz.dvz_panel_query_px(panel, float(x), float(y), ctypes.byref(request))


def poll_queries(scene):
    while True:
        result = dvz.DvzQueryResult()
        if not dvz.dvz_scene_poll_query(scene, ctypes.byref(result)):
            break
        yield result


def create_item_selection(scene, tint=YELLOW):
    desc = dvz.dvz_selection_desc()
    desc.mode = dvz.DVZ_SELECT_TOGGLE
    desc.target = dvz.DVZ_SCENE_TARGET_ITEM
    selection = dvz.dvz_selection(scene, ctypes.byref(desc))
    if not selection:
        raise RuntimeError("dvz_selection() failed")

    style = dvz.dvz_selection_visual_style()
    style.selected_visual_flags = dvz.DVZ_ITEM_STATE_VISUAL_TINT
    style.selected_tint = tint
    style.selected_tint_mix = 1.0
    style.unselected_visual_flags = dvz.DVZ_ITEM_STATE_VISUAL_NONE
    if dvz.dvz_selection_set_visual_style(selection, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_selection_set_visual_style() failed")
    return selection


def create_item_hover(scene, scale: float):
    desc = dvz.dvz_hover_desc()
    desc.target = dvz.DVZ_SCENE_TARGET_ITEM
    desc.hit_policy = dvz.DVZ_QUERY_HIT_FRONTMOST
    hover = dvz.dvz_hover(scene, ctypes.byref(desc))
    if not hover:
        raise RuntimeError("dvz_hover() failed")

    style = dvz.dvz_item_state_visual_style()
    style.visual_flags = dvz.DVZ_ITEM_STATE_VISUAL_SCALE
    style.scale = float(scale)
    if dvz.dvz_hover_set_visual_style(hover, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_hover_set_visual_style() failed")
    return hover


def query_item_hit(query, visual_family, item_count: int) -> bool:
    return (
        query.status == dvz.DVZ_QUERY_STATUS_HIT
        and bool(query.hit)
        and query.visual_family == visual_family
        and query.resolved_target == dvz.DVZ_SCENE_TARGET_ITEM
        and query.resolved_id < item_count
    )


def bind_panzoom(view, scene, panel, dims):
    controller = dvz.dvz_panzoom(scene, None)
    if not controller:
        raise RuntimeError("dvz_panzoom() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dims) != 0:
        raise RuntimeError("dvz_view_bind_controller() failed")
    panzoom = dvz.dvz_controller_panzoom(controller)
    if not panzoom:
        raise RuntimeError("dvz_controller_panzoom() failed")
    return controller, panzoom


def manual_camera(panel):
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (1.55, -2.00, 1.45)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 0.0, 1.0)
    camera.projection.fov_y = 0.66
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")
    return camera


def add_cube_mesh(scene, panel, size: float = 1.25):
    h = 0.5 * size
    positions = np.array(
        [
            [-h, -h, -h],
            [+h, -h, -h],
            [+h, +h, -h],
            [-h, +h, -h],
            [-h, -h, +h],
            [+h, -h, +h],
            [+h, +h, +h],
            [-h, +h, +h],
        ],
        dtype=np.float32,
    )
    colors = color_array(CYAN, GREEN, YELLOW, RED, BLUE, CYAN, GREEN, WHITE)
    indices = np.array(
        [
            0, 1, 2, 2, 3, 0,
            4, 6, 5, 6, 4, 7,
            0, 4, 5, 5, 1, 0,
            1, 5, 6, 6, 2, 1,
            2, 6, 7, 7, 3, 2,
            3, 7, 4, 4, 0, 3,
        ],
        dtype=np.uint32,
    )

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        raise RuntimeError("dvz_mesh() failed")
    if dvz.dvz_visual_set_data_many(mesh, {"position": positions, "color": colors}) != 0:
        raise RuntimeError("dvz_visual_set_data_many() failed")
    if dvz.dvz_visual_set_index_data(mesh, indices) != 0:
        raise RuntimeError("dvz_visual_set_index_data() failed")
    add_visual(panel, mesh)
    return mesh


def set_filled_point_style(visual):
    style = dvz.dvz_point_style_desc()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_FILLED
    style.stroke_width_px = 0.0
    if dvz.dvz_point_set_style(visual, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_point_set_style() failed")


def set_outline_marker_style(visual, edge_color=TEXT):
    style = dvz.dvz_marker_style()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_OUTLINE
    style.edge_color = edge_color
    style.stroke_width_px = 2.25
    if dvz.dvz_marker_set_style(visual, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_marker_set_style() failed")


def add_visual(panel, visual):
    if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
        raise RuntimeError("dvz_panel_add_visual() failed")


def add_points(scene, panel, positions, colors, diameters):
    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    )
    set_filled_point_style(point)
    dvz.dvz_visual_set_depth_test(point, False)
    add_visual(panel, point)
    return point
