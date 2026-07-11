#!/usr/bin/env python3
"""Native input event logging with a synthetic offscreen smoke path."""

from __future__ import annotations

import sys
from dataclasses import dataclass

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 5


@dataclass
class InputEventsState:
    subscription_id: int = 0
    pointer_count: int = 0
    keyboard_count: int = 0
    resize_count: int = 0
    verbose: bool = True


def _pointer_event_label(event_type: int) -> str:
    labels = {
        dvz.DVZ_POINTER_EVENT_RELEASE: "release",
        dvz.DVZ_POINTER_EVENT_PRESS: "press",
        dvz.DVZ_POINTER_EVENT_MOVE: "move",
        dvz.DVZ_POINTER_EVENT_CLICK: "click",
        dvz.DVZ_POINTER_EVENT_DOUBLE_CLICK: "double-click",
        dvz.DVZ_POINTER_EVENT_DRAG_START: "drag-start",
        dvz.DVZ_POINTER_EVENT_DRAG: "drag",
        dvz.DVZ_POINTER_EVENT_DRAG_STOP: "drag-stop",
        dvz.DVZ_POINTER_EVENT_WHEEL: "wheel",
        dvz.DVZ_POINTER_EVENT_NONE: "none",
        dvz.DVZ_POINTER_EVENT_ALL: "all",
    }
    return labels.get(event_type, "unknown")


def _pointer_button_label(button: int) -> str:
    labels = {
        dvz.DVZ_POINTER_BUTTON_NONE: "none",
        dvz.DVZ_POINTER_BUTTON_LEFT: "left",
        dvz.DVZ_POINTER_BUTTON_MIDDLE: "middle",
        dvz.DVZ_POINTER_BUTTON_RIGHT: "right",
    }
    return labels.get(button, "unknown")


def _keyboard_event_label(event_type: int) -> str:
    labels = {
        dvz.DVZ_KEYBOARD_EVENT_NONE: "none",
        dvz.DVZ_KEYBOARD_EVENT_PRESS: "press",
        dvz.DVZ_KEYBOARD_EVENT_REPEAT: "repeat",
        dvz.DVZ_KEYBOARD_EVENT_RELEASE: "release",
    }
    return labels.get(event_type, "unknown")


def _key_label(key: int) -> str:
    if dvz.DVZ_KEY_A <= key <= dvz.DVZ_KEY_Z:
        return chr(ord("A") + key - dvz.DVZ_KEY_A)
    if dvz.DVZ_KEY_0 <= key <= dvz.DVZ_KEY_9:
        return chr(ord("0") + key - dvz.DVZ_KEY_0)
    labels = {
        dvz.DVZ_KEY_UNKNOWN: "unknown",
        dvz.DVZ_KEY_NONE: "none",
        dvz.DVZ_KEY_SPACE: "space",
        dvz.DVZ_KEY_ESCAPE: "escape",
        dvz.DVZ_KEY_ENTER: "enter",
        dvz.DVZ_KEY_TAB: "tab",
        dvz.DVZ_KEY_BACKSPACE: "backspace",
        dvz.DVZ_KEY_LEFT: "left",
        dvz.DVZ_KEY_RIGHT: "right",
        dvz.DVZ_KEY_UP: "up",
        dvz.DVZ_KEY_DOWN: "down",
    }
    return labels.get(key, "unknown")


def _format_mods(mods: int) -> str:
    if mods == dvz.DVZ_KEY_MODIFIER_NONE:
        return "none"
    labels = []
    for bit, label in (
        (dvz.DVZ_KEY_MODIFIER_SHIFT, "shift"),
        (dvz.DVZ_KEY_MODIFIER_CONTROL, "control"),
        (dvz.DVZ_KEY_MODIFIER_ALT, "alt"),
        (dvz.DVZ_KEY_MODIFIER_SUPER, "super"),
    ):
        if mods & int(bit):
            labels.append(label)
    known = (
        int(dvz.DVZ_KEY_MODIFIER_SHIFT)
        | int(dvz.DVZ_KEY_MODIFIER_CONTROL)
        | int(dvz.DVZ_KEY_MODIFIER_ALT)
        | int(dvz.DVZ_KEY_MODIFIER_SUPER)
    )
    if mods & ~known:
        labels.append(f"0x{mods & ~known:x}")
    return "+".join(labels)


def _handle_pointer(event, state: InputEventsState) -> None:
    state.pointer_count += 1
    if not state.verbose:
        return
    mods = _format_mods(event.mods)
    if event.type == dvz.DVZ_POINTER_EVENT_WHEEL:
        print(
            "input_events: pointer wheel "
            f"dx={event.content.w.dir[0]:.2f} dy={event.content.w.dir[1]:.2f} "
            f"x={event.pos[0]:.1f} y={event.pos[1]:.1f} mods={mods}"
        )
    else:
        print(
            "input_events: pointer "
            f"{_pointer_event_label(event.type)} button={_pointer_button_label(event.button)} "
            f"x={event.pos[0]:.1f} y={event.pos[1]:.1f} mods={mods}"
        )


def _handle_keyboard(event, state: InputEventsState) -> None:
    state.keyboard_count += 1
    if state.verbose:
        print(
            "input_events: key "
            f"{_keyboard_event_label(event.type)} key={_key_label(event.key)} "
            f"mods={_format_mods(event.mods)}"
        )


def _handle_resize(event, state: InputEventsState) -> None:
    state.resize_count += 1
    if state.verbose:
        print(
            "input_events: resize "
            f"framebuffer={event.framebuffer_width}x{event.framebuffer_height} "
            f"window={event.window_width}x{event.window_height} "
            f"scale={event.content_scale_x:.2f}x{event.content_scale_y:.2f}"
        )


def _subscribe_events(router, state: InputEventsState) -> None:
    def callback(_router, event_ptr, _user_data):
        event = event_ptr.contents
        if event.type == dvz.DVZ_INPUT_EVENT_POINTER:
            _handle_pointer(event.content.pointer, state)
        elif event.type == dvz.DVZ_INPUT_EVENT_KEYBOARD:
            _handle_keyboard(event.content.keyboard, state)
        elif event.type == dvz.DVZ_INPUT_EVENT_RESIZE:
            _handle_resize(event.content.resize, state)

    state.subscription_id = dvz.dvz_input_subscribe_event(router, callback, None)
    if state.subscription_id == 0:
        raise RuntimeError("dvz_input_subscribe_event() failed")


def _unsubscribe_events(router, state: InputEventsState) -> None:
    if state.subscription_id:
        dvz.dvz_input_unsubscribe(router, state.subscription_id)
        state.subscription_id = 0


def _add_points(scene, panel) -> None:
    positions = np.array(
        [
            [-0.60, -0.30, 0.0],
            [-0.30, +0.25, 0.0],
            [+0.00, -0.05, 0.0],
            [+0.35, +0.32, 0.0],
            [+0.62, -0.24, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.GREEN, ex.TEXT, ex.YELLOW, ex.RED)
    sizes = np.array([22.0, 34.0, 42.0, 30.0, 24.0], dtype=np.float32)

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": sizes,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    ex.add_visual(panel, point)


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    _add_points(scene, panel)
    return scene, figure, panel


def _emit_synthetic_events(view) -> None:
    if dvz.dvz_view_emit_resize(view, ex.WIDTH, ex.HEIGHT, ex.WIDTH, ex.HEIGHT, 1.0, 1.0) != 0:
        raise RuntimeError("dvz_view_emit_resize() failed")
    if (
        dvz.dvz_view_emit_pointer(
            view,
            dvz.DVZ_POINTER_EVENT_MOVE,
            140.0,
            160.0,
            ex.WIDTH,
            ex.HEIGHT,
            dvz.DVZ_POINTER_BUTTON_NONE,
            dvz.DVZ_KEY_MODIFIER_NONE,
        )
        != 0
    ):
        raise RuntimeError("dvz_view_emit_pointer(move) failed")
    if (
        dvz.dvz_view_emit_pointer(
            view,
            dvz.DVZ_POINTER_EVENT_PRESS,
            140.0,
            160.0,
            ex.WIDTH,
            ex.HEIGHT,
            dvz.DVZ_POINTER_BUTTON_LEFT,
            dvz.DVZ_KEY_MODIFIER_NONE,
        )
        != 0
    ):
        raise RuntimeError("dvz_view_emit_pointer(press) failed")
    if (
        dvz.dvz_view_emit_wheel(
            view, 140.0, 160.0, ex.WIDTH, ex.HEIGHT, 0.0, +1.0, dvz.DVZ_KEY_MODIFIER_NONE
        )
        != 0
    ):
        raise RuntimeError("dvz_view_emit_wheel() failed")
    if (
        dvz.dvz_view_emit_key(
            view, dvz.DVZ_KEYBOARD_EVENT_PRESS, dvz.DVZ_KEY_A, dvz.DVZ_KEY_MODIFIER_NONE
        )
        != 0
    ):
        raise RuntimeError("dvz_view_emit_key(press) failed")
    if (
        dvz.dvz_view_emit_key(
            view, dvz.DVZ_KEYBOARD_EVENT_RELEASE, dvz.DVZ_KEY_A, dvz.DVZ_KEY_MODIFIER_NONE
        )
        != 0
    ):
        raise RuntimeError("dvz_view_emit_key(release) failed")


def _run_synthetic(verbose: bool = True) -> InputEventsState:
    scene, figure, _panel = _build_scene()
    app = dvz.dvz_app(scene)
    if not app:
        dvz.dvz_scene_destroy(scene)
        raise RuntimeError("dvz_app() failed")
    try:
        view = dvz.dvz_view_offscreen(app, figure, ex.WIDTH, ex.HEIGHT)
        if not view:
            raise RuntimeError("dvz_view_offscreen() failed")
        router = dvz.dvz_view_input(view)
        if not router:
            raise RuntimeError("dvz_view_input() failed")
        state = InputEventsState(verbose=verbose)
        _subscribe_events(router, state)
        try:
            _emit_synthetic_events(view)
            status = dvz.dvz_view_render_once(view)
            if status != dvz.DVZ_CANVAS_FRAME_READY:
                raise RuntimeError("dvz_view_render_once() failed")
            ex.capture_smoke(view)
            if state.pointer_count < 3 or state.keyboard_count != 2 or state.resize_count < 1:
                raise RuntimeError("input event callbacks did not receive emitted events")
            return state
        finally:
            _unsubscribe_events(router, state)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def main(argv: list[str] | None = None) -> None:
    argv = sys.argv[1:] if argv is None else argv
    if "--synthetic" in argv or ex.SMOKE_MODE:
        _run_synthetic(verbose=True)
        return

    scene, figure, _panel = _build_scene()
    app = dvz.dvz_app(scene)
    if not app:
        dvz.dvz_scene_destroy(scene)
        raise RuntimeError("dvz_app() failed")
    try:
        view = dvz.dvz_view_window(app, figure, ex.WIDTH, ex.HEIGHT, b"Input Events")
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        router = dvz.dvz_view_input(view)
        if not router:
            raise RuntimeError("dvz_view_input() failed")
        state = InputEventsState(verbose=True)
        _subscribe_events(router, state)
        try:
            print("input_events: move/click/scroll/resize/type in the window; close it to exit")
            ex.run_app(app, view)
        finally:
            _unsubscribe_events(router, state)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


if __name__ == "__main__":
    main()
