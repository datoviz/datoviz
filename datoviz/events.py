"""Async event helpers over the raw Datoviz input router."""

from __future__ import annotations

import asyncio
import inspect
import warnings
from dataclasses import dataclass
from typing import Any, Awaitable, Callable

from . import raw


_POINTER_EVENT_NAMES = {
    "release": raw.DvzPointerEventType.DVZ_POINTER_EVENT_RELEASE,
    "press": raw.DvzPointerEventType.DVZ_POINTER_EVENT_PRESS,
    "move": raw.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE,
    "click": raw.DvzPointerEventType.DVZ_POINTER_EVENT_CLICK,
    "double_click": raw.DvzPointerEventType.DVZ_POINTER_EVENT_DOUBLE_CLICK,
    "drag_start": raw.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG_START,
    "drag": raw.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG,
    "drag_stop": raw.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG_STOP,
    "wheel": raw.DvzPointerEventType.DVZ_POINTER_EVENT_WHEEL,
}


@dataclass(frozen=True)
class PointerEvent:
    """Copied pointer event passed to Python handlers."""

    type: raw.DvzPointerEventType
    x: float
    y: float
    button: raw.DvzPointerButton
    mods: int
    content_scale: float
    timestamp_ns: int
    user_data: int | None


Handler = Callable[[PointerEvent], Awaitable[Any] | Any]


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
    )


def _close_awaitable(result: Awaitable[Any]) -> None:
    close = getattr(result, 'close', None)
    if callable(close):
        close()


class EventSource:
    """Subscribe async Python handlers to a Datoviz input router."""

    def __init__(self, router):
        if not router:
            raise ValueError("router is NULL")
        self.router = router
        self._handlers: dict[str, list[Handler]] = {}
        self._loop: asyncio.AbstractEventLoop | None = None
        self._callback = self._on_pointer
        raw.dvz_input_subscribe_pointer(self.router, self._callback, None)

    def on(self, event_name: str):
        """Register a handler for a pointer event name."""

        if event_name not in _POINTER_EVENT_NAMES:
            names = ", ".join(sorted(_POINTER_EVENT_NAMES))
            raise ValueError(f"unknown event {event_name!r}; expected one of: {names}")

        def decorator(func: Handler) -> Handler:
            self._handlers.setdefault(event_name, []).append(func)
            return func

        return decorator

    def bind_loop(self, loop: asyncio.AbstractEventLoop | None = None) -> None:
        """Bind handler dispatch to an asyncio loop."""

        self._loop = loop if loop is not None else asyncio.get_running_loop()

    def close(self) -> None:
        """Unsubscribe this source from the router."""

        if self.router is not None:
            raw.dvz_input_unsubscribe_pointer(self.router, self._callback, None)
            self.router = None
        self._handlers.clear()

    def _schedule(self, handler: Handler, event: PointerEvent) -> None:
        result = handler(event)
        if inspect.isawaitable(result):
            loop = self._loop
            if loop is None:
                try:
                    loop = asyncio.get_running_loop()
                    self._loop = loop
                except RuntimeError:
                    _close_awaitable(result)
                    warnings.warn(
                        'async Datoviz handlers require EventSource.bind_loop() or a running '
                        'asyncio loop; handler was skipped',
                        RuntimeWarning,
                        stacklevel=2,
                    )
                    return
            loop.call_soon_threadsafe(loop.create_task, result)

    def _on_pointer(self, _router, event_ptr, _user_data) -> None:
        event = _copy_pointer_event(event_ptr)
        for name, event_type in _POINTER_EVENT_NAMES.items():
            if event.type != event_type:
                continue
            handlers = list(self._handlers.get(name, ()))
            for handler in handlers:
                self._schedule(handler, event)


class View(EventSource):
    """Thin Python wrapper adding event sugar to a raw DvzView pointer."""

    def __init__(self, view):
        self.handle = view
        router = raw.dvz_view_input(view)
        super().__init__(router)

    def request_frame(self) -> None:
        """Request another frame for this view."""

        raw.dvz_view_request_frame(self.handle)

    def render_once(self) -> int:
        """Render this view once."""

        return raw.dvz_view_render_once(self.handle)
