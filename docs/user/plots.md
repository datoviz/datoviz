# Quickstart: basic plotting

## First plot with the Python API

!!! note
    The Python bindings are at a very early stage of development.

The following code example shows how to make a simple scatter plot in Python:

<!-- CODE_PYTHON bindings/cython/examples/test.py -->

<!-- IMAGE ../images/python_example.png -->


## First plot with the C API

<!-- CODE_C examples/standalone.c -->

<!-- IMAGE ../images/c_example.png -->


## Next steps

The simple examples above showed:

* how to create an application,
* how to create a canvas,
* how to create a panel with an axes controller,
* how to add a visual,
* how to set visual data,
* how to run the application.

These steps already cover a large number of simple plotting applications. The strength of the library is to provide a unified interface to all kinds of visual elements. Next steps will show you:

* the list of currently supported visuals, with their associated user-settable properties,
* the list of currently supported controller types,
* how to create subplots.

Further notes:

* The examples shown here use the so-called **scene API**. Visky provides other, lower-level APIs for those with more advanced needs (see the expert manual). In particular, it is possible to use a "raw" canvas and have full control on what is displayed (for example, simple animations or video games).
