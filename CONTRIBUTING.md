# Contributing notes

This document is a work in progress.

## Management commands

We use the [just](https://github.com/casey/just) tool for all management commands.
The commands are implemented in `justfile`.


## Branches

* The `main` branch is reserved to stable releases.
* Development occurs in the `dev` branch.


## Python bindings

The Python (ctypes) bindings are automatically generated into `datoviz/__init__.py` which is commmitted in the repository, but should not be manually edited.


## Continuous integration/continuous delivery

GitHub Actions-based CI/CD is not yet active in this repository.
In the near future, we intend to activate it for:

- cross-platform automated testing of the C library and Python bindings ;
- automated build of the Python wheels on all supported platforms ;
- automated build of the documentation and gallery.


## Debugging


### Console logging

Set this environment variable:

* `DVZ_LOG_LEVEL=2`: info logging level, default
* `DVZ_LOG_LEVEL=1`: debug logging level
* `DVZ_LOG_LEVEL=0`: trace logging level (caution: extremely verbose)


### Datoviz Intermediate Protocol requests

User-exposed Datoviz commands generate an internal stream of rendering requests which are processed in real time by the Datoviz Vulkan renderer.
You can inspect these commands for debugging purposes, and depending on whether the requests are correct, determine whether the bug occurs in the high-level code generating these requests (most frequent case), or in the Vulkan renderer.

* `DVZ_VERBOSE=prt`: print a YAML representation of the requests to the standard output.


### Screenshot

Set this environment variable to force offscreen rendering of all Datoviz applications:

* `DVZ_CAPTURE_PNG=path/to/image.png`: save the figure to a PNG file.


### Performance

Set these environment variable to display some performance statistics

* `DVZ_FPS=1`: display an FPS counter (frames per second).
* `DVZ_MONITOR=1`: display a memory monitor (allocated GPU memory).

_Note_: the FPS computation algorithm is currently suboptimal, it will be improved later. Contributions welcome.
