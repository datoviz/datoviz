# Graphical User Interfaces (GUI)

Datoviz includes a built-in immediate-mode GUI system based on [Dear ImGui](https://github.com/ocornut/imgui), accessible directly from Python. This system allows you to build interactive widgets like buttons, sliders, checkboxes, tables, and trees.

!!! warning

    The GUI API currently mirrors the underlying C API closely. While powerful, it is relatively low-level and may be simplified in a future version.

---

## API Layers

The v0.4 C API has two Dear ImGui entry points:

| Header | Purpose |
| ------ | ------- |
| `datoviz/gui.h` | Datoviz-owned GUI objects and convenience helpers such as `DvzGui`, `DvzGuiViewport`, `dvz_gui_begin()`, sliders, range editors, color editors, sections, fonts, and dockable Datoviz viewports. |
| `datoviz/imgui.h` | Raw generated cimgui bindings with upstream `ig*` names such as `igBegin()`, `igButton()`, and `igShowDemoWindow()`. |

Raw `ig*` calls are intended for advanced C code that needs full Dear ImGui expressiveness beyond
the curated `dvz_gui_*` helpers. They are valid inside a Datoviz GUI callback, where Datoviz has
already selected the current ImGui context for the window.

The C examples show the intended split:

| Example | Use |
| ------- | --- |
| `visuals/point` | regular Datoviz GUI helper API in a visual stress workbench |
| `techniques/gui_viewport` | dockable Datoviz render target inside an ImGui window |
| `techniques/gui_multi_viewport` | multiple dockable Datoviz render targets |

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

!!! note

    You'll find the full list of GUI functions in the [API reference](../reference/api_c.md#gui-functions).

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

There are also integer versions: `dvz.gui_slider_int`, `dvz.gui_slider_ivec2`, etc.

In C, use `dvz_gui_slider_float()`, `dvz_gui_slider_float_format()`,
`dvz_gui_slider_int()`, and the `dvz_gui_slider_float2/3/4()` variants.

### Ranges

```c
float xmin = 0.1f;
float xmax = 0.9f;
bool changed = dvz_gui_range_float(gui, "X range", &xmin, &xmax, 0.01f, 0.0f, 1.0f, "%.2f");
```

The range helper wraps Dear ImGui's two-value drag range control, which is useful for
clip boxes, data windows, and near/far depth-cue ranges.

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

The C wrapper exposes `dvz_gui_color_edit4()`, `dvz_gui_color_picker4()`, and
`dvz_gui_color_edit_dvz()`. The first two edit normalized float RGBA arrays; the
`DvzColor` variant performs the float-to-8-bit conversion for scene colors.

### Sections

```c
dvz_gui_separator_text(gui, "Material");
if (dvz_gui_collapsing_header(gui, "Advanced", 0))
{
    /* widgets */
}
```

Use `dvz_gui_separator_text()` for labeled groups and `dvz_gui_collapsing_header()` for
larger optional subsections. Raw `igTreeNode*()`, tab bars, tables, and child windows remain
available from `datoviz/imgui.h` when a panel needs full Dear ImGui coverage.

---

## Layout Helpers

* `dvz.gui_pos(pos, pivot)`: position next dialog in pixels
* `dvz.gui_size(size)`: set size of next dialog
* `dvz.gui_begin(title, flags)`: start a new dialog
* `dvz.gui_end()`: end current dialog

---

## Example

```python
--8<-- "cleaned/features/gui.py"
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
