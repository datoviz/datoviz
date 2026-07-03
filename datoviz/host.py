"""Python-hosted integration helpers for Datoviz raw ctypes handles."""

from __future__ import annotations

import asyncio
import concurrent.futures
import ctypes
import functools
import inspect
import warnings
from collections.abc import Callable
from dataclasses import dataclass
from typing import Any, Awaitable

from . import raw


_POINTER_EVENT_NAMES = {
    'release': raw.DvzPointerEventType.DVZ_POINTER_EVENT_RELEASE,
    'press': raw.DvzPointerEventType.DVZ_POINTER_EVENT_PRESS,
    'move': raw.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE,
    'click': raw.DvzPointerEventType.DVZ_POINTER_EVENT_CLICK,
    'double_click': raw.DvzPointerEventType.DVZ_POINTER_EVENT_DOUBLE_CLICK,
    'drag_start': raw.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG_START,
    'drag': raw.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG,
    'drag_stop': raw.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG_STOP,
    'wheel': raw.DvzPointerEventType.DVZ_POINTER_EVENT_WHEEL,
    'all': raw.DvzPointerEventType.DVZ_POINTER_EVENT_ALL,
}


_KEYBOARD_EVENT_NAMES = {
    'press': raw.DvzKeyboardEventType.DVZ_KEYBOARD_EVENT_PRESS,
    'repeat': raw.DvzKeyboardEventType.DVZ_KEYBOARD_EVENT_REPEAT,
    'release': raw.DvzKeyboardEventType.DVZ_KEYBOARD_EVENT_RELEASE,
    'all': None,
}


Handler = Callable[[Any], Awaitable[Any] | Any]


@dataclass(frozen=True)
class PointerEvent:
    """Copied pointer event passed to Python host handlers."""

    type: raw.DvzPointerEventType
    x: float
    y: float
    button: raw.DvzPointerButton
    mods: int
    content_scale: float
    timestamp_ns: int
    user_data: int | None
    wheel_dir: tuple[float, float]
    drag_press_pos: tuple[float, float]
    drag_last_pos: tuple[float, float]
    drag_shift: tuple[float, float]
    is_press_valid: bool


@dataclass(frozen=True)
class KeyboardEvent:
    """Copied keyboard event passed to Python host handlers."""

    type: raw.DvzKeyboardEventType
    key: raw.DvzKeyCode
    mods: int
    user_data: int | None


@dataclass(frozen=True)
class ResizeEvent:
    """Copied resize event passed to Python host handlers."""

    framebuffer_width: int
    framebuffer_height: int
    window_width: int
    window_height: int
    content_scale_x: float
    content_scale_y: float


@dataclass(frozen=True)
class ScaleEvent:
    """Copied content-scale event passed to Python host handlers."""

    content_scale_x: float
    content_scale_y: float


def _ptr_value(obj) -> int | None:
    if obj is None:
        return None
    if isinstance(obj, int):
        return obj
    try:
        return ctypes.cast(obj, ctypes.c_void_p).value
    except (ctypes.ArgumentError, TypeError, ValueError):
        pass
    value = getattr(obj, 'value', None)
    if isinstance(value, int) or value is None:
        return value
    return id(obj)


def _close_awaitable(result: Awaitable[Any]) -> None:
    close = getattr(result, 'close', None)
    if callable(close):
        close()


def _copy_pointer_event(event_ptr) -> PointerEvent:
    event = event_ptr.contents
    return PointerEvent(
        type=raw.DvzPointerEventType(event.type),
        x=float(event.pos[0]),
        y=float(event.pos[1]),
        button=raw.DvzPointerButton(event.button),
        mods=int(event.mods),
        content_scale=float(event.content_scale),
        timestamp_ns=int(event.timestamp_ns),
        user_data=event.user_data,
        wheel_dir=(float(event.content.w.dir[0]), float(event.content.w.dir[1])),
        drag_press_pos=(
            float(event.content.d.press_pos[0]),
            float(event.content.d.press_pos[1]),
        ),
        drag_last_pos=(float(event.content.d.last_pos[0]), float(event.content.d.last_pos[1])),
        drag_shift=(float(event.content.d.shift[0]), float(event.content.d.shift[1])),
        is_press_valid=bool(event.content.d.is_press_valid),
    )


def _copy_keyboard_event(event_ptr) -> KeyboardEvent:
    event = event_ptr.contents
    return KeyboardEvent(
        type=raw.DvzKeyboardEventType(event.type),
        key=raw.DvzKeyCode(event.key),
        mods=int(event.mods),
        user_data=event.user_data,
    )


def _copy_resize_event(event_ptr) -> ResizeEvent:
    event = event_ptr.contents
    return ResizeEvent(
        framebuffer_width=int(event.framebuffer_width),
        framebuffer_height=int(event.framebuffer_height),
        window_width=int(event.window_width),
        window_height=int(event.window_height),
        content_scale_x=float(event.content_scale_x),
        content_scale_y=float(event.content_scale_y),
    )


def _copy_scale_event(event_ptr) -> ScaleEvent:
    event = event_ptr.contents
    return ScaleEvent(
        content_scale_x=float(event.content_scale_x),
        content_scale_y=float(event.content_scale_y),
    )


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

    host = Host(None)
    try:
        return await host.run_process(func, *args, executor=executor, **kwargs)
    finally:
        host.shutdown_executors()


class _InputAdapter:
    def __init__(self, host: Host, router):
        if not router:
            raise ValueError('router is NULL')
        self._host = host
        self.router = router
        self._closed = False
        self._pointer_handlers: dict[str, list[Handler]] = {}
        self._keyboard_handlers: dict[str, list[Handler]] = {}
        self._resize_handlers: list[Handler] = []
        self._scale_handlers: list[Handler] = []
        self._pointer_callback = self._on_pointer
        self._keyboard_callback = self._on_keyboard
        self._resize_callback = self._on_resize
        self._scale_callback = self._on_scale
        self._pointer_subscription_id = 0
        self._keyboard_subscription_id = 0
        self._resize_subscription_id = 0
        self._scale_subscription_id = 0

    def pointer(self, event_name: str = 'all'):
        """Register a handler for a pointer event name."""

        self._assert_open()
        if event_name not in _POINTER_EVENT_NAMES:
            names = ', '.join(sorted(_POINTER_EVENT_NAMES))
            raise ValueError(f"unknown pointer event {event_name!r}; expected one of: {names}")
        self._ensure_pointer_subscribed()

        def decorator(func: Handler) -> Handler:
            self._pointer_handlers.setdefault(event_name, []).append(func)
            return func

        return decorator

    def keyboard(self, event_name: str = 'all'):
        """Register a handler for a keyboard event name."""

        self._assert_open()
        if event_name not in _KEYBOARD_EVENT_NAMES:
            names = ', '.join(sorted(_KEYBOARD_EVENT_NAMES))
            raise ValueError(f"unknown keyboard event {event_name!r}; expected one of: {names}")
        self._ensure_keyboard_subscribed()

        def decorator(func: Handler) -> Handler:
            self._keyboard_handlers.setdefault(event_name, []).append(func)
            return func

        return decorator

    def resize(self):
        """Register a handler for resize events."""

        self._assert_open()
        self._ensure_resize_subscribed()

        def decorator(func: Handler) -> Handler:
            self._resize_handlers.append(func)
            return func

        return decorator

    def scale(self):
        """Register a handler for content-scale events."""

        self._assert_open()
        self._ensure_scale_subscribed()

        def decorator(func: Handler) -> Handler:
            self._scale_handlers.append(func)
            return func

        return decorator

    def close(self) -> None:
        """Unsubscribe handlers installed by this adapter."""

        if self._closed:
            return
        if self._pointer_subscription_id:
            raw.dvz_input_unsubscribe(self.router, self._pointer_subscription_id)
        if self._keyboard_subscription_id:
            raw.dvz_input_unsubscribe(self.router, self._keyboard_subscription_id)
        if self._resize_subscription_id:
            raw.dvz_input_unsubscribe(self.router, self._resize_subscription_id)
        if self._scale_subscription_id:
            raw.dvz_input_unsubscribe(self.router, self._scale_subscription_id)
        self._pointer_handlers.clear()
        self._keyboard_handlers.clear()
        self._resize_handlers.clear()
        self._scale_handlers.clear()
        self._pointer_subscription_id = 0
        self._keyboard_subscription_id = 0
        self._resize_subscription_id = 0
        self._scale_subscription_id = 0
        self._closed = True

    def _assert_open(self) -> None:
        if self._closed:
            raise RuntimeError('Datoviz host input adapter is closed')

    def _ensure_pointer_subscribed(self) -> None:
        if not self._pointer_subscription_id:
            self._pointer_subscription_id = raw.dvz_input_subscribe_pointer(
                self.router, self._pointer_callback, None
            )

    def _ensure_keyboard_subscribed(self) -> None:
        if not self._keyboard_subscription_id:
            self._keyboard_subscription_id = raw.dvz_input_subscribe_keyboard(
                self.router, self._keyboard_callback, None
            )

    def _ensure_resize_subscribed(self) -> None:
        if not self._resize_subscription_id:
            self._resize_subscription_id = raw.dvz_input_subscribe_resize(
                self.router, self._resize_callback, None
            )

    def _ensure_scale_subscribed(self) -> None:
        if not self._scale_subscription_id:
            self._scale_subscription_id = raw.dvz_input_subscribe_scale(
                self.router, self._scale_callback, None
            )

    def _on_pointer(self, _router, event_ptr, _user_data) -> None:
        event = _copy_pointer_event(event_ptr)
        handlers = list(self._pointer_handlers.get('all', ()))
        for name, event_type in _POINTER_EVENT_NAMES.items():
            if name == 'all' or event.type != event_type:
                continue
            handlers.extend(self._pointer_handlers.get(name, ()))
        for handler in handlers:
            self._host._schedule(handler, event)

    def _on_keyboard(self, _router, event_ptr, _user_data) -> None:
        event = _copy_keyboard_event(event_ptr)
        handlers = list(self._keyboard_handlers.get('all', ()))
        for name, event_type in _KEYBOARD_EVENT_NAMES.items():
            if name == 'all' or event_type is None or event.type != event_type:
                continue
            handlers.extend(self._keyboard_handlers.get(name, ()))
        for handler in handlers:
            self._host._schedule(handler, event)

    def _on_resize(self, _router, event_ptr, _user_data) -> None:
        event = _copy_resize_event(event_ptr)
        for handler in list(self._resize_handlers):
            self._host._schedule(handler, event)

    def _on_scale(self, _router, event_ptr, _user_data) -> None:
        event = _copy_scale_event(event_ptr)
        for handler in list(self._scale_handlers):
            self._host._schedule(handler, event)


class ViewAdapter(_InputAdapter):
    """Python host adapter for an existing raw DvzView pointer."""

    def __init__(self, host: Host, view):
        self.handle = view
        router = raw.dvz_view_input(view)
        super().__init__(host, router)

    def request_frame(self) -> None:
        """Request another frame for this view and wake the host loop."""

        raw.dvz_view_request_frame(self.handle)
        self._host.request_frame()

    def render_once(self) -> int:
        """Render this view once through the raw API."""

        return raw.dvz_view_render_once(self.handle)


class Host:
    """Host a raw Datoviz app from Python without hiding the raw C handles."""

    def __init__(self, app, *, fps: float | None = None):
        self.app = app
        self.fps = fps
        self._views: dict[int | None, ViewAdapter] = {}
        self._event: asyncio.Event | None = None
        self._loop: asyncio.AbstractEventLoop | None = None
        self._running = False
        self._closed = False
        self._request_callbacks: dict[int | None, Any] = {}
        self._process_executor: concurrent.futures.ProcessPoolExecutor | None = None

    def __enter__(self) -> Host:
        return self

    def __exit__(self, _exc_type, _exc, _tb) -> None:
        self.close()

    def view(self, raw_view) -> ViewAdapter:
        """Wrap and register an existing raw DvzView pointer."""

        if self._closed:
            raise RuntimeError('Datoviz Host is closed')
        key = _ptr_value(raw_view)
        if key in self._views:
            return self._views[key]
        adapter = ViewAdapter(self, raw_view)
        self._views[key] = adapter
        if self._loop is not None:
            self._install_view_callback(adapter)
        return adapter

    def request_frame(self) -> None:
        """Schedule one render tick on the host loop."""

        if self._loop is not None and self._event is not None:
            self._loop.call_soon_threadsafe(self._event.set)

    def stop(self) -> None:
        """Stop the host loop after the next wakeup."""

        self._running = False
        self.request_frame()

    async def run_thread(
        self,
        func: Callable[..., Any],
        *args: Any,
        executor: concurrent.futures.Executor | None = None,
        **kwargs: Any,
    ) -> Any:
        """Run a callable in a thread executor and await its result."""

        return await run_thread(func, *args, executor=executor, **kwargs)

    async def run_process(
        self,
        func: Callable[..., Any],
        *args: Any,
        executor: concurrent.futures.ProcessPoolExecutor | None = None,
        **kwargs: Any,
    ) -> Any:
        """Run a callable in a process executor and await its result."""

        loop = asyncio.get_running_loop()
        if executor is None:
            if self._process_executor is None:
                self._process_executor = concurrent.futures.ProcessPoolExecutor()
            executor = self._process_executor
        call = functools.partial(func, *args, **kwargs)
        return await loop.run_in_executor(executor, call)

    def shutdown_executors(self) -> None:
        """Shut down executors owned by this host."""

        if self._process_executor is not None:
            self._process_executor.shutdown()
            self._process_executor = None

    async def run_async(self) -> None:
        """Drive dvz_app_render_once() from the current asyncio loop until stopped."""

        self._loop = asyncio.get_running_loop()
        self._event = asyncio.Event()
        self._running = True
        for view in self._views.values():
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
                if self.app is not None:
                    raw.dvz_app_render_once(self.app)
        finally:
            self._clear_view_callbacks()
            self._running = False

    def run(self):
        """Run or schedule the Python-hosted Datoviz app loop."""

        coro = self.run_async()
        try:
            loop = asyncio.get_running_loop()
        except RuntimeError:
            return asyncio.run(coro)
        return loop.create_task(coro)

    def close(self) -> None:
        """Release Python-side callbacks, subscriptions, and executors."""

        if self._closed:
            return
        self.stop()
        self._clear_view_callbacks()
        for view in list(self._views.values()):
            view.close()
        self._views.clear()
        self.shutdown_executors()
        self._closed = True

    def _install_view_callback(self, view: ViewAdapter) -> None:
        key = _ptr_value(view.handle)
        if key in self._request_callbacks:
            return

        def callback(_view, _user_data):
            self.request_frame()

        self._request_callbacks[key] = callback
        raw.dvz_view_set_request_frame_callback(view.handle, callback, None)

    def _clear_view_callbacks(self) -> None:
        for key in list(self._request_callbacks):
            view = self._views.get(key)
            if view is not None:
                raw.dvz_view_set_request_frame_callback(view.handle, None, None)
            self._request_callbacks.pop(key, None)

    def _schedule(self, handler: Handler, event: Any) -> None:
        result = handler(event)
        if not inspect.isawaitable(result):
            return

        loop = self._loop
        if loop is None:
            try:
                loop = asyncio.get_running_loop()
                self._loop = loop
            except RuntimeError:
                _close_awaitable(result)
                warnings.warn(
                    'async Datoviz host handlers require Host.run(), Host.run_async(), '
                    'or a running asyncio loop; handler was skipped',
                    RuntimeWarning,
                    stacklevel=2,
                )
                return
        loop.call_soon_threadsafe(loop.create_task, result)


__all__ = [
    'Host',
    'KeyboardEvent',
    'PointerEvent',
    'ResizeEvent',
    'ScaleEvent',
]
