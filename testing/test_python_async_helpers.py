#!/usr/bin/env python3
"""Tests for the thin Python async/event helpers."""

from __future__ import annotations

import asyncio
from pathlib import Path

import pytest


ROOT_DIR = Path(__file__).resolve().parents[1]


def _library_exists() -> bool:
    names = ('libdatoviz.so', 'libdatoviz.dylib', 'libdatoviz.dll')
    return any((ROOT_DIR / 'build' / 'src' / name).exists() for name in names) or any(
        (ROOT_DIR / 'build' / name).exists() for name in names
    )


def _add_one(value: int) -> int:
    return value + 1


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_async_app_loop_schedules_render_once(monkeypatch):
    import datoviz as dvz
    import datoviz.loop as dvz_loop

    calls = []

    def render_once(app):
        calls.append(app)
        return 0

    async def run_case():
        app = object()
        monkeypatch.setattr(dvz_loop.raw, 'dvz_app_render_once', render_once)
        loop = dvz.AppLoop(app)
        task = asyncio.create_task(loop.run_async())
        await asyncio.sleep(0)
        loop.request_frame()
        await asyncio.sleep(0)
        loop.stop()
        await task
        assert calls

    asyncio.run(run_case())


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_async_worker_helpers():
    import datoviz as dvz

    async def run_case():
        assert await dvz.run_thread(_add_one, 2) == 3
        assert await dvz.run_process(_add_one, 3) == 4

    try:
        asyncio.run(run_case())
    finally:
        dvz.shutdown_executors()


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_async_pointer_event_dispatch():
    import datoviz as dvz

    async def run_case():
        router = dvz.dvz_input_router()
        assert bool(router)
        source = dvz.EventSource(router)
        source.bind_loop()
        seen = []

        @source.on("move")
        async def on_move(ev):
            value = await dvz.run_thread(_add_one, int(ev.x))
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
            source.close()
            dvz.dvz_input_router_destroy(router)

    asyncio.run(run_case())
