# GUI

## GUI controls


### `dvz_gui()`
### `dvz_gui_checkbox()`
### `dvz_gui_slider_float()`
### `dvz_gui_slider_int()`
### `dvz_gui_label()`
### `dvz_gui_textbox()`
### `dvz_gui_button()`
### `dvz_gui_colormap()`
### `dvz_gui_demo()`
### `dvz_gui_destroy()`



## Implementing a new control

Dear ImGui provides an impressive number of controls. Using it directly requires to write custom C++ callback functions. To simplify the creation of simple GUIs, Datoviz provides a simple declarative API that allows to specify in advance basic GUI dialogs. This is particularly convenient when using the Python bindings, since there is no need to wrap C++ code nor to call Python code at every frame.

!!! note
    You can still use Dear ImGui directly, [there is an example in the How to guides](../howto/standalone_imgui.md).

Here is a short checklist for Datoviz contributors who'd like to implement a new control in the Dear ImGui wrapper.
