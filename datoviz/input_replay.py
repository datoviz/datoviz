"""
Record original host input and replay through the same GUI/scene ingress.

All native operations run on the owning thread. Capture callbacks copy bounded
payloads into memory; only the caller writes a completed trace. Paced playback
keeps offering input against wall time. Controlled playback declares fixed
logical GUI ticks, and is a different workload from paced playback.
"""

from __future__ import annotations

import ctypes
import hashlib
import json
import math
import os
import platform
import sys
from pathlib import Path
from time import monotonic_ns

from .input_trace import (
    DEFAULT_LIMITS,
    MODIFIERS,
    Observation,
    TraceError,
    build_trace,
    plain,
    validate_trace,
)

# GLFW left/right/middle/extra buttons versus W3C's middle/right numbering.
_NATIVE_BUTTON = {0: 0, 1: 2, 2: 1, 3: 3, 4: 4}
_W3C_BUTTON = {value: key for key, value in _NATIVE_BUTTON.items()}
_W3C_BITS = {0: 1, 1: 4, 2: 2, 3: 8, 4: 16}
_METRIC_SIZES = {
    'logicalWidth': ('logical_size', 'width'),
    'logicalHeight': ('logical_size', 'height'),
    'nativeWidth': ('native_size', 'width'),
    'nativeHeight': ('native_size', 'height'),
    'framebufferWidth': ('surface_size', 'width'),
    'framebufferHeight': ('surface_size', 'height'),
    'scaleX': ('framebuffer_scale', 'x'),
    'scaleY': ('framebuffer_scale', 'y'),
    'nativeToLogicalX': ('native_to_logical', 'x'),
    'nativeToLogicalY': ('native_to_logical', 'y'),
}


def _metrics(value):
    return {
        name: getattr(getattr(value, group), field)
        for name, (group, field) in _METRIC_SIZES.items()
    }


def _mods(value):
    return dict(zip(MODIFIERS, (bool(value & bit) for bit in (2, 1, 4, 8)), strict=True))


def _clipboard(event):
    if event['type'] not in ('keydown', 'keyup'):
        return False
    native = event['datoviz']
    keys = {native['key'], native['logicalKey']}
    return bool((native['nativeMods'] | native['guiMods']) & (2 | 8)) and bool(
        keys & {67, 86, 88, 45, 260, 261}
    )


class NativeHost:
    """Borrow a live view's host; reacquire its window before each operation."""

    def __init__(self, dvz, view):
        """Check the optional native API before retaining any callback."""
        self.dvz, self.view = dvz, view
        self.platform = sys.platform
        required = (
            'dvz_view_host_window',
            'dvz_window_emit_host_input',
            'dvz_window_input_frame',
            'dvz_window_input_replay',
            'dvz_app_step',
            'DvzHostInputEvent',
            'DvzHostInputCallback',
        )
        if any(not hasattr(dvz, name) for name in required):
            raise RuntimeError(
                'input replay requires a Datoviz build with host-input replay support'
            )
        self._callbacks = {}

    def window(self):
        """Return a currently owned host or fail before accessing a reaped host."""
        window = self.dvz.dvz_view_host_window(self.view)
        if not window:
            raise TraceError('the view has no live native host')
        return window

    def snapshot(self):
        """Copy current host metrics and neutral-state information."""
        window = self.window()
        if int(self.dvz.dvz_window_backend_type(window)) != int(self.dvz.DVZ_BACKEND_GLFW):
            raise TraceError('this native replay adapter requires a GLFW host')
        state = self.dvz.dvz_window_host_input_state(window).contents
        if state.replaying:
            raise TraceError('the native host is already owned by another replay')
        if state.buttons or (state.mods & 15) or state.held_key_count:
            raise TraceError('capture/replay must start with all buttons and keys released')
        return {
            'metrics': _metrics(self.dvz.dvz_window_metrics(window).contents),
            'focused': bool(state.focused),
            'entered': bool(state.entered),
            'pointerX': state.x,
            'pointerY': state.y,
            'buttons': 0,
            'keys': [],
        }

    def provenance(self):
        """Identify the exact loaded library and presentation policy outside measurement."""
        from . import _ctypes  # noqa: PLC0415

        library = Path(_ctypes.dvz._name)
        digest = hashlib.sha256()
        with library.open('rb') as handle:
            while chunk := handle.read(1024 * 1024):
                digest.update(chunk)
        return {
            'native_library_sha256': digest.hexdigest(),
            'python': platform.python_version(),
            'platform': platform.system(),
            'requested_present_mode': os.environ.get('DVZ_PRESENT_MODE'),
            'resolved_present_mode_vk': None,
            'resolved_present_mode_status': 'unavailable through the public Python API',
        }

    def now(self):
        """Read the native monotonic input clock, not a fabricated gesture clock."""
        return int(self.dvz.dvz_input_timestamp_ns())

    def subscribe(self, callback):
        """Retain the ctypes callback until native unsubscription finishes."""
        wrapped = self.dvz.DvzHostInputCallback(
            lambda window, event, origin, data: callback(event.contents, int(origin))
        )
        identifier = self.dvz.dvz_window_subscribe_host_input(self.window(), wrapped, None)
        if not identifier:
            raise TraceError('could not subscribe to original host input')
        self._callbacks[identifier] = wrapped
        return identifier

    def unsubscribe(self, identifier):
        """Detach before dropping the owned Python callback."""
        window = self.dvz.dvz_view_host_window(self.view)
        if not window:
            raise TraceError('stop capture before reaping or destroying its host')
        if not self.dvz.dvz_window_unsubscribe_host_input(window, identifier):
            raise TraceError('native host did not confirm capture unsubscription')
        self._callbacks.pop(identifier, None)

    def replay(self, enabled):
        """Acquire or release exclusive replay input on the owning thread."""
        window = self.dvz.dvz_view_host_window(self.view)
        if not enabled and not window:
            return
        if not window or self.dvz.dvz_window_input_replay(window, enabled) != 0:
            raise TraceError('could not change exclusive replay input ownership')

    def frame(self, timestamp_ns, delta_seconds):
        """Set virtual GUI processing time before one declared GUI pump."""
        if self.dvz.dvz_window_input_frame(self.window(), timestamp_ns, delta_seconds) != 0:
            raise TraceError('could not supply replay GUI processing time')

    def check(self):
        """Reject platform-dependent operations instead of silently replacing them."""
        window = self.window()
        if self.dvz.dvz_window_should_close(window):
            raise TraceError('the selected replay host was closed')
        if self.dvz.dvz_window_host_input_state(window).contents.unsupported_operation:
            raise TraceError('replay requested an unsupported clipboard or platform operation')

    def poll(self):
        """Poll close requests while replay continues to own all user input."""
        window = self.window()
        self.dvz.dvz_window_host_poll(self.dvz.dvz_window_host_for_window(window))
        self.check()

    def pending(self):
        """Report GUI observations waiting for a later processing tick."""
        return int(self.dvz.dvz_window_input_pending(self.window()))

    def emit(self, event, timestamp_ns, *, replay=True):
        """Translate one validated canonical observation to an owned native POD."""
        dvz = self.dvz
        value = dvz.DvzHostInputEvent()
        value.timestamp_ns = timestamp_ns
        tag = event['type']
        if tag.startswith('pointer'):
            value.x, value.y = event['clientX'], event['clientY']
            value.mods = event['datoviz']['nativeMods']
            if event['button'] == -1:
                value.type = 1
            else:
                value.type = 2
                value.button = _W3C_BUTTON[event['button']]
                value.gui_mods = event['datoviz']['guiMods']
                value.action = int(bool(event['buttons'] & _W3C_BITS[event['button']]))
        elif tag == 'wheel':
            value.type = 3
            value.x, value.y = event['clientX'], event['clientY']
            value.dx, value.dy = event['datoviz']['offsetX'], event['datoviz']['offsetY']
            value.mods = event['datoviz']['nativeMods']
        elif tag in ('keydown', 'keyup'):
            value.type = 4
            value.key = event['datoviz']['key']
            value.logical_key = event['datoviz']['logicalKey']
            value.gui_mods = event['datoviz']['guiMods']
            value.scancode = event['datoviz']['scancode']
            value.action = 0 if tag == 'keyup' else 2 if event['repeat'] else 1
            value.mods = event['datoviz']['nativeMods']
        elif tag == 'datoviz.textcommit':
            value.type = 5
            text = event['text'].encode('utf-8')
            value.utf8, value.byte_size = text, len(text)
            value.mods = event['datoviz']['nativeMods']
        elif tag == 'datoviz.focus':
            value.type, value.focused = 6, event['focused']
        elif tag == 'datoviz.enter':
            value.type, value.entered = 7, event['entered']
        elif tag == 'datoviz.metrics':
            value.type = 8
            # Preserve non-profile native fields from the current resolved metrics.
            value.metrics = self.dvz.dvz_window_metrics(self.window()).contents
            for name, (group, field) in _METRIC_SIZES.items():
                setattr(getattr(value.metrics, group), field, event['metrics'][name])
        elif tag == 'datoviz.cancel':
            value.type = 9
        else:
            raise TraceError(f'unsupported native observation: {tag}')
        if dvz.dvz_window_emit_host_input(self.window(), ctypes.byref(value), int(replay)) != 0:
            raise TraceError('native host rejected an original input observation')


class Recorder:
    """Bounded capture before GUI arbitration; no filesystem I/O in callbacks."""

    def __init__(self, dvz, view, *, fixture=None, limits=DEFAULT_LIMITS, host=None):
        """Retain borrowed handles and own all recorded payloads."""
        self.host = host if host is not None else NativeHost(dvz, view)
        self.fixture, self.limits = plain(fixture or {}), limits
        self.records, self.error = [], None
        self._subscription = None
        self._buttons, self._bytes = 0, 0
        self._started = False

    def start(self):
        """Begin one neutral recording segment on the owning thread."""
        if self._started:
            raise TraceError('a Recorder instance captures one segment only')
        self.initial = self.host.snapshot()
        self.zero = self.host.now()
        self._subscription = self.host.subscribe(self._observe)
        self._started = True
        return self

    def _observe(self, value, origin):
        if origin != 0 or self.error is not None:
            return
        try:
            event = self._copy(value)
            # Clipboard and OS shortcuts cannot be reconstructed by committed-text observations.
            if _clipboard(event):
                raise TraceError('clipboard shortcuts are outside the replay profile')
            stamp = int(value.timestamp_ns) - self.zero
            if stamp < 0 or (self.records and stamp < self.records[-1].time_ns):
                raise TraceError('host observation time precedes the capture boundary')
            record = Observation(len(self.records), stamp, 'host-0', event)
            size = len(json.dumps(record.as_dict(), ensure_ascii=False).encode('utf-8')) + 1
            if (
                len(self.records) >= self.limits.events
                or size > self.limits.event_bytes
                or self._bytes + size > self.limits.total_bytes
            ):
                raise TraceError('capture overflow: recording is incomplete')
            self._bytes += size
            self.records.append(record)
        except Exception as error:
            # Exceptions may never escape a ctypes callback and be silently discarded.
            self.error = error

    def _copy(self, value):
        kind = int(value.type)
        if kind in (1, 2):
            before = self._buttons
            button = -1
            if kind == 2:
                if value.button not in _NATIVE_BUTTON:
                    raise TraceError('unsupported additional native mouse button')
                button = _NATIVE_BUTTON[value.button]
                bit = _W3C_BITS[button]
                self._buttons = before | bit if value.action else before & ~bit
            tag = (
                'pointermove'
                if kind == 1 or (before and self._buttons)
                else 'pointerdown'
                if self._buttons
                else 'pointerup'
            )
            return {
                'type': tag,
                'pointerId': 1,
                'pointerType': 'mouse',
                'clientX': value.x,
                'clientY': value.y,
                'button': button,
                'buttons': self._buttons,
                'datoviz': {'nativeMods': value.mods, 'guiMods': value.gui_mods},
                **_mods(value.mods),
            }
        if kind == 3:
            return {
                'type': 'wheel',
                'clientX': value.x,
                'clientY': value.y,
                'datoviz': {
                    'convention': 'glfw-scroll-v1',
                    'offsetX': value.dx,
                    'offsetY': value.dy,
                    'nativeMods': value.mods,
                },
                **_mods(value.mods),
            }
        if kind == 4:
            if value.key < 0:
                raise TraceError('unidentified physical keys are outside this native profile')
            return {
                'type': 'keyup' if value.action == 0 else 'keydown',
                'code': 'Unidentified',
                'key': 'Unidentified',
                'location': 0,
                'repeat': value.action == 2,
                'datoviz': {
                    'key': value.key,
                    'scancode': value.scancode,
                    'logicalKey': value.logical_key,
                    'guiMods': value.gui_mods,
                    'nativeMods': value.mods,
                },
                **_mods(value.mods),
            }
        if kind == 5:
            if value.byte_size > self.limits.text_bytes:
                raise TraceError('text commit exceeds capture limit')
            # c_char_p field access scans for NUL; native text is a borrowed length-delimited
            # span and need not be terminated. Read its address without dereferencing the field.
            pointer = (
                ctypes.c_void_p.from_address(
                    ctypes.addressof(value) + type(value).utf8.offset
                ).value
                if isinstance(value, ctypes.Structure)
                else value.utf8
            )
            return {
                'type': 'datoviz.textcommit',
                'text': ctypes.string_at(pointer, value.byte_size).decode('utf-8'),
                'datoviz': {'nativeMods': value.mods},
            }
        if kind == 6:
            if not value.focused:
                self._buttons = 0
            return {'type': 'datoviz.focus', 'focused': bool(value.focused)}
        if kind == 7:
            return {'type': 'datoviz.enter', 'entered': bool(value.entered)}
        if kind == 8:
            return {'type': 'datoviz.metrics', 'metrics': _metrics(value.metrics)}
        if kind == 9:
            self._buttons = 0
            return {'type': 'datoviz.cancel', 'reason': 'host-cancel'}
        raise TraceError('unsupported host observation')

    def stop(self):
        """
        Detach and validate before the caller reaps or destroys the captured host.

        The ordinary run loop preserves the closing host until this point. A failed or non-neutral
        capture never produces a complete artifact.
        """
        if self._subscription is None:
            raise TraceError('Recorder is not active')
        try:
            self.host.unsubscribe(self._subscription)
        finally:
            self._subscription = None
        if self.error is not None:
            raise TraceError(f'incomplete recording: {self.error}') from self.error
        stamp = max(self.host.now() - self.zero, self.records[-1].time_ns if self.records else 0)
        return build_trace(
            self.records,
            {'host-0': self.initial},
            fixture=self.fixture,
            duration_ns=stamp,
            adapter_platform=getattr(self.host, 'platform', 'unspecified'),
            limits=self.limits,
        )


class Player:
    """Replay one host with either an original wall schedule or fixed logical ticks."""

    def __init__(self, dvz, app, view, *, host=None, clock=monotonic_ns):
        """Borrow native resources; the caller retains all destruction responsibilities."""
        self.dvz, self.app, self.view = dvz, app, view
        self.host = host if host is not None else NativeHost(dvz, view)
        self.clock = clock

    def run(self, trace, *, mode='paced', tick_seconds=1 / 120, drain_seconds=0.25, finalize=None):
        """
        Validate completely, dispatch on the owner thread, drain and release ownership.

        Paced lateness uses actual dispatch time minus original offered time. Scheduler iteration
        durations include waiting and are deliberately not labelled CPU frame or GPU time.
        Controlled timestamps depend only on logical ticks; wall time remains real in reports.
        """
        validate_trace(trace)
        if (
            mode not in ('paced', 'controlled')
            or not math.isfinite(tick_seconds)
            or not (0 < tick_seconds <= 1)
            or not math.isfinite(drain_seconds)
            or not 0 <= drain_seconds <= 60
        ):
            raise TraceError('invalid playback policy')
        if set(trace.manifest['windows']) != {'host-0'}:
            raise TraceError('this native player supports exactly one host-0 window')
        initial = trace.manifest['windows']['host-0']
        adapter_platform = getattr(self.host, 'platform', None)
        if adapter_platform is not None and trace.manifest['adapter_platform'] != adapter_platform:
            raise TraceError('recorded native adapter platform is incompatible with this host')
        current = self.host.snapshot()
        if plain(current['metrics']) != plain(initial['metrics']):
            raise TraceError('replay host metrics differ from the recorded fixture')
        for item in trace.observations:
            event = item.event
            if event['type'] == 'datoviz.metrics' and plain(event['metrics']) != plain(
                initial['metrics']
            ):
                raise TraceError('this player requires fixed host geometry during replay')
            if event['type'] in ('keydown', 'keyup'):
                if not 0 <= event['datoviz']['key'] <= 348 or not (
                    -1 <= event['datoviz']['logicalKey'] <= 348
                ):
                    raise TraceError('unsupported native key identity')
                if _clipboard(event):
                    raise TraceError('clipboard shortcuts are outside the replay profile')
        tick = max(1, round(tick_seconds * 1e9))
        drain = round(drain_seconds * 1e9)
        records = trace.observations
        end = int(trace.manifest['duration_ns']) + drain
        report = {
            'mode': mode,
            'trace_sha256': trace.manifest['trace_sha256'],
            'offered': len(records),
            'delivered': 0,
            'lateness_ns': [] if mode == 'paced' else None,
            'logical_delivery_delay_ns': [] if mode == 'controlled' else None,
            'native_cpu_frame_timing': 'unavailable in this report; use native profiling',
            'native_gpu_timing': 'unavailable in this report; use native profiling',
            'scheduler_iterations_ns': [],
            'scheduler_iteration_count': 0,
            'scheduler_iteration_total_ns': 0,
            'scheduler_iteration_max_ns': 0,
            'scheduler_samples_truncated': False,
            'logical_tick_ns': tick if mode == 'controlled' else None,
            'gui_drain_ticks': 0,
            'complete': False,
        }
        if hasattr(self.host, 'provenance'):
            report['provenance'] = self.host.provenance()
        acquired = False
        try:
            self.host.replay(True)
            acquired = True
            zero = self.host.now()
            # Recorded initial conditions are virtualized, rather than moving the OS cursor.
            self.host.emit({'type': 'datoviz.focus', 'focused': initial['focused']}, zero)
            self.host.emit(
                {
                    'type': 'pointermove',
                    'clientX': initial['pointerX'],
                    'clientY': initial['pointerY'],
                    'button': -1,
                    'buttons': 0,
                    'datoviz': {'nativeMods': 0, 'guiMods': 0},
                    **dict.fromkeys(MODIFIERS, False),
                },
                zero,
            )
            self.host.emit({'type': 'datoviz.enter', 'entered': initial['entered']}, zero)
            wall_start, logical, index = self.clock(), 0, 0
            previous_processing = 0
            while True:
                wall_now = self.clock()
                processing = max(0, wall_now - wall_start) if mode == 'paced' else logical
                while index < len(records) and records[index].time_ns <= processing:
                    item = records[index]
                    # Rendering delays never rebase the offered schedule.
                    lateness = max(0, self.clock() - wall_start - item.time_ns)
                    self.host.emit(item.event, zero + item.time_ns)
                    if mode == 'paced':
                        report['lateness_ns'].append(lateness)
                    else:
                        report['logical_delivery_delay_ns'].append(processing - item.time_ns)
                    report['delivered'] += 1
                    index += 1
                delta = processing - previous_processing if previous_processing else tick
                self.host.frame(
                    zero + processing, max(1, delta) / 1e9 if mode == 'controlled' else 0
                )
                previous_processing = processing
                begin = self.clock()
                if mode == 'controlled':
                    if hasattr(self.host, 'poll'):
                        self.host.poll()
                    if hasattr(self.dvz, 'dvz_app_should_exit') and self.dvz.dvz_app_should_exit(
                        self.app
                    ):
                        raise TraceError('playback stopped before the trace completed')
                    result = self.dvz.dvz_app_render_once(self.app)
                    logical += tick
                else:
                    next_due = (
                        records[index].time_ns
                        if index < len(records)
                        else max(end, processing + tick)
                        if self.host.pending()
                        else end
                    )
                    # The ordinary scheduler owns frame deadlines. An idle recording may
                    # sleep until its next offered event, preserving on-demand CPU behavior.
                    wait = max(0, next_due - (self.clock() - wall_start)) / 1e9
                    result = self.dvz.dvz_app_step(self.app, wait)
                elapsed = self.clock() - begin
                report['scheduler_iteration_count'] += 1
                report['scheduler_iteration_total_ns'] += elapsed
                report['scheduler_iteration_max_ns'] = max(
                    report['scheduler_iteration_max_ns'], elapsed
                )
                if len(report['scheduler_iterations_ns']) < 65536:
                    report['scheduler_iterations_ns'].append(elapsed)
                else:
                    report['scheduler_samples_truncated'] = True
                if hasattr(self.host, 'check'):
                    self.host.check()
                if result < 0:
                    raise TraceError('playback stopped or a native frame failed')
                if index == len(records) and processing >= end:
                    if self.host.pending() == 0:
                        break
                    report['gui_drain_ticks'] += 1
                    if report['gui_drain_ticks'] > 1024 or self.clock() - wall_start > end + 60e9:
                        raise TraceError('GUI input queue did not drain')
            report['wall_duration_ns'] = self.clock() - wall_start
            report['complete'] = report['delivered'] == len(records)
            if finalize is not None:
                finalization_start = self.clock()

                def pump():
                    # The input schedule has ended. Finish scientific work while live input is
                    # still isolated; these owner-thread frames are outside the measured phase.
                    self.host.frame(zero + previous_processing, tick_seconds)
                    if hasattr(self.host, 'poll'):
                        self.host.poll()
                    if hasattr(self.dvz, 'dvz_app_should_exit') and self.dvz.dvz_app_should_exit(
                        self.app
                    ):
                        raise TraceError('playback stopped during scientific finalization')
                    result = self.dvz.dvz_app_render_once(self.app)
                    if result < 0:
                        raise TraceError('a scientific finalization frame failed')
                    if hasattr(self.host, 'check'):
                        self.host.check()

                report['finalization'] = finalize(pump)
                report['finalization_duration_ns'] = self.clock() - finalization_start
        finally:
            if acquired:
                # Releasing ownership cancels recognizer/GUI held state without clicks.
                self.host.replay(False)
        return report
