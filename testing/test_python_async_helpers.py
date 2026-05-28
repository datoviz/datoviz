#!/usr/bin/env python3
"""Tests for the thin Python async/event helpers."""

from __future__ import annotations

import asyncio
import ctypes
import json
from pathlib import Path

import pytest


ROOT_DIR = Path(__file__).resolve().parents[1]
ABI_PATH = ROOT_DIR / 'build' / 'bindings' / 'ctypes_abi.json'


def _library_exists() -> bool:
    names = ('libdatoviz.so', 'libdatoviz.dylib', 'libdatoviz.dll')
    return any((ROOT_DIR / 'build' / 'src' / name).exists() for name in names) or any(
        (ROOT_DIR / 'build' / name).exists() for name in names
    )


def _add_one(value: int) -> int:
    return value + 1


def _assert_record_layout(cls, record: dict) -> None:
    assert ctypes.sizeof(cls) == record['size']
    assert ctypes.alignment(cls) == record['align']
    for field_name, offset in record.get('fields', {}).items():
        assert hasattr(cls, field_name)
        assert getattr(cls, field_name).offset == offset


@pytest.mark.skipif(not ABI_PATH.exists(), reason='ctypes ABI facts have not been generated')
def test_pointer_event_wrapper_layout_matches_c_abi():
    import datoviz.raw as dvz

    with ABI_PATH.open() as f:
        records = json.load(f).get('records', {})

    layout_classes = {
        'DvzInputResizeEvent': dvz.DvzInputResizeEvent,
        'DvzInputScaleEvent': dvz.DvzInputScaleEvent,
        'DvzKeyboardEvent': dvz.DvzKeyboardEvent,
        'DvzPointerWheelEvent': dvz.DvzPointerWheelEvent,
        'DvzPointerDragEvent': dvz.DvzPointerDragEvent,
        'DvzPointerEventUnion': dvz.DvzPointerEventUnion,
        'DvzPointerEvent': dvz.DvzPointerEvent,
    }
    for name, cls in layout_classes.items():
        assert name in records
        _assert_record_layout(cls, records[name])


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_async_app_loop_schedules_render_once(monkeypatch):
    import datoviz.host as dvz_host

    calls = []

    def render_once(app):
        calls.append(app)
        return 0

    async def run_case():
        app = object()
        monkeypatch.setattr(dvz_host.raw, 'dvz_app_render_once', render_once)
        host = dvz_host.Host(app)
        task = asyncio.create_task(host.run_async())
        await asyncio.sleep(0)
        host.request_frame()
        await asyncio.sleep(0)
        host.stop()
        await task
        assert calls

    asyncio.run(run_case())


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_async_worker_helpers():
    from datoviz.host import Host

    async def run_case():
        host = Host(None)
        try:
            assert await host.run_thread(_add_one, 2) == 3
            assert await host.run_process(_add_one, 3) == 4
        finally:
            host.shutdown_executors()

    asyncio.run(run_case())


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_async_pointer_event_dispatch(monkeypatch):
    import datoviz.host as dvz_host
    import datoviz.raw as dvz

    async def run_case():
        router = dvz.dvz_input_router()
        assert bool(router)
        monkeypatch.setattr(dvz_host.raw, 'dvz_view_input', lambda _view: router)
        host = dvz_host.Host(None)
        view = host.view(object())
        seen = []

        @view.pointer("move")
        async def on_move(ev):
            value = await host.run_thread(_add_one, int(ev.x))
            seen.append((round(ev.x, 2), round(ev.y, 2), value, ev.timestamp_ns))

        try:
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
                42,
                None,
            )
            for _ in range(20):
                if seen:
                    break
                await asyncio.sleep(0.01)
            assert seen == [(7.0, 8.0, 8, 42)]
        finally:
            host.close()
            dvz.dvz_input_router_destroy(router)

    asyncio.run(run_case())


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_host_input_family_dispatch(monkeypatch):
    import datoviz.host as dvz_host
    import datoviz.raw as dvz

    async def run_case():
        router = dvz.dvz_input_router()
        assert bool(router)
        monkeypatch.setattr(dvz_host.raw, 'dvz_view_input', lambda _view: router)
        host = dvz_host.Host(None)
        view = host.view(object())
        seen = []

        @view.keyboard("press")
        def on_key(ev):
            seen.append(("key", ev.key, ev.mods))

        @view.resize()
        def on_resize(ev):
            seen.append(("resize", ev.framebuffer_width, ev.window_width))

        @view.scale()
        async def on_scale(ev):
            seen.append(("scale", ev.content_scale_x, ev.content_scale_y))

        try:
            dvz.dvz_keyboard_emit(
                router,
                dvz.DvzKeyboardEventType.DVZ_KEYBOARD_EVENT_PRESS,
                dvz.DvzKeyCode.DVZ_KEY_A,
                3,
                None,
            )
            resize = dvz.DvzInputResizeEvent(800, 600, 400, 300, 2.0, 2.0)
            dvz.dvz_input_emit_resize(router, ctypes.byref(resize))
            scale = dvz.DvzInputScaleEvent(1.5, 1.25)
            dvz.dvz_input_emit_scale(router, ctypes.byref(scale))
            for _ in range(20):
                if len(seen) == 3:
                    break
                await asyncio.sleep(0.01)
            assert seen == [
                ("key", dvz.DvzKeyCode.DVZ_KEY_A, 3),
                ("resize", 800, 400),
                ("scale", 1.5, 1.25),
            ]
        finally:
            host.close()
            dvz.dvz_input_router_destroy(router)

    asyncio.run(run_case())


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_async_pointer_handler_without_loop_is_skipped():
    import datoviz.host as dvz_host
    import datoviz.raw as dvz

    router = dvz.dvz_input_router()
    assert bool(router)
    monkeypatch = pytest.MonkeyPatch()
    monkeypatch.setattr(dvz_host.raw, 'dvz_view_input', lambda _view: router)
    host = dvz_host.Host(None)
    view = host.view(object())
    seen = []

    @view.pointer("move")
    async def on_move(ev):
        seen.append((ev.x, ev.y))

    try:
        with pytest.warns(RuntimeWarning, match='Host.run'):
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
                42,
                None,
            )
        assert seen == []
    finally:
        host.close()
        monkeypatch.undo()
        dvz.dvz_input_router_destroy(router)
