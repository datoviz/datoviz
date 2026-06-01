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


def main() -> int:
    sys.path.insert(0, str(ROOT_DIR))

    import datoviz.raw as dvz  # noqa: PLC0415

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
    assert dvz.dvz_view_external_surface_ffi.argtypes == expected_create_args
    assert dvz.dvz_view_external_surface_ffi.restype == ctypes.POINTER(dvz.DvzView)

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
    assert dvz.dvz_view_update_external_surface_ffi.argtypes == expected_update_args
    assert dvz.dvz_view_update_external_surface_ffi.restype == ctypes.c_int

    t0 = dvz.dvz_time_monotonic_ns()
    t1 = dvz.dvz_time_monotonic_ns()
    assert isinstance(t0, int)
    assert t1 >= t0

    scene = dvz.dvz_scene()
    assert bool(scene)
    dvz.dvz_scene_destroy(scene)

    calls: list[int | None] = []

    def on_pointer(_router, _event, user_data):
        calls.append(user_data)

    router = dvz.dvz_input_router()
    assert bool(router)
    user_data = ctypes.c_void_p(1234)
    dvz.dvz_input_subscribe_pointer(router, on_pointer, user_data)
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
    dvz.dvz_input_unsubscribe_pointer(router, on_pointer, user_data)
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
    dvz.dvz_input_subscribe_pointer(router, recorder.on_pointer, user_data)
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
    dvz.dvz_input_unsubscribe_pointer(router, recorder.on_pointer, user_data)
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
