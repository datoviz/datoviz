# Contributing notes

This is a work in progress.

## Debugging


### Console logging

Set this environment variable:

* `DVZ_LOG_LEVEL=2`: (info, default)
* `DVZ_LOG_LEVEL=1`: debug
* `DVZ_LOG_LEVEL=0`: trace (caution: extremely verbose)


### Datoviz Intermediate Protocol requests

User-exposed Datoviz commands generate an internal stream of rendering requests which are processed in real time by the Datoviz Vulkan renderer.
You can inspect these commands for debugging purposes, and depending on whether the requests are correct, determine whether the bug occurs in the high-level code generating these requests (most frequent case), or in the Vulkan renderer.

* `DVZ_VERBOSE=prt`: print a YAML representation of the requests to the standard output.


### Screenshot

Set this environment variable to force offscreen rendering of all Datoviz applications:

* `DVZ_CAPTURE_PNG=path/to/image.png`
