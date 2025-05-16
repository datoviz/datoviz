# Graphical User Interfaces (GUI)

Datoviz includes a built-in immediate-mode GUI system based on [Dear ImGui](https://github.com/ocornut/imgui), accessible directly from Python. This system allows you to build interactive widgets like buttons, sliders, checkboxes, tables, and trees.

⚠️ **Note:** The GUI API currently mirrors the underlying C API closely. While powerful, it is relatively low-level and may be simplified in a future version.

---

## Enabling GUI

To use GUI elements, the `Figure` must be created with `gui=True`:

```python
figure = app.figure(gui=True)
```

You must then register a GUI callback using `@app.connect(figure)` and name the function `on_gui`.

---

## Immediate Mode

The GUI is **immediate-mode**: widgets are recreated from scratch at every frame. Their state (e.g. a checkbox value) must be stored and updated explicitly between frames. Use `dvz.Out()` to define mutable state values:

```python
from datoviz import Out
checked = Out(True)

if dvz.gui_checkbox('Check me', checked):
    print('Checked:', checked.value)
```

---

## Common Widgets

### Buttons

```python
dvz.gui_button('Click me', width, height)
```

Returns `True` if pressed during this frame.

### Sliders

```python
slider = Out(25.0)
dvz.gui_slider('My slider', 0.0, 100.0, slider)
```

Also supports `vec2`, `vec3`, `vec4` versions for multi-axis sliders.

### Checkboxes

```python
checked = Out(True)
dvz.gui_checkbox('Checkbox', checked)
```

### Tables

```python
dvz.gui_table('Table', rows, cols, labels, selected, flags)
```

`labels` is a list of strings (`rows * cols`), `selected` is a boolean array tracking selected rows (must be defined outside the GUI callback functions for data persistence).

### Trees

```python
if dvz.gui_node('Parent'):
    dvz.gui_selectable('Child')
    dvz.gui_pop()
```

Use `dvz.gui_clicked()` to detect clicks on selectable items.

### Color Picker

```python
color = dvz.vec3(0.5, 0.2, 0.7)
dvz.gui_colorpicker('Color', color, 0)
```

---

## Layout Helpers

* `dvz.gui_pos(pos, pivot)`: position next dialog in pixels
* `dvz.gui_size(size)`: set size of next dialog
* `dvz.gui_begin(title, flags)`: start a new dialog
* `dvz.gui_end()`: end current dialog

---

## Example

```python
--8<-- "examples/features/gui.py"
```

This example demonstrates several widgets including a button, tree, table, color picker, and slider.

---

## Summary

| Feature      | Notes                                                |
| ------------ | ---------------------------------------------------- |
| Built-in GUI | Immediate mode, updated every frame                  |
| API Style    | Low-level, mirrors C API closely                     |
| Widgets      | Buttons, sliders, tables, trees, color pickers, etc. |
| State        | Use `Out(...)` to track mutable widget values        |
| Rendering    | GUI renders as an overlay inside the figure window   |

---

See also:

* [Events](events.md): for frame and timer callbacks
