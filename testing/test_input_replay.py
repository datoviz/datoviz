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
    observed = host.emitted[2:]
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
        native(5, utf8='é🐭'.encode(), byte_size=len('é🐭'.encode())),
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
    assert observed[-1] == {'type': 'datoviz.cancel', 'reason': 'capture-stop'}


def test_replay_origins_are_never_recorded_twice():
    clock = Clock()
    host = Host(clock)
    recorder = r.Recorder(None, None, host=host).start()
    host.callback(native(), 1)
    saved = recorder.stop()
    assert len(saved.observations) == 1  # Explicit stop cancellation only.


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
