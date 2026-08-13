#!/usr/bin/env python3
"""Smoke-test the generated raw ctypes binding."""

from __future__ import annotations

import ctypes
import gc
import math
import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
POLICY_PATH = ROOT_DIR / 'spec' / 'bindings' / 'ctypes.yml'


def _smoke_symbols() -> list[str]:
    symbols: list[str] = []
    in_smoke_list = False
    for line in POLICY_PATH.read_text().splitlines():
        stripped = line.strip()
        if stripped == 'smoke_symbols:':
            in_smoke_list = True
            continue
        if in_smoke_list and stripped.startswith('- '):
            symbols.append(stripped[2:].strip())
        elif in_smoke_list and stripped and not stripped.startswith('#'):
            break
    return symbols


def _run_gsp_query_smoke(dvz) -> None:
    caps = dvz.dvz_capability_snapshot()
    assert caps.struct_size == ctypes.sizeof(dvz.DvzCapabilitySnapshot)
    assert not dvz.dvz_view_capabilities(None, ctypes.byref(caps))

    scene = dvz.dvz_scene()
    assert bool(scene)
    try:
        figure = dvz.dvz_figure(scene, 64, 64, 0)
        assert bool(figure)
        panel = dvz.dvz_panel_full(figure)
        assert bool(panel)

        scene_id = dvz.dvz_scene_id(scene)
        figure_id = dvz.dvz_figure_id(figure)
        panel_id = dvz.dvz_panel_id(panel)
        assert scene_id
        assert figure_id
        assert panel_id

        request = dvz.dvz_query_request()
        assert request.struct_size == ctypes.sizeof(dvz.DvzQueryRequest)
        request.request_id = 123
        request.target = dvz.DvzSceneTargetKind.DVZ_SCENE_TARGET_ITEM
        assert dvz.dvz_panel_query_px(panel, 10.0, 10.0, ctypes.byref(request)) == 0

        processed = dvz.dvz_figure_process_queries(figure, None, ctypes.byref(caps))
        assert processed == 1

        result = dvz.DvzQueryResult()
        assert dvz.dvz_scene_poll_query(scene, ctypes.byref(result))
        assert result.request_id == request.request_id
        assert result.status == dvz.DvzQueryStatus.DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL
        assert result.scene_id == scene_id
        assert result.figure_id == figure_id
        assert result.panel_id == panel_id
        assert not dvz.dvz_scene_poll_query(scene, ctypes.byref(result))

        for request_id, target in [
            (124, dvz.DvzSceneTargetKind.DVZ_SCENE_TARGET_GUIDE),
            (125, dvz.DvzSceneTargetKind.DVZ_SCENE_TARGET_ALL_RENDERED),
        ]:
            request = dvz.dvz_query_request()
            request.request_id = request_id
            request.target = target
            assert dvz.dvz_panel_query_px(panel, 10.0, 10.0, ctypes.byref(request)) == 0

        processed = dvz.dvz_figure_process_queries(figure, None, ctypes.byref(caps))
        assert processed == 2

        guide_result = dvz.DvzQueryResult()
        all_rendered_result = dvz.DvzQueryResult()
        assert dvz.dvz_scene_poll_query(scene, ctypes.byref(guide_result))
        assert dvz.dvz_scene_poll_query(scene, ctypes.byref(all_rendered_result))
        assert guide_result.request_id == 124
        assert guide_result.status == dvz.DvzQueryStatus.DVZ_QUERY_STATUS_UNSUPPORTED_TARGET
        assert guide_result.raw_target == dvz.DvzSceneTargetKind.DVZ_SCENE_TARGET_GUIDE
        assert guide_result.resolved_target == dvz.DvzSceneTargetKind.DVZ_SCENE_TARGET_GUIDE
        assert all_rendered_result.request_id == 125
        assert all_rendered_result.status == dvz.DvzQueryStatus.DVZ_QUERY_STATUS_UNSUPPORTED_TARGET
        assert (
            all_rendered_result.raw_target
            == dvz.DvzSceneTargetKind.DVZ_SCENE_TARGET_ALL_RENDERED
        )
        assert (
            all_rendered_result.resolved_target
            == dvz.DvzSceneTargetKind.DVZ_SCENE_TARGET_ALL_RENDERED
        )
        assert not dvz.dvz_scene_poll_query(scene, ctypes.byref(result))
    finally:
        dvz.dvz_scene_destroy(scene)


def _check_query_result_layout(dvz) -> None:
    assert hasattr(dvz, 'DvzQueryResult')
    assert hasattr(dvz.DvzQueryResult, '_fields_')

    field_names = [name for name, *_ in dvz.DvzQueryResult._fields_]
    for required in [
        'request_id',
        'status',
        'hit',
        'panel_position',
        'framebuffer_position',
        'visual_id',
        'visual_family',
        'raw_target',
        'resolved_target',
        'item_id',
        'texel_id',
        'has_display_rgba',
        'display_rgba',
        'value_kind',
        'scalar',
        'vector',
        'label',
    ]:
        assert required in field_names

    result = dvz.DvzQueryResult()
    result.request_id = 123
    result.status = 2
    result.hit = False
    result.panel_position[0] = 12.0
    result.panel_position[1] = 34.0
    assert result.request_id == 123
    assert tuple(result.panel_position) == (12.0, 34.0)
    assert dvz.dvz_scene_poll_query.argtypes == [
        ctypes.POINTER(dvz.DvzScene),
        ctypes.POINTER(dvz.DvzQueryResult),
    ]


def _check_camera_layout_and_bounds(dvz) -> None:
    assert [name for name, _ in dvz.DvzCameraView._fields_] == ['eye', 'target', 'up']
    assert {'view', 'projection'}.issubset(
        {name for name, _ in dvz.DvzCameraDesc._fields_}
    )
    assert hasattr(dvz, 'dvz_camera_view')
    assert hasattr(dvz, 'dvz_camera_desc')
    assert hasattr(dvz, 'dvz_camera_set_orthographic_bounds')

    view = dvz.dvz_camera_view()
    view.eye[0] = 1.0
    view.eye[1] = 2.0
    view.eye[2] = 3.0
    view.target[2] = -1.0

    desc = dvz.dvz_camera_desc()
    assert desc.struct_size == ctypes.sizeof(dvz.DvzCameraDesc)
    desc.view = view
    desc.projection.type = dvz.DVZ_CAMERA_ORTHOGRAPHIC
    desc.projection.near_clip = 0.1
    desc.projection.far_clip = 100.0
    desc.projection.ortho_height = 2.0

    camera = dvz.dvz_camera_create(ctypes.byref(desc))
    assert bool(camera)
    try:
        out = dvz.DvzCameraView()
        dvz.dvz_camera_get_view(camera, ctypes.byref(out))
        assert tuple(out.eye) == (1.0, 2.0, 3.0)
        assert tuple(out.target) == (0.0, 0.0, -1.0)

        assert (
            dvz.dvz_camera_set_orthographic_bounds(
                camera, 2.0, -2.0, -1.0, 3.0, 0.1, 100.0
            )
            == 0
        )
        left = ctypes.c_float()
        right = ctypes.c_float()
        bottom = ctypes.c_float()
        top = ctypes.c_float()
        near = ctypes.c_float()
        far = ctypes.c_float()
        assert (
            dvz.dvz_camera_get_orthographic_bounds(
                camera,
                ctypes.byref(left),
                ctypes.byref(right),
                ctypes.byref(bottom),
                ctypes.byref(top),
                ctypes.byref(near),
                ctypes.byref(far),
            )
            == 0
        )
        assert (left.value, right.value, bottom.value, top.value) == (2.0, -2.0, -1.0, 3.0)
        assert abs(near.value - 0.1) < 1e-6
        assert abs(far.value - 100.0) < 1e-6

        if hasattr(dvz, 'dvz_camera_mvp'):
            mvp = dvz.DvzMVP()
            dvz.dvz_camera_mvp(camera, ctypes.byref(mvp))
            assert any(value != 0 for row in mvp.view for value in row)
            assert any(value != 0 for row in mvp.proj for value in row)
    finally:
        dvz.dvz_camera_destroy(camera)


def _check_concrete_record_defaults_and_controllers(dvz) -> None:
    from datoviz import _ctypes as generated

    geometry = dvz.dvz_field_geometry()
    assert geometry.struct_size == ctypes.sizeof(dvz.DvzFieldGeometry)
    geometry_out = dvz.DvzFieldGeometry()
    assert dvz.dvz_ffi_field_geometry(ctypes.byref(geometry_out))
    assert geometry_out.struct_size == ctypes.sizeof(dvz.DvzFieldGeometry)

    aligned_symbols = [
        'dvz_arcball_mvp',
        'dvz_panzoom_mvp',
        'dvz_panzoom_resolve',
        'dvz_visual_transform_desc',
        'dvz_ffi_visual_transform_desc',
    ]
    if not generated._ctypes_alignment_is_effective(16):
        assert all(not hasattr(dvz, name) for name in aligned_symbols)
        return
    assert all(hasattr(dvz, name) for name in aligned_symbols)

    transform = dvz.dvz_visual_transform_desc()
    assert transform.struct_size == ctypes.sizeof(dvz.DvzVisualTransformDesc)
    transform_out = dvz.DvzVisualTransformDesc()
    assert dvz.dvz_ffi_visual_transform_desc(ctypes.byref(transform_out))
    assert transform_out.struct_size == ctypes.sizeof(dvz.DvzVisualTransformDesc)

    arcball_desc = dvz.dvz_arcball_desc()
    arcball = dvz.dvz_arcball_create(ctypes.byref(arcball_desc))
    assert bool(arcball)
    try:
        arcball_mvp = dvz.DvzMVP()
        dvz.dvz_arcball_mvp(arcball, ctypes.byref(arcball_mvp))
        assert any(value != 0 for row in arcball_mvp.model for value in row)
    finally:
        dvz.dvz_arcball_destroy(arcball)

    panzoom_desc = dvz.dvz_panzoom_desc()
    panzoom = dvz.dvz_panzoom_create(ctypes.byref(panzoom_desc))
    assert bool(panzoom)
    try:
        panzoom_mvp = dvz.DvzMVP()
        model_before = tuple(float(index + 1) for index in range(16))
        for index, value in enumerate(model_before):
            panzoom_mvp.model[index // 4][index % 4] = value
        panzoom_mvp.time = 17.25
        panzoom_mvp.flags = 0xA5A5A5A5
        dvz.dvz_panzoom_mvp(panzoom, ctypes.byref(panzoom_mvp))
        assert tuple(value for row in panzoom_mvp.model for value in row) == model_before
        assert panzoom_mvp.time == 17.25
        assert panzoom_mvp.flags == 0xA5A5A5A5
        for matrix in [panzoom_mvp.view, panzoom_mvp.proj]:
            values = [value for row in matrix for value in row]
            assert all(math.isfinite(value) for value in values)
            assert any(value != 0 for value in values)

        evaluation = dvz.DvzPanzoomEval()
        evaluation.base_extent[:] = (-1.0, 1.0, -1.0, 1.0)
        evaluation.viewport_width = 640.0
        evaluation.viewport_height = 480.0
        resolved = dvz.DvzPanzoomResolved()
        assert dvz.dvz_panzoom_resolve(
            panzoom, ctypes.byref(evaluation), ctypes.byref(resolved)
        )
        assert resolved.visible_extent[1] > resolved.visible_extent[0]
        assert resolved.visible_extent[3] > resolved.visible_extent[2]
    finally:
        dvz.dvz_panzoom_destroy(panzoom)


def _check_bounds_frame_and_guides(dvz) -> None:
    positions = ((ctypes.c_float * 3) * 3)(
        (-2.0, -1.0, 0.0),
        (0.5, 3.0, 0.0),
        (4.0, 1.0, 0.0),
    )
    colors = ((ctypes.c_uint8 * 4) * 3)(
        (255, 0, 0, 255),
        (0, 255, 0, 255),
        (0, 0, 255, 255),
    )
    diameters = (ctypes.c_float * 3)(4.0, 5.0, 6.0)

    scene = dvz.dvz_scene()
    assert bool(scene)
    snapshot = None
    try:
        figure = dvz.dvz_figure(scene, 320, 240, 0)
        panel = dvz.dvz_panel_full(figure)
        view = dvz.dvz_panel_view2d_desc()
        assert dvz.dvz_panel_set_view2d(panel, ctypes.byref(view)) == 0

        visual = dvz.dvz_point(scene, 0)
        assert bool(visual)
        assert (
            dvz.dvz_visual_set_data(
                visual, b'position', ctypes.cast(positions, ctypes.c_void_p), 3
            )
            == 0
        )
        assert (
            dvz.dvz_visual_set_data(
                visual, b'color', ctypes.cast(colors, ctypes.c_void_p), 3
            )
            == 0
        )
        assert (
            dvz.dvz_visual_set_data(
                visual, b'diameter_px', ctypes.cast(diameters, ctypes.c_void_p), 3
            )
            == 0
        )

        visual_bounds = dvz.DvzBounds()
        assert dvz.dvz_visual_bounds(visual, ctypes.byref(visual_bounds)) == 0
        assert visual_bounds.valid
        assert tuple(visual_bounds.min) == (-2.0, -1.0, 0.0)
        assert tuple(visual_bounds.max) == (4.0, 3.0, 0.0)

        assert dvz.dvz_panel_add_visual(panel, visual, None) == 0
        panel_visual_bounds = dvz.DvzBounds()
        assert (
            dvz.dvz_panel_visual_bounds(
                panel,
                visual,
                dvz.DVZ_BOUNDS_SPACE_VISUAL,
                ctypes.byref(panel_visual_bounds),
            )
            == 0
        )
        assert panel_visual_bounds.valid
        panel_bounds = dvz.DvzBounds()
        assert (
            dvz.dvz_panel_bounds(
                panel, dvz.DVZ_BOUNDS_SPACE_VISUAL, ctypes.byref(panel_bounds)
            )
            == 0
        )
        assert panel_bounds.valid

        line_desc = dvz.dvz_guide_line_desc()
        line_desc.orientation = dvz.DVZ_GUIDE_ORIENTATION_HORIZONTAL
        line_desc.value = 0.25
        line_desc.label = b'ctypes threshold'
        assert bool(dvz.dvz_guide_line(panel, ctypes.byref(line_desc)))

        snapshot = dvz.dvz_panel_resolve_frame(panel)
        assert bool(snapshot)
        snapshot_id = dvz.dvz_panel_frame_id(snapshot)
        assert snapshot_id
        if hasattr(dvz, 'dvz_panel_frame_info'):
            info = dvz.DvzPanelFrameInfo()
            assert dvz.dvz_panel_frame_info(snapshot, ctypes.byref(info))
            assert info.struct_size == ctypes.sizeof(dvz.DvzPanelFrameInfo)
            assert info.snapshot_id == snapshot_id

        guide_count = dvz.dvz_panel_frame_guide_count(snapshot)
        assert guide_count > 0
        layout = dvz.DvzGuideLayout()
        assert dvz.dvz_panel_frame_guide_layout(snapshot, 0, ctypes.byref(layout))
        assert layout.struct_size == ctypes.sizeof(dvz.DvzGuideLayout)
        assert layout.has_box
        hit = dvz.DvzGuideHit()
        assert dvz.dvz_panel_frame_guide_hit(
            snapshot,
            layout.box_px.x + 0.5 * layout.box_px.width,
            layout.box_px.y + 0.5 * layout.box_px.height,
            ctypes.byref(hit),
        )
        assert hit.struct_size == ctypes.sizeof(dvz.DvzGuideHit)
        assert hit.hit
    finally:
        if snapshot:
            dvz.dvz_panel_frame_unref(snapshot)
        dvz.dvz_scene_destroy(scene)


def _check_panel_view_state_layout_and_readback(dvz) -> None:
    from datoviz import _ctypes as generated

    supported = generated._ctypes_alignment_is_effective(16)
    records = [dvz.DvzPanelView2DState, dvz.DvzPanelView3DState]
    functions = ['dvz_panel_view2d_state', 'dvz_panel_view3d_state']
    if not supported:
        for record in records:
            assert not hasattr(record, '_fields_')
            assert ctypes.sizeof(record) == 0
        for function in functions:
            assert not hasattr(dvz, function)
            assert function in generated._UNSUPPORTED_FUNCTIONS
        return

    expected = {
        dvz.DvzPanelView2DState: (
            192,
            [
                'struct_size',
                'view_id',
                'revision',
                'mode',
                'aspect',
                'domain_x',
                'domain_y',
                'view_extent',
                'data_to_view',
            ],
        ),
        dvz.DvzPanelView3DState: (
            304,
            [
                'struct_size',
                'view_id',
                'revision',
                'view',
                'projection',
                'has_explicit_orthographic_bounds',
                'orthographic_bounds',
                'model_matrix',
                'view_matrix',
                'projection_matrix',
            ],
        ),
    }
    for record, (size, fields) in expected.items():
        assert ctypes.sizeof(record) == size
        assert ctypes.alignment(record) == 16
        for field in fields:
            assert hasattr(record, field)

    scene = dvz.dvz_scene()
    assert bool(scene)
    try:
        figure = dvz.dvz_figure(scene, 64, 64, 0)
        assert bool(figure)
        panel = dvz.dvz_panel_full(figure)
        assert bool(panel)

        view2d = dvz.dvz_panel_view2d_desc()
        assert dvz.dvz_panel_set_view2d(panel, ctypes.byref(view2d)) == 0
        state2d = dvz.DvzPanelView2DState()
        assert dvz.dvz_panel_view2d_state(panel, ctypes.byref(state2d))
        assert state2d.struct_size == ctypes.sizeof(state2d)
        assert state2d.view_id
        assert state2d.revision

        view3d = dvz.dvz_panel_view3d_desc()
        assert dvz.dvz_panel_set_view3d_desc(panel, ctypes.byref(view3d)) == 0
        state3d = dvz.DvzPanelView3DState()
        assert dvz.dvz_panel_view3d_state(panel, ctypes.byref(state3d))
        assert state3d.struct_size == ctypes.sizeof(state3d)
        assert state3d.view_id
        assert state3d.revision
        assert state3d.projection.far_clip > state3d.projection.near_clip
        assert any(value != 0 for row in state3d.projection_matrix for value in row)
    finally:
        dvz.dvz_scene_destroy(scene)


def _check_input_event_layout(dvz) -> None:
    assert [name for name, _ in dvz.DvzInputEvent._fields_] == ['type', 'content']
    assert [name for name, _ in dvz.DvzInputEventContent._fields_] == [
        'pointer',
        'keyboard',
        'resize',
        'scale',
        'text',
    ]

    event = dvz.DvzInputEvent()
    event.type = dvz.DVZ_INPUT_EVENT_POINTER
    event.content.pointer.type = dvz.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE
    event.content.pointer.pos[0] = 12.0
    event.content.pointer.pos[1] = 34.0
    assert event.content.pointer.type == dvz.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE
    assert tuple(event.content.pointer.pos) == (12.0, 34.0)


def main() -> int:
    sys.path.insert(0, str(ROOT_DIR))

    import datoviz.raw as dvz  # noqa: PLC0415
    from datoviz import _ctypes as generated  # noqa: PLC0415

    assert dvz.DvzResult is ctypes.c_int32

    for symbol in _smoke_symbols():
        if symbol in generated._UNSUPPORTED_FUNCTIONS:
            continue
        assert hasattr(dvz, symbol), f'missing datoviz.raw.{symbol}'

    expected_create_args = [
        ctypes.POINTER(dvz.DvzApp),
        ctypes.POINTER(dvz.DvzFigure),
        ctypes.c_void_p,
        ctypes.c_uint64,
        ctypes.c_uint32,
        ctypes.c_uint32,
        ctypes.c_float,
        ctypes.c_float,
        ctypes.c_bool,
    ]
    assert dvz.dvz_ffi_view_external_surface.argtypes == expected_create_args
    assert dvz.dvz_ffi_view_external_surface.restype == ctypes.POINTER(dvz.DvzView)

    expected_update_args = [
        ctypes.POINTER(dvz.DvzView),
        ctypes.c_void_p,
        ctypes.c_uint64,
        ctypes.c_uint32,
        ctypes.c_uint32,
        ctypes.c_float,
        ctypes.c_float,
        ctypes.c_bool,
    ]
    assert dvz.dvz_ffi_view_update_external_surface.argtypes == expected_update_args
    assert dvz.dvz_ffi_view_update_external_surface.restype == ctypes.c_int32

    assert ctypes.sizeof(dvz.DvzFramePlanEmitConfig) > 0
    emit_cfg_value = dvz.dvz_frame_plan_emit_config()
    assert emit_cfg_value.struct_size == ctypes.sizeof(dvz.DvzFramePlanEmitConfig)

    t0 = dvz.dvz_time_monotonic_ns()
    t1 = dvz.dvz_time_monotonic_ns()
    assert isinstance(t0, int)
    assert t1 >= t0

    _check_query_result_layout(dvz)
    _check_camera_layout_and_bounds(dvz)
    _check_concrete_record_defaults_and_controllers(dvz)
    _check_bounds_frame_and_guides(dvz)
    _check_panel_view_state_layout_and_readback(dvz)
    _check_input_event_layout(dvz)
    _run_gsp_query_smoke(dvz)

    calls: list[int | None] = []

    def on_pointer(_router, _event, user_data):
        calls.append(user_data)

    router = dvz.dvz_input_router()
    assert bool(router)
    user_data = ctypes.c_void_p(1234)
    pointer_id = dvz.dvz_input_subscribe_pointer(router, on_pointer, user_data)
    assert pointer_id != 0
    dvz.dvz_pointer_emit_position(
        router,
        dvz.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE,
        1.0,
        2.0,
        100.0,
        100.0,
        dvz.DvzPointerButton.DVZ_POINTER_BUTTON_NONE,
        0,
        1.0,
        0,
        None,
    )
    assert calls == [1234]
    assert dvz.dvz_input_unsubscribe(router, pointer_id)
    dvz.dvz_pointer_emit_position(
        router,
        dvz.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE,
        3.0,
        4.0,
        100.0,
        100.0,
        dvz.DvzPointerButton.DVZ_POINTER_BUTTON_NONE,
        0,
        1.0,
        0,
        None,
    )
    assert calls == [1234]
    dvz.dvz_input_router_destroy(router)

    class PointerRecorder:
        def __init__(self):
            self.calls: list[int | None] = []

        def on_pointer(self, _router, _event, user_data):
            self.calls.append(user_data)

    recorder = PointerRecorder()
    router = dvz.dvz_input_router()
    assert bool(router)
    pointer_id = dvz.dvz_input_subscribe_pointer(router, recorder.on_pointer, user_data)
    assert pointer_id != 0
    dvz.dvz_pointer_emit_position(
        router,
        dvz.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE,
        5.0,
        6.0,
        100.0,
        100.0,
        dvz.DvzPointerButton.DVZ_POINTER_BUTTON_NONE,
        0,
        1.0,
        0,
        None,
    )
    assert recorder.calls == [1234]
    assert dvz.dvz_input_unsubscribe(router, pointer_id)
    dvz.dvz_pointer_emit_position(
        router,
        dvz.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE,
        7.0,
        8.0,
        100.0,
        100.0,
        dvz.DvzPointerButton.DVZ_POINTER_BUTTON_NONE,
        0,
        1.0,
        0,
        None,
    )
    assert recorder.calls == [1234]
    dvz.dvz_input_router_destroy(router)

    gc.collect()
    print('raw ctypes smoke: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
