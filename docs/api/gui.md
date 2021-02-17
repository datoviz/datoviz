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

!!! note
    This procedure is subject to change in an upcoming version.

* `controls.h`:
    * Add the `DvzGuiControlType` enum
    * If there are control parameters:
        * Create a `DvzGuiControlXXX` typedef struct
        * Declare the struct
        * Add it to the `DvzGuiControlUnion` union
    * Declare the new `dvz_gui_xxx()` function, and write the docstring
* `controls.c`:
    * Write the body of the `dvz_gui_xxx()` function
* `gui.cpp`:
    * In `show_control()`, add a new switch case and call `_show_xxx()`
    * Implement `_show_xxx()`
* `test_canvas.c`:
    * In `test_canvas_gui_1()`, add the new control with a call to `dvz_gui_xxx()`
    * Try the next test with `DVZ_INTERACT=1 ./manage.sh test test_canvas_gui_1`
* `cydatoviz.pxd`:
    * After `# FUNCTION START`, add `void dvz_gui_xxx()` (no need to put the arguments, the Cython bindings generator will do it automatically)
* `pydatoviz.pyx`:
    * Update the `_CONTROLS` dictionary
    * In `Gui.control()`, add a new if statement and make the binding
    * In `_get_ev_args()`, add a new if statement and make the callback argument binding
* Run `./manage.sh cython`
* Test in a Python example `gui.control('xxx', 'name', ...)`
