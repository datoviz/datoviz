"""Owner-thread recording and replay scheduling without a native library or GPU."""

# Test names describe their scenarios.
# ruff: noqa: D103
from copy import deepcopy
from types import SimpleNamespace

import pytest
from test_input_trace import load_module, pointer, t, trace, windows

r = load_module('input_replay')


class Clock:
    """Explicit fake monotonic wall clock."""

    def __init__(self):
        """Start at an exact zero offset."""
        self.value = 0

    def __call__(self):
        """Read without advancing the clock."""
        return self.value


class Host:
    """Owned fake host exposing the recorder/player seam."""

    def __init__(self, clock):
        """Retain a shared fake wall clock and observable calls."""
        self.clock = clock
        self.state = windows()['host-0']
        self.callback = None
        self.emitted = []
        self.frames = []
        self.ownership = []
        self.unsubscribed = []
        self.pending_count = 0
        self.fail_emit = False

    def snapshot(self):
        """Return an owned neutral fixture snapshot."""
        return deepcopy(self.state)

    def now(self):
        """Return the native input clock's distinct epoch."""
        return 1_000_000_000 + self.clock()

    def subscribe(self, callback):
        """Retain exactly one fake host callback."""
        self.callback = callback
        return 7

    def unsubscribe(self, identifier):
        """Detach before clearing callback storage."""
        self.unsubscribed.append(identifier)
        self.callback = None

    def replay(self, enabled):
        """Observe exclusive input ownership and cleanup."""
        self.ownership.append(enabled)

    def frame(self, stamp, delta):
        """Observe virtual GUI processing timestamps."""
        self.frames.append((stamp, delta))

    def emit(self, event, stamp, **_kwargs):
        """Copy emitted originals independently of later mutation."""
        if self.fail_emit:
            raise t.TraceError('simulated dispatch failure')
        self.emitted.append((t.plain(event), stamp))

    def pending(self):
        """Expose queued GUI event count."""
        return self.pending_count


class Engine:
    """Simulate rendering costs while keeping wall and logical clocks separate."""

    def __init__(self, clock, cost=0, result=0):
        """Set a deterministic render cost and optional failure result."""
        self.clock, self.cost, self.result = clock, cost, result
        self.waits = []

    def dvz_app_step(self, app, wait):
        """Account for bounded waits followed by arbitrary rendering work."""
        self.waits.append(wait)
        self.clock.value += round(wait * 1e9) + self.cost
        return self.result

    def dvz_app_render_once(self, app):
        """Advance real time without changing controlled logical time."""
        self.clock.value += self.cost
        return self.result


def native(kind=1, **fields):
    values = dict(
        type=kind,
        timestamp_ns=1_000_000_000,
        x=1.25,
        y=2.5,
        dx=-0.125,
        dy=0.75,
        button=0,
        action=1,
        key=65,
        logical_key=81,
        scancode=38,
        gui_mods=0,
        mods=0,
        focused=True,
        entered=True,
        utf8=b'',
        byte_size=0,
    )
    values.update(fields)
    return SimpleNamespace(**values)


def test_paced_offered_times_do_not_stretch_after_slow_rendering():
    clock = Clock()
    host = Host(clock)
    engine = Engine(clock, cost=25_000_000)
    saved = trace(pointer(x=1), pointer(x=2), pointer(x=3), stamps=[0, 10_000_000, 20_000_000])
    result = r.Player(engine, None, None, host=host, clock=clock).run(
        saved, mode='paced', tick_seconds=0.01, drain_seconds=0
    )
    observed = host.emitted[3:]
    assert [stamp for _, stamp in observed] == [1_000_000_000, 1_010_000_000, 1_020_000_000]
    assert result['lateness_ns'] == [0, 25_000_000, 15_000_000]
    assert result['delivered'] == result['offered'] == 3
    assert result['complete']
    assert host.ownership == [True, False]
    assert max(engine.waits) <= 0.01


def test_controlled_delivery_and_gui_ticks_ignore_render_cost():
    saved = trace(pointer(x=1), pointer(x=2), pointer(x=3), stamps=[0, 4_000_000, 20_000_000])
    captures = []
    for cost in (1_000, 90_000_000):
        clock = Clock()
        host = Host(clock)
        result = r.Player(Engine(clock, cost=cost), None, None, host=host, clock=clock).run(
            saved, mode='controlled', tick_seconds=0.01, drain_seconds=0
        )
        captures.append((host.frames, host.emitted))
        assert result['complete']
        assert result['logical_tick_ns'] == 10_000_000
        assert result['lateness_ns'] is None
        assert result['logical_delivery_delay_ns'] == [0, 6_000_000, 0]
    assert captures[0] == captures[1]
    assert [stamp for stamp, _ in captures[0][0]] == [1_000_000_000, 1_010_000_000, 1_020_000_000]


@pytest.mark.parametrize('dispatch_failure', [False, True])
def test_failure_always_releases_exclusive_replay(dispatch_failure):
    clock = Clock()
    host = Host(clock)
    host.fail_emit = dispatch_failure
    engine = Engine(clock, result=-1)
    with pytest.raises(t.TraceError):
        r.Player(engine, None, None, host=host, clock=clock).run(
            trace(pointer()), mode='controlled', drain_seconds=0
        )
    assert host.ownership == [True, False]


def test_host_fixture_mismatch_is_rejected_before_ownership():
    clock = Clock()
    host = Host(clock)
    host.state['metrics']['logicalWidth'] += 1
    with pytest.raises(t.TraceError, match='metrics'):
        r.Player(Engine(clock), None, None, host=host, clock=clock).run(trace(pointer()))
    assert host.ownership == []
    assert host.emitted == []


def test_gui_queue_cannot_drain_forever():
    clock = Clock()
    host = Host(clock)
    host.pending_count = 1
    with pytest.raises(t.TraceError, match='did not drain'):
        r.Player(Engine(clock), None, None, host=host, clock=clock).run(
            trace(pointer()), mode='controlled', tick_seconds=0.001, drain_seconds=0
        )
    assert host.ownership == [True, False]


def test_recording_copies_chords_layout_repeat_and_text_commits():
    clock = Clock()
    host = Host(clock)
    recorder = r.Recorder(None, None, host=host).start()
    events = [
        native(2, button=0, action=1, gui_mods=16, mods=16),
        native(2, button=1, action=1),
        native(2, button=0, action=0),
        native(2, button=1, action=0),
        native(3),
        native(4, action=1),
        native(4, action=2),
        native(5, utf8='é🐭'.encode(), byte_size=len('é🐭'.encode()), mods=48),
        native(4, action=0),
    ]
    for i, event in enumerate(events):
        event.timestamp_ns += i
        host.callback(event, 0)
    events[0].x = 900
    saved = recorder.stop()
    assert host.unsubscribed == [7]
    observed = [item.event for item in saved.observations]
    assert [event['type'] for event in observed[:4]] == [
        'pointerdown',
        'pointermove',
        'pointermove',
        'pointerup',
    ]
    assert observed[0]['button'] == 0
    assert observed[1]['button'] == 2
    assert observed[0]['clientX'] == 1.25
    assert observed[0]['datoviz'] == {'nativeMods': 16, 'guiMods': 16}
    assert observed[4]['datoviz']['offsetX'] == -0.125
    assert observed[5]['datoviz']['logicalKey'] == 81
    assert observed[5]['datoviz']['guiMods'] == 0
    assert observed[6]['repeat']
    assert observed[7]['text'] == 'é🐭'
    assert observed[7]['datoviz']['nativeMods'] == 48
    assert len(observed) == len(events)
    assert int(saved.manifest['duration_ns']) >= saved.observations[-1].time_ns


def test_replay_origins_are_never_recorded_twice():
    clock = Clock()
    host = Host(clock)
    recorder = r.Recorder(None, None, host=host).start()
    host.callback(native(), 1)
    saved = recorder.stop()
    assert saved.observations == ()


@pytest.mark.parametrize('failure', ['overflow', 'backwards', 'badbutton', 'clipboard', 'badtext'])
def test_failed_recording_unsubscribes_and_never_returns_complete(failure):
    clock = Clock()
    host = Host(clock)
    recorder = r.Recorder(None, None, host=host, limits=t.Limits(events=1)).start()
    if failure == 'overflow':
        host.callback(native(), 0)
        host.callback(native(), 0)
    elif failure == 'backwards':
        host.callback(native(timestamp_ns=999_999_999), 0)
    elif failure == 'badbutton':
        host.callback(native(2, button=5), 0)
    elif failure == 'clipboard':
        host.callback(native(4, key=86, mods=2), 0)
    else:
        host.callback(native(5, utf8=b'\xff', byte_size=1), 0)
    assert recorder.error is not None
    with pytest.raises(t.TraceError, match='incomplete recording'):
        recorder.stop()
    assert host.unsubscribed == [7]
    assert host.callback is None


def fake_native_api():
    import ctypes as c
    import weakref

    class Size(c.Structure):
        _fields_ = [('width', c.c_uint32), ('height', c.c_uint32)]

    class Vector(c.Structure):
        _fields_ = [('x', c.c_double), ('y', c.c_double)]

    class Metrics(c.Structure):
        _fields_ = [
            ('logical_size', Size),
            ('native_size', Size),
            ('surface_size', Size),
            ('framebuffer_scale', Vector),
            ('native_to_logical', Vector),
            ('unprofiled', c.c_int),
        ]

    class Event(c.Structure):
        _fields_ = [
            ('type', c.c_int),
            ('timestamp_ns', c.c_uint64),
            ('x', c.c_double),
            ('y', c.c_double),
            ('dx', c.c_double),
            ('dy', c.c_double),
            ('button', c.c_int32),
            ('key', c.c_int32),
            ('logical_key', c.c_int32),
            ('gui_mods', c.c_int32),
            ('scancode', c.c_int32),
            ('action', c.c_int32),
            ('mods', c.c_int32),
            ('utf8', c.c_char_p),
            ('byte_size', c.c_uint32),
            ('focused', c.c_bool),
            ('entered', c.c_bool),
            ('metrics', Metrics),
        ]

    metrics = Metrics(
        Size(1280, 900), Size(1280, 900), Size(2560, 1800), Vector(2, 2), Vector(1, 1), 197
    )
    calls = []
    api = SimpleNamespace(
        DvzHostInputEvent=Event,
        DvzHostInputCallback=lambda fn: fn,
        dvz_view_host_window=lambda view: 1,
        dvz_window_input_frame=lambda *args: 0,
        dvz_window_input_replay=lambda *args: 0,
        dvz_app_step=lambda *args: 0,
        dvz_window_metrics=lambda window: c.pointer(metrics),
    )

    def emit(window, pointer, origin):
        value = c.cast(pointer, c.POINTER(Event)).contents
        calls.append(
            {
                name: getattr(value, name)
                for name in (
                    'type',
                    'timestamp_ns',
                    'x',
                    'y',
                    'dx',
                    'dy',
                    'button',
                    'key',
                    'logical_key',
                    'gui_mods',
                    'scancode',
                    'action',
                    'mods',
                    'byte_size',
                    'focused',
                    'entered',
                )
            }
        )
        if value.type == 5:
            calls[-1]['text'] = c.string_at(value.utf8, value.byte_size)
        if value.type == 8:
            calls[-1]['metrics'] = r._metrics(value.metrics)
            calls[-1]['unprofiled'] = value.metrics.unprofiled
        calls[-1]['origin'] = origin
        return 0

    def subscribe(window, callback, user_data):
        api.callback_ref = weakref.ref(callback)
        return 11

    def unsubscribe(window, identifier):
        assert api.callback_ref() is not None
        calls.append({'unsubscribed': identifier})
        return True

    api.dvz_window_emit_host_input = emit
    api.dvz_window_subscribe_host_input = subscribe
    api.dvz_window_unsubscribe_host_input = unsubscribe
    return api, metrics, calls


def test_native_bridge_preserves_button_layout_modifier_text_and_wheel_values():
    api, _, calls = fake_native_api()
    host = r.NativeHost(api, object())
    button = pointer(2, 2, 'pointerdown', x=1.25, y=-2.75)
    button['datoviz'] = {'nativeMods': 16, 'guiMods': 32}
    host.emit(button, 500)
    assert calls[-1] | {'unused': None} == {
        **calls[-1],
        'unused': None,
        'button': 1,
        'mods': 16,
        'gui_mods': 32,
        'type': 2,
        'action': 1,
        'x': 1.25,
        'y': -2.75,
        'timestamp_ns': 500,
        'origin': 1,
    }
    host.emit(
        {
            'type': 'keydown',
            'repeat': True,
            'datoviz': {
                'key': 65,
                'logicalKey': 81,
                'scancode': 38,
                'guiMods': 18,
                'nativeMods': 34,
            },
        },
        501,
    )
    assert calls[-1]['key'] == 65
    assert calls[-1]['logical_key'] == 81
    assert calls[-1]['scancode'] == 38
    assert calls[-1]['gui_mods'] == 18
    assert calls[-1]['mods'] == 34
    assert calls[-1]['action'] == 2
    host.emit({'type': 'datoviz.textcommit', 'datoviz': {'nativeMods': 32}, 'text': 'é🐭'}, 502)
    assert calls[-1]['text'] == 'é🐭'.encode()
    assert calls[-1]['mods'] == 32
    assert calls[-1]['byte_size'] == len('é🐭'.encode())
    host.emit(
        {
            'type': 'wheel',
            'clientX': 2.25,
            'clientY': -3.75,
            'datoviz': {'offsetX': -0.03125, 'offsetY': 0.0625, 'nativeMods': 48},
        },
        503,
    )
    assert calls[-1]['dx'] == -0.03125
    assert calls[-1]['dy'] == 0.0625
    assert calls[-1]['mods'] == 48


def test_native_metrics_emission_owns_copy_and_preserves_unprofiled_fields():
    api, original, calls = fake_native_api()
    metrics = windows()['host-0']['metrics']
    metrics['logicalWidth'] = 1000
    host = r.NativeHost(api, object())
    host.emit({'type': 'datoviz.metrics', 'metrics': metrics}, 100)
    assert calls[-1]['metrics'] == metrics
    assert calls[-1]['unprofiled'] == 197
    assert original.logical_size.width == 1280


def test_native_callback_lives_until_unsubscription_then_is_released():
    import gc

    api, _, calls = fake_native_api()
    host = r.NativeHost(api, object())
    identifier = host.subscribe(lambda event, origin: None)
    gc.collect()
    assert api.callback_ref() is not None
    host.unsubscribe(identifier)
    gc.collect()
    assert api.callback_ref() is None
    assert calls == [{'unsubscribed': 11}]


def test_empty_idle_recording_retains_duration_without_fabricating_input():
    clock = Clock()
    host = Host(clock)
    recorder = r.Recorder(None, None, host=host).start()
    clock.value = 500_000_000
    saved = recorder.stop()
    assert saved.observations == ()
    assert saved.manifest['duration_ns'] == '500000000'
    with pytest.raises(t.TraceError):
        recorder.start()


def test_recorder_stop_never_fabricates_successful_release():
    clock = Clock()
    host = Host(clock)
    recorder = r.Recorder(None, None, host=host).start()
    host.callback(native(2, button=0, action=1), 0)
    with pytest.raises(t.TraceError, match='held'):
        recorder.stop()
    assert host.unsubscribed == [7]


def test_empty_controlled_replay_preserves_idle_tail_pump_duration():
    clock = Clock()
    host = Host(clock)
    saved = t.build_trace([], windows(), duration_ns=30_000_000)
    result = r.Player(Engine(clock), None, None, host=host, clock=clock).run(
        saved, mode='controlled', tick_seconds=0.01, drain_seconds=0
    )
    assert result['offered'] == result['delivered'] == 0
    assert result['complete']
    assert host.frames[-1][0] == 1_030_000_000
    assert len(host.emitted) == 3  # Initial focus, enter, and virtual pointer state only.


def test_initial_entered_snapshot_and_restore_are_explicit():
    clock = Clock()
    host = Host(clock)
    state = windows()
    state['host-0']['entered'] = False
    saved = t.build_trace([], state)
    r.Player(Engine(clock), None, None, host=host, clock=clock).run(
        saved, mode='controlled', drain_seconds=0
    )
    assert ({'type': 'datoviz.enter', 'entered': False}, 1_000_000_000) in host.emitted
    api, _, _ = fake_native_api()
    api.dvz_window_host_input_state = lambda window: SimpleNamespace(
        contents=SimpleNamespace(
            buttons=0,
            mods=0,
            held_key_count=0,
            focused=True,
            entered=False,
            replaying=False,
            x=1.25,
            y=2.75,
        )
    )
    initial = r.NativeHost(api, object()).snapshot()
    assert initial['entered'] is False
    assert initial['pointerX'] == 1.25


def test_text_native_modifier_mask_is_bounded_and_owned():
    value = {'type': 'datoviz.textcommit', 'text': 'é', 'datoviz': {'nativeMods': 48}}
    saved = trace(value)
    value['datoviz']['nativeMods'] = 0
    assert saved.observations[0].event['datoviz']['nativeMods'] == 48
    value['datoviz']['nativeMods'] = 64
    with pytest.raises(t.TraceError, match='modifier'):
        trace(value)
