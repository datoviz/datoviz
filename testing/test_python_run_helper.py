#!/usr/bin/env python3
"""Tests for the top-level Python run helper."""

from __future__ import annotations

import builtins
import ctypes
import importlib
import sys
import types


class _Resources(ctypes.Structure):
    _fields_ = [('window_host', ctypes.py_object)]


class _Raw(types.ModuleType):
    def __init__(self):
        super().__init__('datoviz.raw')
        self.calls = []
        self.app = object()
        self.view = object()
        self.window_host = object()
        self.should_close = False

    def dvz_app(self, scene):
        self.calls.append(('app', scene))
        return self.app

    def dvz_app_with_resources(self, scene, config, resources):
        self.calls.append(('app_with_resources', scene, config, resources._obj.window_host))
        return self.app

    def dvz_app_resources(self):
        self.calls.append(('app_resources',))
        return _Resources()

    def dvz_view_window(self, app, figure, width, height, title):
        self.calls.append(('view_window', app, figure, width, height, title))
        return self.view

    def dvz_app_run(self, app, frame_count):
        self.calls.append(('app_run', app, frame_count))

    def dvz_app_render_once(self, app):
        self.calls.append(('app_render_once', app))
        return 0

    def dvz_view_request_frame(self, view):
        self.calls.append(('view_request_frame', view))
        return 0

    def dvz_app_destroy(self, app):
        self.calls.append(('app_destroy', app))

    def dvz_scene_destroy(self, scene):
        self.calls.append(('scene_destroy', scene))

    def dvz_window_host(self):
        self.calls.append(('window_host',))
        return self.window_host

    def dvz_window_host_poll(self, window_host):
        self.calls.append(('window_host_poll', window_host))

    def dvz_window_host_destroy(self, window_host):
        self.calls.append(('window_host_destroy', window_host))

    def dvz_window_should_close(self, window):
        self.calls.append(('window_should_close', window))
        return self.should_close

    def dvz_app_should_exit(self, app):
        self.calls.append(('app_should_exit', app))
        return self.should_close


class TerminalInteractiveShell:
    def __init__(self):
        self.enabled = []

    def enable_gui(self, name):
        self.enabled.append(name)


class _NeverReadyContext:
    def input_is_ready(self):
        return False


def _install_raw(monkeypatch, datoviz):
    raw = _Raw()
    monkeypatch.setitem(sys.modules, 'datoviz.raw', raw)
    monkeypatch.setattr(datoviz, 'raw', raw, raising=False)
    return raw


def test_run_blocks_and_cleans_up_outside_ipython(monkeypatch):
    import datoviz

    raw = _install_raw(monkeypatch, datoviz)
    monkeypatch.delattr(builtins, 'get_ipython', raising=False)

    result = datoviz.run('scene', 'figure', 320, 240, 'Title')

    assert result is None
    assert ('app_run', raw.app, 0) in raw.calls
    assert raw.calls[-1] == ('app_destroy', raw.app)
    assert ('scene_destroy', 'scene') not in raw.calls


def test_run_returns_live_session_in_terminal_ipython(monkeypatch):
    import datoviz

    dvz_run = importlib.import_module('datoviz.run')

    raw = _install_raw(monkeypatch, datoviz)
    shell = TerminalInteractiveShell()
    monkeypatch.setattr(builtins, 'get_ipython', lambda: shell, raising=False)
    monkeypatch.setattr(dvz_run, '_enable_ipython_inputhook', lambda sh: sh is shell)

    session = datoviz.run('scene', 'figure', 320, 240, 'Title')

    assert session.running
    assert ('app_run', raw.app, 0) not in raw.calls
    assert ('app_with_resources', 'scene', None, raw.window_host) in raw.calls
    assert ('view_request_frame', raw.view) in raw.calls

    session.render_once()
    assert raw.calls[-3:] == [
        ('window_host_poll', raw.window_host),
        ('app_should_exit', raw.app),
        ('app_render_once', raw.app),
    ]

    session.close()
    assert raw.calls[-2:] == [
        ('app_destroy', raw.app),
        ('window_host_destroy', raw.window_host),
    ]
    assert ('scene_destroy', 'scene') not in raw.calls


def test_run_can_be_forced_blocking_in_terminal_ipython(monkeypatch):
    import datoviz

    raw = _install_raw(monkeypatch, datoviz)
    monkeypatch.setattr(builtins, 'get_ipython', lambda: TerminalInteractiveShell(), raising=False)

    result = datoviz.run('scene', 'figure', 320, 240, 'Title', blocking=True)

    assert result is None
    assert ('app_run', raw.app, 0) in raw.calls
    assert ('scene_destroy', 'scene') not in raw.calls


def test_run_can_reopen_borrowed_scene_after_blocking_close(monkeypatch):
    import datoviz

    raw = _install_raw(monkeypatch, datoviz)
    monkeypatch.delattr(builtins, 'get_ipython', raising=False)

    datoviz.run('scene', 'figure', 320, 240, 'Title')
    datoviz.run('scene', 'figure', 320, 240, 'Title')

    assert [call[0] for call in raw.calls].count('app') == 2
    assert [call[0] for call in raw.calls].count('app_run') == 2
    assert [call[0] for call in raw.calls].count('app_destroy') == 2
    assert ('scene_destroy', 'scene') not in raw.calls


def test_live_session_closes_when_native_window_requests_close(monkeypatch):
    dvz_run = importlib.import_module('datoviz.run')
    raw = _Raw()
    session = dvz_run.RunSession(raw, 'scene', raw.app, raw.view, window_host=raw.window_host)
    raw.should_close = True

    assert session.render_once() is None
    assert not session.running
    assert ('app_should_exit', raw.app) in raw.calls
    assert raw.calls[-2:] == [
        ('app_destroy', raw.app),
        ('window_host_destroy', raw.window_host),
    ]
    assert ('scene_destroy', 'scene') not in raw.calls


def test_inputhook_returns_when_no_live_sessions():
    dvz_run = importlib.import_module('datoviz.run')

    dvz_run._close_live_runs()
    dvz_run._datoviz_inputhook(_NeverReadyContext())
