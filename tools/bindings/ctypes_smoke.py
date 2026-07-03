#!/usr/bin/env python3
"""Smoke-test the generated raw ctypes binding."""

from __future__ import annotations

import ctypes
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
        assert dvz.dvz_panel_query(panel, 10.0, 10.0, ctypes.byref(request)) == 0

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
            assert dvz.dvz_panel_query(panel, 10.0, 10.0, ctypes.byref(request)) == 0

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
    finally:
        dvz.dvz_camera_destroy(camera)


def _check_input_event_layout(dvz) -> None:
    assert [name for name, _ in dvz.DvzInputEvent._fields_] == ['type', 'content']
    assert [name for name, _ in dvz.DvzInputEventContent._fields_] == [
        'pointer',
        'keyboard',
        'resize',
        'scale',
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

    assert dvz.DvzResult is ctypes.c_int32

    for symbol in _smoke_symbols():
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

    t0 = dvz.dvz_time_monotonic_ns()
    t1 = dvz.dvz_time_monotonic_ns()
    assert isinstance(t0, int)
    assert t1 >= t0

    _check_query_result_layout(dvz)
    _check_camera_layout_and_bounds(dvz)
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

    print('raw ctypes smoke: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
