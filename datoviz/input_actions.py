"""
Strict W3C Actions interchange for discrete single-mouse recordings.

No interpolation, coordinate rounding, clock quantization, wheel unit guessing,
physical-key synthesis or text-to-key conversion is performed. Unsupported
payloads fail explicitly. Binding/fixture metadata stays outside Actions JSON.
"""

from dataclasses import dataclass

from .input_trace import (
    BUTTON_BITS,
    MODIFIERS,
    Observation,
    TraceError,
    _fields,
    _int,
    build_trace,
    plain,
    validate_trace,
)


@dataclass(frozen=True)
class ActionsBinding:
    """Native binding metadata stored separately from the standard payload."""

    window: str
    source_id: str = 'mouse'


def _initial_context(windows, binding):
    state = windows[binding.window]
    # Actions has no native initial-pointer, focus, or window-entry observation.
    # This first interchange profile accepts only the declared default context.
    if (
        state.get('pointerX') != 0
        or state.get('pointerY') != 0
        or state.get('focused') is not True
        or state.get('entered') is not True
    ):
        raise TraceError('Actions requires initial pointer (0, 0), focused and entered host')


def export_actions(trace, *, binding=None):
    """
    Export exactly representable discrete mouse input to standard Actions.

    Same-timestamp order is retained within one source. Every motion is a
    duration-zero action; timestamps must already be exact milliseconds.
    The bound host must start focused and entered, with its pointer at (0, 0).
    """
    validate_trace(trace)
    final_event_time = trace.observations[-1].time_ns if trace.observations else 0
    if int(trace.manifest['duration_ns']) != final_event_time:
        raise TraceError('Actions subset cannot preserve a non-event recording tail')
    windows = trace.manifest['windows']
    binding = binding or ActionsBinding(next(iter(windows)))
    if (
        len(windows) != 1
        or binding.window not in windows
        or not isinstance(binding.source_id, str)
        or not binding.source_id
    ):
        raise TraceError('Actions export requires one bound mouse source/window')
    _initial_context(windows, binding)
    actions, previous = [], 0
    # A newly created WebDriver pointer source starts at viewport (0, 0).
    position = (0, 0)
    for item in trace.observations:
        event = item.event
        if item.time_ns % 1_000_000:
            raise TraceError('Actions cannot represent sub-millisecond event time')
        stamp = item.time_ns // 1_000_000
        if stamp > previous:
            actions.append({'type': 'pause', 'duration': stamp - previous})
        previous = stamp
        if event['type'] not in ('pointermove', 'pointerdown', 'pointerup'):
            raise TraceError(
                'Actions subset supports only mouse input; native/key/text/host loss rejected'
            )
        if (
            any(event[key] for key in MODIFIERS)
            or event['datoviz']['guiMods']
            or event['datoviz']['nativeMods']
        ):
            raise TraceError('Actions mouse source cannot represent keyboard modifiers')
        target = (event['clientX'], event['clientY'])
        if not all(type(value) in (int, float) and float(value).is_integer() for value in target):
            raise TraceError('Actions integer coordinates cannot preserve fractional input')
        metrics = windows[binding.window]['metrics']
        if not (
            0 <= target[0] <= metrics['nativeWidth'] and 0 <= target[1] <= metrics['nativeHeight']
        ):
            raise TraceError('Actions viewport coordinates out of bounds')
        button = event['button']
        if button == -1:
            actions.append(
                {
                    'type': 'pointerMove',
                    'duration': 0,
                    'origin': 'viewport',
                    'x': int(target[0]),
                    'y': int(target[1]),
                }
            )
        else:
            if target != position:
                raise TraceError(
                    'button event coordinate change requires an additional derived motion'
                )
            pressed = bool(event['buttons'] & BUTTON_BITS[button])
            actions.append({'type': 'pointerDown' if pressed else 'pointerUp', 'button': button})
        position = target
    # Native logical units are not CSS units by inference. A fixture must make
    # this binding explicit, even if its actual numerical conversion is 1:1.
    coordinate_binding = trace.manifest['fixture'].get('actions_coordinates')
    if coordinate_binding != 'native-client-equals-css-pixel':
        raise TraceError('Actions viewport coordinate equivalence must be declared by fixture')
    return {
        'actions': [
            {
                'type': 'pointer',
                'id': binding.source_id,
                'parameters': {'pointerType': 'mouse'},
                'actions': actions,
            }
        ]
    }


def import_actions(payload, windows, *, binding=None, fixture=None):
    """
    Import a strict single-mouse Actions payload without invented interpolation.

    Returns (trace, binding); the source identifier remains external metadata.
    Native/viewport coordinate equivalence must be declared in the fixture.
    A trailing pause has no corresponding input observation and is rejected.
    The bound host must start focused and entered, with its pointer at (0, 0).
    """
    _fields(payload, {'actions'})
    if not isinstance(payload['actions'], list) or len(payload['actions']) != 1:
        raise TraceError('Actions subset requires exactly one source')
    source = payload['actions'][0]
    _fields(source, {'type', 'id', 'parameters', 'actions'})
    _fields(source['parameters'], {'pointerType'})
    if (
        source['type'] != 'pointer'
        or source['parameters']['pointerType'] != 'mouse'
        or not isinstance(source['id'], str)
        or not source['id']
    ):
        raise TraceError('Actions subset requires a named mouse source')
    if len(windows) != 1:
        raise TraceError('Actions subset requires one native window')
    binding = binding or ActionsBinding(next(iter(windows)), source['id'])
    if binding.window not in windows or binding.source_id != source['id']:
        raise TraceError('Actions source binding mismatch')
    _initial_context(windows, binding)
    fixture = fixture or {}
    if fixture.get('actions_coordinates') != 'native-client-equals-css-pixel':
        raise TraceError('Actions viewport coordinate equivalence must be declared by fixture')
    if not isinstance(source['actions'], list) or len(source['actions']) > 1_000_000:
        raise TraceError('invalid/bounded Actions list')
    records, stamp, buttons = [], 0, 0
    x, y = 0, 0
    pending_pause = False
    for action in source['actions']:
        if not isinstance(action, dict):
            raise TraceError('invalid Actions entry')
        tag = action.get('type')
        if tag == 'pause':
            _fields(action, {'type', 'duration'})
            if not _int(action['duration'], 1) or pending_pause:
                raise TraceError('Actions pause requires nonnegative integer milliseconds')
            stamp += action['duration'] * 1_000_000
            pending_pause = True
            continue
        pending_pause = False
        if tag == 'pointerMove':
            _fields(action, {'type', 'duration', 'origin', 'x', 'y'})
            if (
                action['duration'] != 0
                or type(action['duration']) is not int
                or action['origin'] != 'viewport'
                or not _int(action['x'])
                or not _int(action['y'])
            ):
                raise TraceError('only instantaneous integer viewport moves are exact')
            metrics = windows[binding.window]['metrics']
            if action['x'] > metrics['nativeWidth'] or action['y'] > metrics['nativeHeight']:
                raise TraceError('Actions viewport coordinates out of bounds')
            x, y, changed, event_tag = action['x'], action['y'], -1, 'pointermove'
        elif tag in ('pointerDown', 'pointerUp'):
            _fields(action, {'type', 'button'})
            changed = action['button']
            if not _int(changed, 0, 4):
                raise TraceError('unsupported mouse button')
            bit, before = BUTTON_BITS[changed], buttons
            if (tag == 'pointerDown') == bool(buttons & bit):
                raise TraceError('duplicate press or unheld release')
            buttons = buttons | bit if tag == 'pointerDown' else buttons & ~bit
            event_tag = (
                'pointerdown' if before == 0 else 'pointerup' if buttons == 0 else 'pointermove'
            )
        else:
            raise TraceError('unsupported Actions entry')
        event = {
            'type': event_tag,
            'pointerId': 1,
            'pointerType': 'mouse',
            'clientX': x,
            'clientY': y,
            'button': changed,
            'buttons': buttons,
            'datoviz': {'guiMods': 0, 'nativeMods': 0},
            **dict.fromkeys(MODIFIERS, False),
        }
        records.append(Observation(len(records), stamp, binding.window, event))
    if pending_pause:
        raise TraceError('trailing pause has no native input representation')
    return build_trace(
        records, plain(windows), fixture=fixture, required_capabilities={'mouse'}
    ), binding
