# User Input

Datoviz supports interactive user input via **mouse** and **keyboard** event callbacks. These events can be attached to a `Figure` using the `@app.connect()` decorator, which automatically routes events based on the function name.

---

## Overview

Event callbacks respond to:

- Mouse events (click, drag, wheel, move)
- Keyboard events (key press and release)

Each event is handled per figure and can be accessed using a simple decorator-based API.

---

## Connecting input events

To register input callbacks, decorate a function with:

```python
@app.connect(figure)
def on_mouse(ev):
    ...
```

The function name determines the event type:

| Function name | Triggered by     |
| ------------- | ---------------- |
| `on_mouse`    | Mouse actions    |
| `on_keyboard` | Keyboard actions |

---

## Mouse events

Mouse events include actions like move, click, drag, and scroll:

```python
@app.connect(figure)
def on_mouse(ev):
    action = ev.mouse_event()
    x, y = ev.pos()
    print(f'{action} at ({x:.0f}, {y:.0f})')

    if action in ('click', 'double_click'):
        print(f'{ev.button_name()} button')

    if action in ('drag', 'drag_start', 'drag_stop'):
        print(f'{ev.button_name()} drag from {ev.press_pos()}')

    if action == 'wheel':
        print(f'wheel scroll {ev.wheel()}')
```

### Mouse event properties

| Method             | Description                                  |
| ------------------ | -------------------------------------------- |
| `ev.mouse_event()` | Current mouse event name                     |
| `ev.pos()`         | Current mouse position `(x, y)`              |
| `ev.press_pos()`   | Mouse press position during a drag           |
| `ev.button()`      | Raw button enum (int)                        |
| `ev.button_name()` | Button name: `'left'`, `'right'`, `'middle'` |
| `ev.wheel()`       | Vertical wheel scroll amount (float)         |


### Example

```python
--8<-- "examples/features/mouse.py:13:"
```

---

## Keyboard events

Keyboard callbacks receive key presses and releases:

```python
@app.connect(figure)
def on_keyboard(ev):
    print(f'{ev.key_event()} key {ev.key()} ({ev.key_name()})')
```

Each event contains:

* `key()`: the integer key code
* `key_name()`: the name of the key (e.g. `'Escape'`, `'A'`)
* `key_event()`: `'press'` or `'release'`

### Keyboard event properties

| Method          | Description                             |
| --------------- | --------------------------------------- |
| `ev.key_event()`| Current keyboard event name             |
| `ev.key()`      | Raw key code enum (int)                 |
| `ev.key_name()` | Key name string, e.g. `'a'`, `'Escape'` |


### Example

```python
--8<-- "examples/features/keyboard.py:13:"
```

---


## Summary

* ✔️ Simple decorator-based API
* ✔️ Per-figure event handling
* ✔️ Full mouse and keyboard support

See also:

* [Interactivity](interactivity.md) for camera and panzoom controls
* [Events](events.md) for timers, frames, and GUI callbacks
