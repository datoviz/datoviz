"""
IPython inputhook integration for Datoviz.

Register a `%gui datoviz` hook that steps the Datoviz event loop while IPython is idle.
"""

from __future__ import annotations

import time
from typing import TYPE_CHECKING, Optional, Any

try:
    from IPython.terminal.pt_inputhooks import register
except Exception:  # pragma: no cover - IPython may not be installed
    register = None

if TYPE_CHECKING:
    from ._app import App


_IDLE_SLEEP = 0.001
_current_app: Optional['App'] = None
_is_registered = False
_is_enabled = False


def _ensure_registered() -> None:
    global _is_registered
    if _is_registered:
        return
    if register is None:
        raise ImportError('IPython is required for `%gui datoviz` integration.')
    register('datoviz', _inputhook_datoviz)
    _is_registered = True


def _disable() -> None:
    global _current_app, _is_enabled
    _current_app = None
    _is_enabled = False


def _inputhook_datoviz(context: Any) -> None:
    """
    Inputhook called while IPython is idle. It keeps the Datoviz window responsive by stepping the
    event loop until input is ready again.
    """
    while True:
        if hasattr(context, 'input_is_ready') and context.input_is_ready():
            return

        app = _current_app
        if app is None:
            time.sleep(_IDLE_SLEEP)
            continue

        try:
            alive = app.step()
        except Exception:
            _disable()
            raise

        if not alive:
            _disable()
            return

        time.sleep(_IDLE_SLEEP)


def enable(app: 'App') -> None:
    """
    Enable `%gui datoviz` integration for the provided app instance.
    """
    _ensure_registered()
    global _current_app, _is_enabled
    _current_app = app
    _is_enabled = True

    # If running inside IPython, switch the active GUI hook to datoviz automatically.
    try:
        from IPython import get_ipython
    except Exception:
        get_ipython = None

    if get_ipython is not None:
        ipy = get_ipython()
        if ipy is not None:
            try:
                ipy.enable_gui('datoviz')
            except Exception:
                # Best effort; user can still run `%gui datoviz` manually.
                pass


def disable() -> None:
    """
    Disable the Datoviz inputhook.
    """
    _disable()
