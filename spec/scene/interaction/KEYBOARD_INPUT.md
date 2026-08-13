# Keyboard And Text Input

Status: normative v0.4 input contract.

This document distinguishes physical keyboard controls from layout-aware text input across native and hosted backends.

## Physical Key Events

`DvzKeyboardEvent` reports physical key identity, action, and modifier state. `DVZ_KEY_*` printable-key names identify positions using the standard US layout, matching GLFW key tokens; they do not identify the character produced by the active keyboard layout.

Physical key events are the authoritative path for press, repeat, and release state, positional controls, function keys, navigation keys, and modifier tracking. Applications must not use `DVZ_KEY_A` through `DVZ_KEY_Z` to implement commands described to users as typed letters.

Native and hosted backends must normalize the same physical key to the same `DvzKeyCode` when the host exposes physical identity. A hosted backend that cannot recover physical identity emits `DVZ_KEY_UNKNOWN`; it must not silently substitute layout-generated text.

## Text Input Events

Layout-aware input is a separate committed UTF-8 stream. The v0.4 public surface adds `DvzInputTextEvent`, `DvzInputTextCallback`, `DVZ_INPUT_EVENT_TEXT`, `dvz_input_subscribe_text()`, `dvz_input_emit_text()`, and hosted injection through `dvz_view_emit_text()` alongside the physical keyboard path.

`DvzInputTextEvent` carries a borrowed UTF-8 byte span, its byte size, a modifier snapshot, and borrowed backend/application user data. Dispatch is synchronous, so the byte span remains valid only until the emit call returns. Each event preserves one host commit boundary and may contain one or several Unicode scalar values. Text events have no release state and do not promise a one-to-one relationship with physical keys.

Applications use text events for unmodified commands presented as characters, including `m`, `w`, or `r`, and for ordinary text entry. Physical keyboard events remain the correct path for key state and positional controls. General remappable shortcuts and composed editing commands are outside this focused v0.4 addition.

The API has two injection layers only: `dvz_input_emit_text()` dispatches a complete event through a router, while `dvz_view_emit_text()` lets a hosted adapter inject a UTF-8 commit into a view. There is no `dvz_text_input_emit()` or `dvz_text_emit()` helper; the `dvz_text_*` namespace remains reserved for scene text rendering.

## Backend Requirements

- GLFW routes its Unicode character callback into the input router unless an installed raw integration callback consumes the event.
- Hosted adapters route complete committed UTF-8 spans through `dvz_view_emit_text()` and preserve host commit boundaries.
- The Qt adapter derives ordinary committed text from `QKeyEvent::text()` on key press and auto-repeat, converts it to valid UTF-8, and does not emit text on release. Composition/preedit and full IME behavior are outside this v0.4 slice unless `QInputMethodEvent::commitString()` is implemented and tested separately.
- GUI integration feeds its own text input first and suppresses application routing only when the GUI owns keyboard/text capture; raw, GUI, and application paths must not duplicate a commit.
- Synthetic router and hosted events use the same public path as backend events.

## Validation

- Router tests cover subscription, unsubscription, union-event delivery, callback mutation safety, borrowed-span lifetime, malformed UTF-8 rejection, non-ASCII commits, multiple scalars, and modifier snapshots.
- GLFW coverage proves that the physical key callback and character callback remain distinct and that GUI consumption prevents duplicate delivery.
- Hosted coverage proves ordered UTF-8 commit delivery independently of physical key identity.
- Qt coverage proves UTF-8 conversion, repeat behavior, release suppression, and separation from native scan-code physical mapping. Unrepresentable physical keys produce `DVZ_KEY_UNKNOWN` rather than layout-generated text masquerading as a key identity.
- Public header and binding changes require `just ctypes`, `just ctypes-check`, focused input tests, `just build`, and `git diff --check`.
