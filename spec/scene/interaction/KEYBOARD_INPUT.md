# Keyboard And Text Input

Status: normative v0.4 input contract.

This document distinguishes physical keyboard controls from layout-aware text input across native and hosted backends.

## Physical Key Events

`DvzKeyboardEvent` reports physical key identity, action, and modifier state. `DVZ_KEY_*` printable-key names identify positions using the standard US layout, matching GLFW key tokens; they do not identify the character produced by the active keyboard layout.

Physical key events are the authoritative path for press, repeat, and release state, positional controls, function keys, navigation keys, and modifier tracking. Applications must not use `DVZ_KEY_A` through `DVZ_KEY_Z` to implement commands described to users as typed letters.

Native and hosted backends must normalize the same physical key to the same `DvzKeyCode` when the host exposes physical identity. A hosted backend that cannot recover physical identity emits `DVZ_KEY_UNKNOWN`; it must not silently substitute layout-generated text.

## Text Input Events

Layout-aware input is a separate Unicode code-point stream. The v0.4 public surface adds `DvzTextInputEvent`, `DvzTextInputCallback`, `DVZ_INPUT_EVENT_TEXT`, `dvz_input_subscribe_text()`, `dvz_input_emit_text()`, `dvz_text_emit()`, and `dvz_view_emit_text()` alongside the physical keyboard path.

Each text event carries one Unicode scalar value and borrowed backend/application user data. Backends emit committed text in order and may emit zero, one, or several code points for one physical key action. Text events have no release state and do not promise a one-to-one relationship with physical keys.

Applications use text events for unmodified commands presented as characters, including `m`, `w`, or `r`, and for ordinary text entry. Physical keyboard events remain the correct path for key state and positional controls. General remappable shortcuts and composed editing commands are outside this focused v0.4 addition.

## Backend Requirements

- GLFW routes its Unicode character callback into the input router unless an installed raw integration callback consumes the event.
- Hosted adapters route committed host text through `dvz_view_emit_text()` and preserve Unicode scalar order.
- The Qt adapter derives text from `QKeyEvent::text()` on key press and auto-repeat, decodes the returned UTF-16 string correctly, and does not emit text on release.
- GUI capture may consume text before Datoviz application routing, consistently with physical keyboard capture.
- Synthetic router and hosted events use the same public path as backend events.

## Validation

- Router tests cover subscription, unsubscription, union-event delivery, callback mutation safety, and non-ASCII code points.
- GLFW coverage proves that the physical key callback and character callback remain distinct and that GUI consumption prevents duplicate delivery.
- Hosted coverage proves ordered text delivery independently of physical key identity.
- Qt coverage proves text decoding, repeat behavior, release suppression, and separation from `nativeScanCode()`/physical mapping.
- Public header and binding changes require `just ctypes`, `just ctypes-check`, focused input tests, `just build`, and `git diff --check`.
