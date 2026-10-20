"""Pure input trace tests: import without loading native Datoviz or a GPU."""

# Test names describe the scenarios.
# ruff: noqa: D103

import importlib.util
import json
import subprocess
import sys
import types
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = '_datoviz_trace_test'
if PACKAGE not in sys.modules:
    package = types.ModuleType(PACKAGE)
    package.__path__ = [str(ROOT / 'datoviz')]
    sys.modules[PACKAGE] = package


def load_module(name):
    spec = importlib.util.spec_from_file_location(
        f'{PACKAGE}.{name}', ROOT / 'datoviz' / f'{name}.py'
    )
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


t = load_module('input_trace')
a = load_module('input_actions')


def windows():
    return {
        'host-0': {
            'metrics': {
                'logicalWidth': 1280,
                'logicalHeight': 900,
                'framebufferWidth': 2560,
                'framebufferHeight': 1800,
                'scaleX': 2.0,
                'scaleY': 2.0,
                'nativeWidth': 1280,
                'nativeHeight': 900,
                'nativeToLogicalX': 1.0,
                'nativeToLogicalY': 1.0,
            },
            'focused': True,
            'entered': True,
            'pointerX': 0.0,
            'pointerY': 0.0,
            'buttons': 0,
            'keys': [],
        }
    }


def pointer(button=-1, buttons=0, tag='pointermove', x=0.0, y=0.0):
    return {
        'type': tag,
        'pointerId': 1,
        'pointerType': 'mouse',
        'clientX': x,
        'clientY': y,
        'button': button,
        'buttons': buttons,
        'datoviz': {'guiMods': 0, 'nativeMods': 0},
        **dict.fromkeys(t.MODIFIERS, False),
    }


def key(tag='keydown', repeat=False):
    return {
        'type': tag,
        'code': 'Unidentified',
        'key': 'Unidentified',
        'location': 0,
        'repeat': repeat,
        'datoviz': {'key': 65, 'scancode': 38, 'logicalKey': 81, 'guiMods': 0, 'nativeMods': 0},
        **dict.fromkeys(t.MODIFIERS, False),
    }


def trace(*events, stamps=None, fixture=None, limits=t.DEFAULT_LIMITS):
    stamps = stamps or list(range(len(events)))
    return t.build_trace(
        [t.Observation(i, stamps[i], 'host-0', event) for i, event in enumerate(events)],
        windows(),
        fixture=fixture,
        limits=limits,
    )


def mixed_events():
    return [
        pointer(x=401.25, y=-2.5),
        pointer(0, 1, 'pointerdown'),
        pointer(2, 3),
        pointer(0, 2),
        pointer(2, 0, 'pointerup'),
        {
            'type': 'wheel',
            'clientX': 0.25,
            'clientY': 0.5,
            'datoviz': {
                'convention': 'glfw-scroll-v1',
                'offsetX': -0.002,
                'offsetY': 1.25,
                'nativeMods': 0,
            },
            **dict.fromkeys(t.MODIFIERS, False),
        },
        key(),
        key(repeat=True),
        {'type': 'datoviz.textcommit', 'datoviz': {'nativeMods': 0}, 'text': 'é🐭a\u0301'},
        key('keyup'),
        {'type': 'datoviz.enter', 'entered': False},
        {'type': 'datoviz.metrics', 'metrics': windows()['host-0']['metrics']},
        {'type': 'datoviz.focus', 'focused': False},
    ]


def test_owned_roundtrip_precision_chords_repeat_text(tmp_path):
    events = mixed_events()
    saved = trace(*events)
    events[0]['clientX'] = 999
    assert saved.observations[0].event['clientX'] == 401.25
    with pytest.raises(TypeError):
        saved.observations[0].event['clientX'] = 0
    path = tmp_path / 'manifest.json'
    t.write_trace(saved, path)
    loaded = t.read_trace(path)
    assert loaded == saved
    assert loaded.observations[8].event['text'] == 'é🐭a\u0301'
    assert loaded.observations[5].event['datoviz']['offsetX'] == -0.002


@pytest.mark.parametrize(
    'events',
    [
        [pointer(2, 2, 'pointermove')],
        [pointer(0, 1, 'pointerdown'), pointer(2, 3, 'pointerdown')],
        [pointer(0, 1, 'pointerdown')],
        [pointer(0, 3, 'pointerdown')],
        [pointer(button=True)],
        [key(repeat=True)],
        [key('keyup')],
        [key(), key()],
        [key(), key('keyup', True)],
        [{'type': 'future.event'}],
        [pointer(x=float('nan'))],
        [pointer(x=10**400)],
        [{'type': 'datoviz.textcommit', 'datoviz': {'nativeMods': 0}, 'text': '\ud800'}],
        [{'type': 'datoviz.textcommit', 'datoviz': {'nativeMods': 0}, 'text': ''}],
    ],
)
def test_invalid_states_and_values(events):
    with pytest.raises(t.TraceError):
        trace(*events)


def test_explicit_cancellation_and_focus_loss_clear_state():
    for stop in (
        {'type': 'datoviz.cancel', 'reason': 'capture-stop'},
        {'type': 'datoviz.focus', 'focused': False},
    ):
        t.validate_trace(trace(pointer(0, 1, 'pointerdown'), key(), stop))


def test_capabilities_and_neutral_initial_state():
    original = trace(pointer())
    manifest = t.plain(original.manifest)
    manifest['required_capabilities'].append('touch')
    with pytest.raises(t.TraceError, match='capabilities'):
        t.validate_trace(t.Trace(manifest, original.observations))
    manifest = t.plain(original.manifest)
    manifest['required_capabilities'] = []
    with pytest.raises(t.TraceError, match='capability'):
        t.validate_trace(t.Trace(manifest, original.observations))
    state = windows()
    state['host-0']['buttons'] = 1
    with pytest.raises(t.TraceError, match='neutral'):
        t.build_trace([], state)


@pytest.mark.parametrize(
    'records',
    [
        [t.Observation(0, 10, 'host-0', pointer()), t.Observation(1, 9, 'host-0', pointer())],
        [t.Observation(0, 0, 'host-0', pointer()), t.Observation(0, 1, 'host-0', pointer())],
        [t.Observation(0, 0, 'absent', pointer())],
    ],
)
def test_ordering_and_window_binding(records):
    with pytest.raises(t.TraceError):
        t.build_trace(records, windows())


def test_timestamp_representation_is_decimal_string():
    raw = {'seq': 0, 'time_ns': '123', 'window': 'host-0', 'event': pointer()}
    assert t.build_trace([raw], windows()).observations[0].time_ns == 123
    for invalid in (123, '001', '-1', '1.0', '1e3'):
        raw['time_ns'] = invalid
        with pytest.raises(t.TraceError):
            t.build_trace([raw], windows())


@pytest.mark.parametrize(
    'mutation', ['truncate', 'missingline', 'corrupt', 'badutf8', 'duplicate', 'incomplete']
)
def test_integrity_failures_never_play(tmp_path, mutation):
    saved = trace(pointer(), pointer(x=1))
    path = tmp_path / 'manifest.json'
    t.write_trace(saved, path)
    events = tmp_path / saved.manifest['events_file']
    data = events.read_bytes()
    if mutation == 'truncate':
        events.write_bytes(data[:-1])
    elif mutation == 'missingline':
        events.write_bytes(data.splitlines(keepends=True)[0])
    elif mutation == 'corrupt':
        events.write_bytes(data.replace(b'"clientX":0.0', b'"clientX":2.0', 1))
    elif mutation == 'badutf8':
        events.write_bytes(b'\xff\n')
    elif mutation == 'duplicate':
        events.write_bytes(data.replace(b'"seq":0', b'"seq":0,"seq":0', 1))
    else:
        manifest = json.loads(path.read_text())
        manifest['complete'] = False
        path.write_text(json.dumps(manifest))
    with pytest.raises(t.TraceError):
        t.read_trace(path)


def test_limits_unknown_fields_and_unsafe_paths(tmp_path):
    with pytest.raises(t.TraceError, match='limit'):
        trace(pointer(), limits=t.Limits(events=0))
    with pytest.raises(t.TraceError, match='limit'):
        trace(pointer(), limits=t.Limits(event_bytes=10))
    with pytest.raises(t.TraceError):
        trace(
            {'type': 'datoviz.textcommit', 'datoviz': {'nativeMods': 0}, 'text': 'a' * 20},
            limits=t.Limits(text_bytes=10),
        )
    with pytest.raises(t.TraceError):
        trace({**pointer(), 'unknown': 1})
    path = tmp_path / 'manifest.json'
    path.write_text('{"events_file":"../outside.jsonl"}')
    with pytest.raises(t.TraceError, match='unsafe'):
        t.read_trace(path)


def test_actions_exact_mouse_chords():
    fixture = {'actions_coordinates': 'native-client-equals-css-pixel'}
    original = trace(
        pointer(x=40, y=50),
        pointer(0, 1, 'pointerdown', x=40, y=50),
        pointer(2, 3, x=40, y=50),
        pointer(0, 2, x=40, y=50),
        pointer(2, 0, 'pointerup', x=40, y=50),
        stamps=[0, 10_000_000, 10_000_000, 20_000_000, 20_000_000],
        fixture=fixture,
    )
    payload = a.export_actions(original)
    restored, binding = a.import_actions(payload, windows(), fixture=fixture)
    assert restored.observations == original.observations
    assert a.export_actions(restored, binding=binding) == payload
    assert payload['actions'][0]['actions'][3] == {'type': 'pointerDown', 'button': 2}


@pytest.mark.parametrize(
    'events,stamps,fixture',
    [
        ([pointer(x=0.25)], [0], {'actions_coordinates': 'native-client-equals-css-pixel'}),
        ([pointer()], [123], {'actions_coordinates': 'native-client-equals-css-pixel'}),
        ([pointer()], [0], {}),
        ([{'type': 'datoviz.textcommit', 'datoviz': {'nativeMods': 0}, 'text': 'é'}], [0], {}),
        ([pointer(0, 1, 'pointerdown', x=4), pointer(0, 0, 'pointerup', x=4)], [0, 0], {}),
    ],
)
def test_actions_reject_loss(events, stamps, fixture):
    with pytest.raises(t.TraceError):
        a.export_actions(trace(*events, stamps=stamps, fixture=fixture))


def test_executable_schemas_accept_recording_and_reject_unknown_tag():
    jsonschema = pytest.importorskip('jsonschema')
    directory = ROOT / 'spec' / 'scene' / 'interaction'
    manifest_schema = json.loads((directory / 'input_manifest_v1.schema.json').read_text())
    event_schema = json.loads((directory / 'input_observation_v1.schema.json').read_text())
    saved = trace(*mixed_events())
    jsonschema.Draft202012Validator.check_schema(manifest_schema)
    jsonschema.Draft202012Validator.check_schema(event_schema)
    jsonschema.validate(t.plain(saved.manifest), manifest_schema)
    for item in saved.observations:
        jsonschema.validate(item.as_dict(), event_schema)
    invalid = saved.observations[0].as_dict()
    invalid['event']['type'] = 'unknown.required.tag'
    with pytest.raises(jsonschema.ValidationError):
        jsonschema.validate(invalid, event_schema)


def test_manifest_metadata_tampering_detected(tmp_path):
    saved = trace(pointer(), fixture={'dataset': 'original'})
    path = tmp_path / 'manifest.json'
    t.write_trace(saved, path)
    manifest = json.loads(path.read_text())
    manifest['fixture']['dataset'] = 'changed'
    path.write_text(json.dumps(manifest))
    with pytest.raises(t.TraceError, match='manifest digest'):
        t.read_trace(path)


def test_atomic_manifest_failure_does_not_replace_previous_recording(tmp_path, monkeypatch):
    path = tmp_path / 'manifest.json'
    previous = trace(pointer())
    t.write_trace(previous, path)
    original_replace = t.os.replace

    def fail_manifest(source, destination):
        if destination == path:
            raise OSError('simulated full filesystem')
        original_replace(source, destination)

    monkeypatch.setattr(t.os, 'replace', fail_manifest)
    with pytest.raises(OSError):
        t.write_trace(trace(pointer(x=4)), path)
    assert t.read_trace(path) == previous
    assert not list(tmp_path.glob('.input-trace-*'))


def test_actions_reject_interpolation_nonmouse_and_trailing_pause():
    fixture = {'actions_coordinates': 'native-client-equals-css-pixel'}
    for source in [
        {'type': 'pointer', 'id': 'm', 'parameters': {'pointerType': 'pen'}, 'actions': []},
        {
            'type': 'pointer',
            'id': 'm',
            'parameters': {'pointerType': 'mouse'},
            'actions': [
                {'type': 'pointerMove', 'duration': 100, 'origin': 'viewport', 'x': 1, 'y': 1}
            ],
        },
        {
            'type': 'pointer',
            'id': 'm',
            'parameters': {'pointerType': 'mouse'},
            'actions': [{'type': 'pause', 'duration': 1}],
        },
    ]:
        with pytest.raises(t.TraceError):
            a.import_actions({'actions': [source]}, windows(), fixture=fixture)


def test_codec_loads_without_loading_native_sdk():
    script = """
import importlib.util
import sys
spec = importlib.util.spec_from_file_location('input_trace_pure', sys.argv[1])
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)
assert 'datoviz._ctypes' not in sys.modules
assert 'datoviz' not in sys.modules
"""
    # Fixed interpreter and source literal; no shell or untrusted command input.
    subprocess.run(  # noqa: S603
        [sys.executable, '-c', script, str(ROOT / 'datoviz' / 'input_trace.py')], check=True
    )


def test_event_symlink_cannot_escape_artifact_directory(tmp_path):
    saved = trace(pointer())
    path = tmp_path / 'artifact' / 'manifest.json'
    t.write_trace(saved, path)
    events = path.parent / saved.manifest['events_file']
    outside = tmp_path / 'outside.jsonl'
    events.replace(outside)
    try:
        events.symlink_to(outside)
    except (NotImplementedError, OSError):
        pytest.skip('symlink creation unavailable on this platform')
    with pytest.raises(t.TraceError, match='symbolic links'):
        t.read_trace(path)


def test_named_pipe_is_rejected_without_blocking(tmp_path):
    if not hasattr(t.os, 'mkfifo'):
        pytest.skip('POSIX named pipes unavailable')
    path = tmp_path / 'manifest.json'
    t.os.mkfifo(path)
    with pytest.raises(t.TraceError, match='regular file'):
        t.read_trace(path)


def test_malformed_window_type_and_deep_json_fail_explicitly(tmp_path):
    with pytest.raises(t.TraceError):
        t.build_trace([t.Observation(0, 0, [], pointer())], windows())
    path = tmp_path / 'manifest.json'
    path.write_text('[' * 2000 + '0' + ']' * 2000)
    with pytest.raises(t.TraceError):
        t.read_trace(path)
    path.write_text('{"number":' + '9' * 5000 + '}')
    with pytest.raises(t.TraceError):
        t.read_trace(path)


def test_actions_initial_pointer_and_pause_boundaries_are_explicit():
    state = windows()
    state['host-0']['pointerX'] = 40
    state['host-0']['pointerY'] = 50
    fixture = {'actions_coordinates': 'native-client-equals-css-pixel'}
    saved = t.build_trace(
        [
            t.Observation(0, 0, 'host-0', pointer(0, 1, 'pointerdown', x=40, y=50)),
            t.Observation(1, 0, 'host-0', pointer(0, 0, 'pointerup', x=40, y=50)),
        ],
        state,
        fixture=fixture,
    )
    with pytest.raises(t.TraceError, match='coordinate change'):
        a.export_actions(saved)
    for actions in [
        [{'type': 'pause', 'duration': 0}],
        [{'type': 'pause', 'duration': 1}, {'type': 'pause', 'duration': 2}],
        [{'type': 'pointerMove', 'duration': 0, 'origin': 'viewport', 'x': 100000, 'y': 1}],
    ]:
        payload = {
            'actions': [
                {
                    'type': 'pointer',
                    'id': 'm',
                    'parameters': {'pointerType': 'mouse'},
                    'actions': actions,
                }
            ]
        }
        with pytest.raises(t.TraceError):
            a.import_actions(payload, windows(), fixture=fixture)


def test_duration_preserves_idle_tail_and_rejects_impossible_end():
    saved = t.build_trace([t.Observation(0, 10, 'host-0', pointer())], windows(), duration_ns=50)
    assert saved.manifest['duration_ns'] == '50'
    empty = t.build_trace([], windows(), duration_ns=500_000_000)
    assert empty.observations == ()
    assert empty.manifest['duration_ns'] == '500000000'
    with pytest.raises(t.TraceError, match='duration'):
        t.build_trace([t.Observation(0, 10, 'host-0', pointer())], windows(), duration_ns=9)
    with pytest.raises(t.TraceError, match='duration'):
        t.build_trace([], windows(), duration_ns=True)
    with pytest.raises(t.TraceError, match='duration'):
        t.build_trace([], windows(), duration_ns=t.DEFAULT_LIMITS.duration_ns + 1)


def test_actions_reject_non_event_recording_tail():
    saved = t.build_trace(
        [],
        windows(),
        duration_ns=1,
        fixture={'actions_coordinates': 'native-client-equals-css-pixel'},
    )
    with pytest.raises(t.TraceError, match='tail'):
        a.export_actions(saved)


def test_native_modifier_locks_are_preserved_and_standard_modifiers_must_agree():
    event = pointer()
    event['datoviz'] = {'nativeMods': 48, 'guiMods': 16}
    saved = trace(event)
    assert saved.observations[0].event['datoviz']['nativeMods'] == 48
    event['datoviz']['nativeMods'] = 2
    with pytest.raises(t.TraceError, match='disagree'):
        trace(event)
    event['ctrlKey'] = True
    assert trace(event).observations[0].event['ctrlKey']
    event['datoviz']['guiMods'] = 64
    with pytest.raises(t.TraceError, match='modifier'):
        trace(event)
