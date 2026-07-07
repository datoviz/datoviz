"""Top-level run helper and terminal-IPython integration."""

from __future__ import annotations

import atexit
import ctypes
import time
from typing import Any


_LIVE_RUNS = set()
_INPUTHOOK_REGISTERED = False
_ATEXIT_REGISTERED = False


def _view_window(raw, view):
    """Return the borrowed DvzWindow* from DvzView's private prefix."""

    required = ('DvzApp', 'DvzFigure', 'DvzWindow', 'DvzCanvas')
    if view is None or not all(hasattr(raw, name) for name in required):
        return None

    class _DvzViewPrefix(ctypes.Structure):
        _fields_ = [
            ('app', ctypes.POINTER(raw.DvzApp)),
            ('figure', ctypes.POINTER(raw.DvzFigure)),
            ('kind', ctypes.c_int),
            ('window', ctypes.POINTER(raw.DvzWindow)),
            ('canvas', ctypes.POINTER(raw.DvzCanvas)),
        ]

    return ctypes.cast(view, ctypes.POINTER(_DvzViewPrefix)).contents.window


class RunSession:
    """Live session returned by nonblocking ``datoviz.run()`` calls."""

    def __init__(self, raw, scene, app, view, *, window_host=None, fps: float = 60.0):
        self._raw = raw
        self.scene = scene
        self.app = app
        self.view = view
        self.window_host = window_host
        self.fps = fps
        self.running = True
        self._closed = False
        self._last_tick = 0.0
        _LIVE_RUNS.add(self)

    def __repr__(self) -> str:
        state = 'closed' if self._closed else ('running' if self.running else 'stopped')
        return f'<datoviz run session {state}>'

    def request_frame(self):
        """Request a redraw of the live view."""

        if not self._closed and self.view is not None:
            return self._raw.dvz_view_request_frame(self.view)
        return None

    def render_once(self):
        """Poll the hosted window, then render one app frame."""

        if self._closed or not self.running:
            return None
        self._poll()
        if self.should_close():
            self.close()
            return None
        return self._raw.dvz_app_render_once(self.app)

    def stop(self) -> None:
        """Stop the nonblocking prompt integration without destroying C objects."""

        self.running = False

    def close(self) -> None:
        """Destroy the live app/window resources owned by this session."""

        if self._closed:
            return
        self.running = False
        _LIVE_RUNS.discard(self)
        if self.app:
            self._raw.dvz_app_destroy(self.app)
            self.app = None
        if self.window_host:
            self._raw.dvz_window_host_destroy(self.window_host)
            self.window_host = None
        self.view = None
        self.scene = None
        self._closed = True

    def should_close(self) -> bool:
        """Return whether the native window has received a close request."""

        if self._closed or self.view is None:
            return True
        window = _view_window(self._raw, self.view)
        if not window:
            return False
        return bool(self._raw.dvz_window_should_close(window))

    def _poll(self) -> None:
        if self.window_host:
            self._raw.dvz_window_host_poll(self.window_host)

    def _inputhook_tick(self, now: float) -> None:
        if self._closed or not self.running:
            return
        min_interval = 1.0 / self.fps if self.fps > 0 else 0.0
        if min_interval > 0 and now - self._last_tick < min_interval:
            return
        self._last_tick = now
        self.render_once()


def _close_live_runs() -> None:
    for session in list(_LIVE_RUNS):
        session.close()


def _ensure_atexit() -> None:
    global _ATEXIT_REGISTERED
    if not _ATEXIT_REGISTERED:
        atexit.register(_close_live_runs)
        _ATEXIT_REGISTERED = True


def _ipython_shell() -> Any | None:
    try:
        get_ipython = __builtins__.get('get_ipython')  # type: ignore[union-attr]
    except AttributeError:
        get_ipython = getattr(__builtins__, 'get_ipython', None)
    if not callable(get_ipython):
        return None
    try:
        return get_ipython()
    except Exception:
        return None


def _is_terminal_ipython(shell: Any | None) -> bool:
    return shell is not None and shell.__class__.__name__ == 'TerminalInteractiveShell'


def _datoviz_inputhook(context) -> None:
    while not context.input_is_ready():
        now = time.monotonic()
        active = [session for session in list(_LIVE_RUNS) if session.running]
        for session in active:
            session._inputhook_tick(now)
        time.sleep(0.005 if active else 0.05)


def _enable_ipython_inputhook(shell: Any | None) -> bool:
    global _INPUTHOOK_REGISTERED
    if shell is None or not hasattr(shell, 'enable_gui'):
        return False
    try:
        from IPython.terminal import pt_inputhooks
    except Exception:
        return False
    if not _INPUTHOOK_REGISTERED:
        pt_inputhooks.register('datoviz', _datoviz_inputhook)
        _INPUTHOOK_REGISTERED = True
    try:
        shell.enable_gui('datoviz')
    except Exception:
        return False
    return True


def _make_nonblocking_session(raw, scene, figure, width, height, title_bytes) -> RunSession:
    window_host = raw.dvz_window_host()
    if not window_host:
        raise RuntimeError('dvz_window_host() failed')

    resources = raw.dvz_app_resources()
    resources.window_host = window_host
    app = raw.dvz_app_with_resources(scene, None, ctypes.byref(resources))
    if not app:
        raw.dvz_window_host_destroy(window_host)
        raise RuntimeError('dvz_app_with_resources() failed')

    view = raw.dvz_view_window(app, figure, width, height, title_bytes)
    if not view:
        raw.dvz_app_destroy(app)
        raw.dvz_window_host_destroy(window_host)
        raise RuntimeError('dvz_view_window() failed')

    session = RunSession(raw, scene, app, view, window_host=window_host)
    session.request_frame()
    _ensure_atexit()
    return session


def run(scene, figure, width=800, height=600, title='Datoviz', blocking: bool | None = None):
    """Open a native window and run it.

    In terminal IPython, the default is nonblocking: the prompt remains usable and a live-session
    handle is returned. In regular Python scripts, the default remains the blocking app loop.
    The scene and figure are borrowed and remain owned by the caller.
    """

    from . import raw as _raw

    title_bytes = title.encode() if isinstance(title, str) else title
    shell = _ipython_shell()
    if blocking is None:
        blocking = not _is_terminal_ipython(shell)

    if not blocking:
        session = _make_nonblocking_session(_raw, scene, figure, width, height, title_bytes)
        if _is_terminal_ipython(shell) and not _enable_ipython_inputhook(shell):
            session.close()
            raise RuntimeError('IPython Datoviz input hook could not be enabled')
        return session

    app = _raw.dvz_app(scene)
    if not app:
        raise RuntimeError('dvz_app() failed')
    view = _raw.dvz_view_window(app, figure, width, height, title_bytes)
    if not view:
        _raw.dvz_app_destroy(app)
        raise RuntimeError('dvz_view_window() failed')
    _raw.dvz_app_run(app, 0)
    _raw.dvz_app_destroy(app)
    return None


__all__ = ['RunSession', 'run']
