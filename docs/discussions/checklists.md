# Developer checklists


## Adding a new Python example

* Create a new `bindings/cython/examples/mynewexample.py` file
    * Here is a template code snippet:

    ```python
    """
    # My new example

    This example shows ...
    The text here will be copied automatically to the corresponding page in the documentation.

    """

    # Imports.
    import numpy as np
    import numpy.random as nr
    from datoviz import canvas, run, colormap

    # Add your code here...

    # Start the event loop.
    run()
    ```

* `mkdocs.yml`:
    * Add your new example to `nav > Examples`
* Test
    * Your new example is automatically added to the Python testing suite. As such, your example will run and a screenshot will be made automatically for the documentation website.
    * Run `DVZ_DEBUG=1 pytest -vvsk mynewexample` to test your example interactively.
    * Run `./manage.sh pytest` to check that the full Python testing suite passes.
    * Run `/.manage.sh docs` to regenerate the documentation and run it locally. Check that your example has been automatically included in the website.


## Adding a new C example

* Create a new `examples/mynewexample.h` file
    * Here is a template code snippet:

    ```c
    /*************************************************************************************************/
    /*  My new example.                                                                              */
    /*************************************************************************************************/

    // We include the library header file.
    #include <datoviz/datoviz.h>

    static int demo_mynewexample()
    {
        DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
        DvzGpu* gpu = dvz_gpu_best(app);
        DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);
        DvzScene* scene = dvz_scene(canvas, 1, 1);
        DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

        // Add your code here.

        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        return 0;
    }
    ```

* `examples/examples.h`
    * add `#include "mynewexample.h"`
* `cli/main.c`
    * In `demo()`, add a new line with `SWITCH_DEMO(mynewexample)`


## Implementing a new visual

* Determine the graphics you'll be using, and the list of source and props
* `vislib.h`:
    * Make sure the `DvzVisualType` enum exists, or create a new one
* `visuals.h`:
    * Check that all prop types exist, otherwise add them
* `vislib.c`:
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
* `test_vislib.h`:
    * Add a new visual test declaration
* `test_vislib.c`:
    * Write the body of the test function
* `main.c`:
    * Add the new test to the list of test functions
* Test the new visual without interactivity and save a visual screenshot
* Add a section in the visual reference `docs/reference/visuals.md`, document the props, sources, etc.
* `pydatoviz.pyx`:
    * Add your new visual to the `_VISUALS` dictionary


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
    * If there are parameters, they should be implemented as a struct in the **first user binding** `USER_BINDING`
    * Make sure the GLSL parameters struct matches exactly the one you just defined in `graphics.h`
    * The next bindings should be numbered with `USER_BINDING+1` etc
    * The body of the main fragment shader function should always begin with `CLIP`
* `graphics.c`:
    * Add a new section, with `_graphics_xxx()` and `_graphics_xxx_callback()` if there is a non-default graphics callback function
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
    * In `_show_control()`, add a new switch case and call `_show_xxx()`
    * Implement `_show_xxx()`
* `test_canvas.c`:
    * In `test_canvas_gui()`, add the new control with a call to `dvz_gui_xxx()`
    * Try the next test with `DVZ_DEBUG=1 ./manage.sh test test_canvas_gui`
* `pydatoviz.pyx`:
    * Add your new control to the `_CONTROLS` dictionary
    * In `_get_event_args()`, add a new if statement and make the callback argument binding
    * In `Gui.control()`, add a new if statement and make the binding
    * (*Optional*) In `GuiControl.get()` , add a new if statement and get the control value
    * (*Optional*) In `GuiControl.set()` , add a new if statement and set the control value
* Run `./manage.sh cython`
* Test in a Python example `gui.control('xxx', 'name', ...)`
