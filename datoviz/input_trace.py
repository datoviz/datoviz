"""
Owned, bounded input recordings. This module has no native-library dependency.

The JSON manifest commits an exact UTF-8 JSONL byte stream by SHA-256. Input
records use W3C event vocabulary; GLFW wheel offsets stay explicitly native.
"""

from __future__ import annotations

import hashlib
import json
import math
import os
import re
import stat
import tempfile
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path
from types import MappingProxyType

FORMAT = 'datoviz-input-trace'
PROFILE = 1
SAFE_INTEGER = 2**53 - 1
CAPABILITIES = frozenset(
    {'mouse', 'native-wheel-glfw-v1', 'physical-key', 'text-commit', 'host-state'}
)
STANDARDS = {
    'pointer': 'https://www.w3.org/TR/2026/REC-pointerevents3-20260630/',
    'keyboard-code': 'https://www.w3.org/TR/2025/REC-uievents-code-20250422/',
    'ui-events': 'https://www.w3.org/TR/2026/WD-uievents-20260221/',
    'actions': 'https://www.w3.org/TR/2026/WD-webdriver2-20260702/#actions',
}
MODIFIERS = ('ctrlKey', 'shiftKey', 'altKey', 'metaKey')
# W3C button numbering differs from the buttons bitmask for middle/right.
BUTTON_BITS = {0: 1, 1: 4, 2: 2, 3: 8, 4: 16}


class TraceError(ValueError):
    """Invalid, incomplete, unsupported, or non-benchmarkable trace."""


@dataclass(frozen=True)
class Limits:
    """Hard artifact and parser limits, checked before benchmark playback."""

    manifest_bytes: int = 1_048_576
    event_bytes: int = 65_536
    total_bytes: int = 256 * 1024 * 1024
    events: int = 1_000_000
    text_bytes: int = 16_384
    depth: int = 24
    windows: int = 16
    duration_ns: int = 7 * 24 * 3600 * 1_000_000_000


DEFAULT_LIMITS = Limits()


def _fail(message):
    raise TraceError(message)


def _freeze(value):
    if isinstance(value, Mapping):
        return MappingProxyType({key: _freeze(item) for key, item in value.items()})
    if isinstance(value, (list, tuple)):
        return tuple(_freeze(item) for item in value)
    return value


def plain(value):
    """Return a detached JSON-compatible copy of immutable trace values."""
    if isinstance(value, Mapping):
        return {key: plain(item) for key, item in value.items()}
    if isinstance(value, (tuple, list)):
        return [plain(item) for item in value]
    return value


@dataclass(frozen=True)
class Observation:
    """One ordered observation with owned immutable event payload."""

    seq: int
    time_ns: int
    window: str
    event: Mapping

    def __post_init__(self):
        """Copy mutable containers at the ownership boundary."""
        object.__setattr__(self, 'event', _freeze(self.event))

    def as_dict(self):
        """Serialize nanoseconds as a decimal string without precision loss."""
        return {
            'seq': self.seq,
            'time_ns': str(self.time_ns),
            'window': self.window,
            'event': plain(self.event),
        }


@dataclass(frozen=True)
class Trace:
    """Complete trace envelope and its owned ordered observations."""

    manifest: Mapping
    observations: tuple[Observation, ...]

    def __post_init__(self):
        """Copy mutable containers at the ownership boundary."""
        object.__setattr__(self, 'manifest', _freeze(self.manifest))
        object.__setattr__(self, 'observations', tuple(self.observations))


def _int(value, minimum=0, maximum=SAFE_INTEGER):
    return type(value) is int and minimum <= value <= maximum


def _number(value):
    return type(value) in (int, float) and abs(value) <= SAFE_INTEGER and math.isfinite(value)


def _fields(value, required, optional=()):
    if (
        not isinstance(value, Mapping)
        or set(value) - set(required) - set(optional)
        or set(required) - set(value)
    ):
        _fail(f'unexpected/missing fields (expected {sorted(required)})')


def _tree(value, limits, depth=0):
    if depth > limits.depth:
        _fail('maximum JSON nesting exceeded')
    if isinstance(value, Mapping):
        for key, item in value.items():
            if not isinstance(key, str):
                _fail('non-string JSON key')
            _tree(key, limits, depth + 1)
            _tree(item, limits, depth + 1)
    elif isinstance(value, (tuple, list)):
        for item in value:
            _tree(item, limits, depth + 1)
    elif isinstance(value, str):
        try:
            value.encode('utf-8', errors='strict')
        except UnicodeError as error:
            raise TraceError('invalid UTF-8 scalar') from error
    elif value is not None and type(value) not in (bool, int, float):
        _fail('non-JSON value')
    elif type(value) in (int, float) and not _number(value):
        _fail('non-finite or unsafe number')


def _json_bytes(value):
    return json.dumps(
        plain(value), ensure_ascii=False, allow_nan=False, sort_keys=True, separators=(',', ':')
    ).encode('utf-8')


def _pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            _fail(f'duplicate JSON field: {key}')
        result[key] = value
    return result


def _decode(data, limits):
    try:
        result = json.loads(
            data.decode('utf-8', errors='strict'),
            object_pairs_hook=_pairs,
            parse_constant=lambda _: _fail('non-finite JSON constant'),
        )
        _tree(result, limits)
        return result
    except (UnicodeError, ValueError, RecursionError) as error:
        raise TraceError('malformed UTF-8 JSON') from error


def _metrics(value):
    required = {
        'logicalWidth',
        'logicalHeight',
        'framebufferWidth',
        'framebufferHeight',
        'scaleX',
        'scaleY',
        'nativeWidth',
        'nativeHeight',
        'nativeToLogicalX',
        'nativeToLogicalY',
    }
    _fields(value, required)
    if not all(
        _int(value[key], 1, 1_000_000)
        for key in required - {'scaleX', 'scaleY', 'nativeToLogicalX', 'nativeToLogicalY'}
    ):
        _fail('invalid host dimensions')
    if not all(
        _number(value[key]) and 0 < value[key] <= 100
        for key in ('scaleX', 'scaleY', 'nativeToLogicalX', 'nativeToLogicalY')
    ):
        _fail('invalid host scale')


def _modifiers(event):
    if not all(type(event[key]) is bool for key in MODIFIERS):
        _fail('invalid modifiers')
    native = event.get('datoviz')
    if not isinstance(native, Mapping) or not _int(native.get('nativeMods'), 0, 63):
        _fail('invalid native modifier mask')
    expected = (bool(native['nativeMods'] & bit) for bit in (2, 1, 4, 8))
    if any(event[key] != value for key, value in zip(MODIFIERS, expected, strict=True)):
        _fail('native and standard modifiers disagree')


def _windows(windows, limits):
    if not isinstance(windows, Mapping) or not 1 <= len(windows) <= limits.windows:
        _fail('invalid windows')
    for name, state in windows.items():
        if not re.fullmatch(r'[A-Za-z0-9_-]{1,64}', name):
            _fail('invalid trace-local window name')
        _fields(state, {'metrics', 'focused', 'pointerX', 'pointerY', 'buttons', 'keys'})
        _metrics(state['metrics'])
        if (
            type(state['focused']) is not bool
            or not _number(state['pointerX'])
            or not _number(state['pointerY'])
        ):
            _fail('invalid initial pointer/focus')
        if (
            state['buttons'] != 0
            or type(state['buttons']) is not int
            or state['keys'] not in ([], ())
        ):
            _fail('initial input must be neutral')


def _event(event, state, limits):
    if not isinstance(event, Mapping) or 'type' not in event:
        _fail('event tag required')
    tag = event['type']
    if tag in ('pointermove', 'pointerdown', 'pointerup'):
        _fields(
            event,
            {
                'type',
                'pointerId',
                'pointerType',
                'clientX',
                'clientY',
                'button',
                'buttons',
                'datoviz',
                *MODIFIERS,
            },
        )
        if (
            event['pointerId'] != 1
            or type(event['pointerId']) is not int
            or event['pointerType'] != 'mouse'
        ):
            _fail('only mouse pointer 1 is supported')
        if not all(_number(event[key]) for key in ('clientX', 'clientY')) or not _int(
            event['buttons'], 0, 31
        ):
            _fail('invalid pointer values')
        _modifiers(event)
        _fields(event['datoviz'], {'guiMods', 'nativeMods'})
        if not _int(event['datoviz']['guiMods'], 0, 63):
            _fail('invalid native GUI modifier mask')
        before, after, button = state['buttons'], event['buttons'], event['button']
        if not _int(button, -1, 4):
            _fail('invalid pointer button')
        if button == -1:
            if tag != 'pointermove' or after != before:
                _fail('motion cannot change buttons')
        else:
            bit = BUTTON_BITS[button]
            if before ^ after != bit:
                _fail('button transition must change exactly its button')
            expected = (
                'pointerdown' if before == 0 else 'pointerup' if after == 0 else 'pointermove'
            )
            if tag != expected:
                _fail('incorrect W3C chord transition')
        state['buttons'] = after
    elif tag == 'wheel':
        _fields(event, {'type', 'clientX', 'clientY', 'datoviz', *MODIFIERS})
        _modifiers(event)
        _fields(event['datoviz'], {'convention', 'offsetX', 'offsetY', 'nativeMods'})
        if event['datoviz']['convention'] != 'glfw-scroll-v1':
            _fail('unsupported native wheel convention')
        if not all(
            _number(value)
            for value in (
                event['clientX'],
                event['clientY'],
                event['datoviz']['offsetX'],
                event['datoviz']['offsetY'],
            )
        ):
            _fail('invalid native wheel values')
    elif tag in ('keydown', 'keyup'):
        _fields(event, {'type', 'code', 'key', 'location', 'repeat', 'datoviz', *MODIFIERS})
        _modifiers(event)
        _fields(event['datoviz'], {'key', 'scancode', 'logicalKey', 'guiMods', 'nativeMods'})
        if not all(
            _int(event['datoviz'][key], -1, 2**31 - 1) for key in ('key', 'scancode', 'logicalKey')
        ):
            _fail('invalid native key identity')
        if (
            not isinstance(event['code'], str)
            or len(event['code']) > 64
            or not isinstance(event['key'], str)
            or len(event['key']) > 64
            or not _int(event['location'], 0, 3)
            or type(event['repeat']) is not bool
        ):
            _fail('invalid keyboard fields')
        if not _int(event['datoviz']['guiMods'], 0, 63):
            _fail('invalid native GUI modifier mask')
        identity = (event['datoviz']['key'], event['datoviz']['scancode'])
        if identity == (-1, -1):
            _fail('key must have native identity')
        held = identity in state['keys']
        if tag == 'keydown':
            if held != event['repeat']:
                _fail('repeat must match held key state')
            state['keys'].add(identity)
        else:
            if not held or event['repeat']:
                _fail('invalid key release')
            state['keys'].remove(identity)
    elif tag == 'datoviz.textcommit':
        _fields(event, {'type', 'text'})
        if (
            not isinstance(event['text'], str)
            or not event['text']
            or '\0' in event['text']
            or len(event['text'].encode('utf-8')) > limits.text_bytes
        ):
            _fail('invalid text commit')
    elif tag == 'datoviz.focus':
        _fields(event, {'type', 'focused'})
        if type(event['focused']) is not bool:
            _fail('invalid focus')
        if not event['focused']:
            state['buttons'] = 0
            state['keys'].clear()
        state['focused'] = event['focused']
    elif tag == 'datoviz.enter':
        _fields(event, {'type', 'entered'})
        if type(event['entered']) is not bool:
            _fail('invalid enter')
    elif tag == 'datoviz.metrics':
        _fields(event, {'type', 'metrics'})
        _metrics(event['metrics'])
    elif tag == 'datoviz.cancel':
        _fields(event, {'type', 'reason'})
        if event['reason'] not in ('focus-loss', 'capture-stop', 'replay-abort', 'host-cancel'):
            _fail('invalid cancellation reason')
        state['buttons'] = 0
        state['keys'].clear()
    else:
        _fail(f'unsupported event: {tag}')


def validate_trace(trace, *, limits=DEFAULT_LIMITS, supported_capabilities=CAPABILITIES):
    """Fully validate before playback; return the same immutable trace."""
    manifest = trace.manifest
    _tree(manifest, limits)
    _fields(
        manifest,
        {
            'format',
            'version',
            'standards',
            'required_capabilities',
            'adapter',
            'coordinate_space',
            'windows',
            'fixture',
            'event_count',
            'duration_ns',
            'events_sha256',
            'events_file',
            'complete',
            'final_neutral',
            'trace_sha256',
        },
    )
    if (
        manifest['format'] != FORMAT
        or type(manifest['version']) is not int
        or manifest['version'] != PROFILE
    ):
        _fail('unsupported trace profile')
    if (
        plain(manifest['standards']) != STANDARDS
        or manifest['adapter'] != 'glfw-scroll-v1'
        or manifest['coordinate_space'] != 'native-client'
    ):
        _fail('unsupported standards/adapter profile')
    caps = manifest['required_capabilities']
    if (
        not isinstance(caps, (list, tuple))
        or not all(isinstance(cap, str) for cap in caps)
        or len(set(caps)) != len(caps)
        or set(caps) - set(supported_capabilities)
    ):
        _fail('unsupported required capabilities')
    if manifest['complete'] is not True or manifest['final_neutral'] is not True:
        _fail('incomplete/non-neutral trace')
    if not _int(manifest['event_count'], 0, limits.events) or manifest['event_count'] != len(
        trace.observations
    ):
        _fail('invalid event count')
    duration = manifest['duration_ns']
    if (
        not isinstance(duration, str)
        or not re.fullmatch(r'0|[1-9][0-9]{0,19}', duration)
        or not _int(int(duration), 0, limits.duration_ns)
    ):
        _fail('invalid recording duration')
    duration = int(duration)
    digest = manifest['events_sha256']
    if (
        not isinstance(digest, str)
        or not re.fullmatch('[0-9a-f]{64}', digest)
        or manifest['events_file'] != f'events-{digest}.jsonl'
    ):
        _fail('invalid digest/events filename')
    if not isinstance(manifest['fixture'], Mapping):
        _fail('fixture must be an object')
    _windows(manifest['windows'], limits)
    states = {
        name: {'buttons': 0, 'keys': set(), 'focused': item['focused']}
        for name, item in manifest['windows'].items()
    }
    previous_seq, previous_time, total = -1, 0, 0
    hasher = hashlib.sha256()
    used = set()
    for observation in trace.observations:
        if (
            not _int(observation.seq)
            or observation.seq <= previous_seq
            or not _int(observation.time_ns, 0, limits.duration_ns)
            or observation.time_ns < previous_time
            or not isinstance(observation.window, str)
            or observation.window not in states
        ):
            _fail(f'invalid ordering/window at sequence {observation.seq}')
        _tree(observation.event, limits)
        try:
            _event(observation.event, states[observation.window], limits)
        except TraceError as error:
            raise TraceError(f'sequence {observation.seq}: {error}') from error
        tag = observation.event['type']
        used.add(
            'mouse'
            if tag.startswith('pointer')
            else 'native-wheel-glfw-v1'
            if tag == 'wheel'
            else 'physical-key'
            if tag in ('keydown', 'keyup')
            else 'text-commit'
            if tag == 'datoviz.textcommit'
            else 'host-state'
        )
        line = _json_bytes(observation.as_dict()) + b'\n'
        total += len(line)
        if len(line) > limits.event_bytes or total > limits.total_bytes:
            _fail('trace event byte limit exceeded')
        hasher.update(line)
        previous_seq, previous_time = observation.seq, observation.time_ns
    if previous_time > duration:
        _fail('recording duration precedes final observation')
    if len(states) > 1:
        used.add('multiwindow')
    if used - set(caps):
        _fail('event capability missing from manifest')
    if any(state['buttons'] or state['keys'] for state in states.values()):
        _fail('trace stops with held inputs; explicit cancel required')
    if hasher.hexdigest() != digest:
        _fail('trace digest mismatch')
    metadata = {key: value for key, value in manifest.items() if key != 'trace_sha256'}
    if manifest['trace_sha256'] != hashlib.sha256(_json_bytes(metadata)).hexdigest():
        _fail('manifest digest mismatch')
    if len(_json_bytes(manifest)) + 1 > limits.manifest_bytes:
        _fail('manifest byte limit exceeded')
    return trace


def build_trace(
    observations,
    windows,
    *,
    fixture=None,
    required_capabilities=CAPABILITIES,
    limits=DEFAULT_LIMITS,
    duration_ns=None,
):
    """Construct and validate a complete trace from owned normalized observations."""
    records = []
    total = 0
    hasher = hashlib.sha256()
    for item in observations:
        if len(records) >= limits.events:
            _fail('event count limit exceeded')
        if not isinstance(item, Observation):
            _fields(item, {'seq', 'time_ns', 'window', 'event'})
            stamp = item['time_ns']
            if not isinstance(stamp, str) or not re.fullmatch(r'0|[1-9][0-9]{0,19}', stamp):
                _fail('time_ns must be a decimal string')
            item = Observation(item['seq'], int(stamp), item['window'], item['event'])
        _tree(item.as_dict(), limits)
        line = _json_bytes(item.as_dict()) + b'\n'
        total += len(line)
        if len(line) > limits.event_bytes or total > limits.total_bytes:
            _fail('trace event byte limit exceeded')
        hasher.update(line)
        records.append(item)
    if duration_ns is None:
        duration_ns = records[-1].time_ns if records else 0
    if not _int(duration_ns, 0, limits.duration_ns):
        _fail('invalid recording duration')
    digest = hasher.hexdigest()
    manifest = {
        'format': FORMAT,
        'version': PROFILE,
        'standards': STANDARDS,
        'required_capabilities': sorted(required_capabilities),
        'adapter': 'glfw-scroll-v1',
        'coordinate_space': 'native-client',
        'windows': windows,
        'fixture': fixture or {},
        'event_count': len(records),
        'duration_ns': str(duration_ns),
        'events_sha256': digest,
        'events_file': f'events-{digest}.jsonl',
        'complete': True,
        'final_neutral': True,
    }
    manifest['trace_sha256'] = hashlib.sha256(_json_bytes(manifest)).hexdigest()
    return validate_trace(Trace(manifest, tuple(records)), limits=limits)


def _atomic_write(path, data):
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=path.parent, prefix='.input-trace-', delete=False
        ) as handle:
            temporary = Path(handle.name)
            handle.write(data)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def write_trace(trace, manifest_path, *, limits=DEFAULT_LIMITS):
    """Commit events first, then atomically publish the complete manifest."""
    validate_trace(trace, limits=limits)
    path = Path(manifest_path)
    if path.name == trace.manifest['events_file']:
        _fail('manifest must not replace event stream')
    path.parent.mkdir(parents=True, exist_ok=True)
    events = b''.join(_json_bytes(item.as_dict()) + b'\n' for item in trace.observations)
    _atomic_write(path.parent / trace.manifest['events_file'], events)
    _atomic_write(path, _json_bytes(trace.manifest) + b'\n')


def _open_regular(path):
    # Refuse links/devices/FIFOs. O_NOFOLLOW closes the final-link race on POSIX.
    if path.is_symlink():
        _fail('trace artifacts must not be symbolic links')
    flags = os.O_RDONLY | getattr(os, 'O_NOFOLLOW', 0) | getattr(os, 'O_NONBLOCK', 0)
    try:
        descriptor = os.open(path, flags)
    except OSError as error:
        raise TraceError('cannot open trace artifact as a regular file') from error
    try:
        if not stat.S_ISREG(os.fstat(descriptor).st_mode):
            _fail('trace artifact must be a regular file')
        return os.fdopen(descriptor, 'rb')
    except BaseException:
        os.close(descriptor)
        raise


def read_trace(manifest_path, *, limits=DEFAULT_LIMITS, supported_capabilities=CAPABILITIES):
    """Read a complete bounded artifact. No input may be dispatched before return."""
    path = Path(manifest_path)
    with _open_regular(path) as handle:
        data = handle.read(limits.manifest_bytes + 1)
    if len(data) > limits.manifest_bytes:
        _fail('manifest byte limit exceeded')
    manifest = _decode(data, limits)
    if not isinstance(manifest, dict):
        _fail('manifest must be an object')
    filename = manifest.get('events_file')
    if not isinstance(filename, str) or not re.fullmatch(r'events-[0-9a-f]{64}\.jsonl', filename):
        _fail('unsafe events filename')
    observations, total = [], 0
    hasher = hashlib.sha256()
    with _open_regular(path.parent / filename) as handle:
        while line := handle.readline(limits.event_bytes + 1):
            total += len(line)
            if (
                len(line) > limits.event_bytes
                or total > limits.total_bytes
                or len(observations) >= limits.events
            ):
                _fail('trace size limit exceeded')
            if not line.endswith(b'\n'):
                _fail('truncated event record')
            item = _decode(line, limits)
            _fields(item, {'seq', 'time_ns', 'window', 'event'})
            stamp = item['time_ns']
            if not isinstance(stamp, str) or not re.fullmatch(r'0|[1-9][0-9]{0,19}', stamp):
                _fail('time_ns must be a decimal string')
            observation = Observation(item['seq'], int(stamp), item['window'], item['event'])
            # Canonical bytes make a manifest's digest stable across read/write cycles.
            if line != _json_bytes(observation.as_dict()) + b'\n':
                _fail('noncanonical event encoding')
            hasher.update(line)
            observations.append(observation)
    if hasher.hexdigest() != manifest.get('events_sha256'):
        _fail('event bytes do not match manifest digest')
    return validate_trace(
        Trace(manifest, tuple(observations)),
        limits=limits,
        supported_capabilities=supported_capabilities,
    )
