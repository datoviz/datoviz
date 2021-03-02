# Developer checklists

## Implementing a new visual

* Determine the graphics you'll be using, and the list of source and props
* `builtin_visuals.h`:
    * Make sure the `DvzVisualType` enum exists, or create a new one
* `visuals.h`:
    * Check that all prop types exist, otherwise add them
* `builtin_visuals.c`:
    * Make a new section for the visual code
    * Write the main visual function
        * Specify the graphics pipeline(s)
        * Specify the sources:
            * Vertex buffer
            * Index buffer (optional)
            * Param buffer (optional)
            * Textures (optional)
        * Specify the props
            * The cast and copy spec are not mandatory if there is a custom baking callback function
            * Note that the special POS prop is DVEC3 and is typically cast to VEC3, but only *after* baking and panel transform
            * Add a length prop if there are multiple objects
            * Add the common props
            * Add the param props
            * Specify the props that need DPI scaling
        * Specify the baking callback function (optional)
    * Write the baking callback function
        * Take DPI scaling into account for props that require it
    * Add a new switch case in `dvz_visual_builtin()`
* `test_builtin_visuals.h`:
    * Add a new visual test declaration
* `test_builtin_visuals.c`:
    * Write the body of the test function
* `main.c`:
    * Add the new test to the list of test functions
* Test the new visual without interactivity and save a visual screenshot
* Add a section in the visual reference `docs/reference/visuals.md`, document the props, sources, etc.



## Implementing a new graphics

* `graphics.h`:
    * Add the typedef of the vertex, item, params structures
    * Add new section and structs for vertex, item, params structures
        * Make sure all fields in the params struct have a byte size that is not 3 or divisible by 3
    * Put comments after each struct field (used by automatic documentation generator)
* `vklite.h`:
    * Make sure the `DvzGraphicsType` enum exists, or create a new one
* Shaders: `graphics_xxx.vert|frag`:
    * Don't forget to import `common.glsl` in all shaders
    * The first user binding should be params, should match exactly the struct
    * The next bindings should be numbered with USER_BINDING+1 etc
    * The body of the main fragment shader function should always begin with `CLIP`
* `graphics.c`:
    * Add new section, with `_graphics_xxx()` and `_graphics_xxx_callback()` if there is a non-default graphics callback function
    * Write the main graphics function
        * Specify the shaders
        * Specify the primitive type
        * Specify the vertex attributes, should match the vertex shader
        * Add the common slots
        * Add slots for params/textures
        * Specify the graphics callback function
    * (optional) Write the graphics callback function
        * This function is called when a new item is added to the graphics. What an "item" is is up to the create of a graphics. It's typically the smallest bit of data that has a meaning in the context of the graphics. The graphics callback is mostly used for pre-upload CPU-side "triangulation" of the data, so that the visuals that reuse this graphics don't have to know the details of the triangulation.
    * Add the switch case in `dvz_graphics_builtin()`
* `test_graphics.h`:
    * Add the new graphics test declaration
* `test_graphics.c`:
    * Write the body of the test function
    * Set to save the screenshot with the graphics name
* `main.c`:
    * Add the new test to the list of test functions
* Test the graphics without interactivity and save the graphics screenshot
* Add graphics section in `docs/reference/graphics.md`



## Implementing a new GUI control

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
