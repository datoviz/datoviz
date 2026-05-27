"""Small asyncio helpers for Datoviz raw ctypes integrations."""

from __future__ import annotations

import asyncio
import concurrent.futures
import functools
from collections.abc import Callable
from typing import Any

from . import raw


_PROCESS_EXECUTOR: concurrent.futures.ProcessPoolExecutor | None = None


async def run_thread(
    func: Callable[..., Any],
    *args: Any,
    executor: concurrent.futures.Executor | None = None,
    **kwargs: Any,
) -> Any:
    """Run a callable in a thread executor and await its result."""

    loop = asyncio.get_running_loop()
    call = functools.partial(func, *args, **kwargs)
    return await loop.run_in_executor(executor, call)


async def run_process(
    func: Callable[..., Any],
    *args: Any,
    executor: concurrent.futures.ProcessPoolExecutor | None = None,
    **kwargs: Any,
) -> Any:
    """Run a callable in a process executor and await its result."""

    global _PROCESS_EXECUTOR
    loop = asyncio.get_running_loop()
    if executor is None:
        if _PROCESS_EXECUTOR is None:
            _PROCESS_EXECUTOR = concurrent.futures.ProcessPoolExecutor()
        executor = _PROCESS_EXECUTOR
    call = functools.partial(func, *args, **kwargs)
    return await loop.run_in_executor(executor, call)


def shutdown_executors() -> None:
    """Shut down module-owned executors."""

    global _PROCESS_EXECUTOR
    if _PROCESS_EXECUTOR is not None:
        _PROCESS_EXECUTOR.shutdown()
        _PROCESS_EXECUTOR = None


class AppLoop:
    """Asyncio-driven render loop for hosted Python integrations."""

    def __init__(self, app, *, views: list[Any] | tuple[Any, ...] = (), fps: float | None = None):
        self.app = app
        self.views = list(views)
        self.fps = fps
        self._event: asyncio.Event | None = None
        self._loop: asyncio.AbstractEventLoop | None = None
        self._running = False
        self._callbacks: list[Any] = []

    def add_view(self, view) -> None:
        """Register a view whose request-frame callback should wake this loop."""

        self.views.append(view)
        if self._loop is not None:
            self._install_view_callback(view)

    def request_frame(self) -> None:
        """Schedule one render tick."""

        if self._loop is not None and self._event is not None:
            self._loop.call_soon_threadsafe(self._event.set)

    def stop(self) -> None:
        """Stop the loop after the next wakeup."""

        self._running = False
        self.request_frame()

    def _install_view_callback(self, view) -> None:
        def callback(_view, _user_data):
            self.request_frame()

        self._callbacks.append(callback)
        raw.dvz_view_set_request_frame_callback(view, callback, None)

    async def run_async(self) -> None:
        """Drive dvz_app_render_once() from the current asyncio loop until cancelled."""

        self._loop = asyncio.get_running_loop()
        self._event = asyncio.Event()
        self._running = True
        for view in self.views:
            self._install_view_callback(view)
        self._event.set()

        try:
            while self._running:
                if self.fps is not None and self.fps > 0:
                    try:
                        await asyncio.wait_for(self._event.wait(), timeout=1.0 / self.fps)
                    except asyncio.TimeoutError:
                        pass
                else:
                    await self._event.wait()
                self._event.clear()
                raw.dvz_app_render_once(self.app)
        finally:
            for view in self.views:
                raw.dvz_view_set_request_frame_callback(view, None, None)
            self._callbacks.clear()
            self._running = False


async def run_async(
    app,
    *,
    views: list[Any] | tuple[Any, ...] = (),
    fps: float | None = None,
) -> None:
    """Drive a Datoviz app from asyncio."""

    await AppLoop(app, views=views, fps=fps).run_async()


def run(app, *, views: list[Any] | tuple[Any, ...] = (), fps: float | None = None):
    """Run or schedule an asyncio-driven Datoviz app loop."""

    coro = run_async(app, views=views, fps=fps)
    try:
        loop = asyncio.get_running_loop()
    except RuntimeError:
        return asyncio.run(coro)
    return loop.create_task(coro)
