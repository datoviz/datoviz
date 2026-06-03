# Array-Aware Python Facade

Status: proposed v0.4 Python binding direction.

This note records the intended Python API layer above the exact generated `ctypes` binding. The goal
is to make Datoviz usable from Python with normal NumPy arrays while preserving the C-shaped
Datoviz API and leaving high-level plotting to GSP/VisPy2.


## Decision

The recommended direct-engine Python import should be:

```python
import datoviz as dvz
```

The top-level `datoviz` package should expose an array-aware facade over the C API:

1. preserve `dvz_*` function names;
2. accept NumPy arrays and compatible Python buffer objects for known data arguments;
3. infer pointer/count or pointer/byte-size arguments only where binding policy declares the
   relationship;
4. pass through unsupported or unannotated calls to the exact raw binding shape;
5. avoid prefixless aliases, object-oriented scene wrappers, and plotting functions.

The exact generated binding remains available as:

```python
import datoviz.raw as raw
```

`datoviz.raw` is for ABI validation, debugging, generator tests, advanced FFI work, and any call
site that needs exact `ctypes` behavior. The top-level facade is the normal Python entry point for
direct Datoviz engine use.

Generated Python documentation examples should target this top-level facade, not raw `ctypes`, when
they are derived from canonical C examples. See
[EXAMPLE_PYTHON_GENERATION.md](EXAMPLE_PYTHON_GENERATION.md).


## Rationale

Datoviz v0.4 is C-first, but that should not imply that Python users must manually write
`ctypes.cast()` boilerplate for common array uploads. Scientists already have NumPy arrays. They
should be able to pass those arrays directly to Datoviz functions whose C contracts are known.

The facade should improve argument adaptation, not rename or remodel the API. Keeping `dvz_*` names
has several benefits:

1. C and Python examples map almost mechanically.
2. Headers, generated references, search results, and AI-assisted translations use the same names.
3. The Python facade remains visibly a direct engine API, not a half-designed plotting API.
4. Names such as `scatter`, `imshow`, `Figure`, and Pythonic visual objects remain available for
   GSP/VisPy2 or a future explicitly designed high-level layer.


## Public Layering

```text
datoviz
  Recommended direct-engine Python API.
  Same dvz_* names as C.
  Accepts arrays for policy-declared pointer/count and pointer/byte-size argument groups.

datoviz.raw
  Exact generated ctypes binding.
  Requires explicit bytes, pointers, counts, and ctypes-compatible arguments.

GSP/VisPy2
  Future high-level plotting, Pythonic scene objects, notebook workflows, and scientific UX.
```

The public message should be:

> Datoviz can be used from Python today. The default Python package mirrors the C API and accepts
> NumPy arrays for common data arguments. It is not yet a high-level plotting package; that layer
> belongs above Datoviz in GSP/VisPy2.


## Example Shape

```python
import numpy as np
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 512, 512, 0)
panel = dvz.dvz_panel_full(figure)

points = dvz.dvz_point(scene, 0)

positions = np.array(
    [
        [-0.5, -0.4, 0.0],
        [+0.5, -0.4, 0.0],
        [0.0, +0.5, 0.0],
    ],
    dtype=np.float32,
)
colors = np.array(
    [
        [255, 80, 80, 255],
        [80, 220, 120, 255],
        [90, 150, 255, 255],
    ],
    dtype=np.uint8,
)
diameters = np.array([18.0, 18.0, 18.0], dtype=np.float32)

dvz.dvz_visual_set_data(points, "position", positions)
dvz.dvz_visual_set_data(points, "color", colors)
dvz.dvz_visual_set_data(points, "diameter", diameters)

dvz.dvz_panel_add_visual(panel, points, None)
```

The corresponding raw calls remain available:

```python
import ctypes
import datoviz.raw as raw

raw.dvz_visual_set_data(
    points,
    b"position",
    positions.ctypes.data_as(ctypes.c_void_p),
    positions.shape[0],
)
```


## Generation Strategy

Do not hand-write wrappers for the whole API. Generate the facade from the same extracted metadata
used by the raw binding, plus source-controlled binding policy for ambiguous pointer relationships:

```text
C public headers
        |
        v
build/bindings/datoviz_api.json
        |
        +--> datoviz/_ctypes.py         exact raw binding
        |
        +--> datoviz/_array_facade.py   array-aware facade
```

The generator should consume policy declarations from `spec/bindings/ctypes.yml` or a sibling
manifest. The policy should describe only relationships that cannot be inferred safely from C syntax:

1. pointer argument that accepts array-like data;
2. count argument inferred from `array.shape[0]`;
3. byte-size argument inferred from `array.nbytes`;
4. stride, shape, or dtype constraints when the C contract requires them;
5. string arguments that should accept Python `str` and encode to UTF-8 bytes;
6. ownership/lifetime rule, especially whether the callee copies data before returning.

Example policy shape:

```yaml
array_facade:
  dvz_visual_set_data:
    strings:
      - slot_name
    groups:
      - pointer_arg: data
        count_arg: item_count
        count_from: shape0
        lifetime: copied_before_return

  dvz_scene_buffer_set_data:
    groups:
      - pointer_arg: data
        size_arg: size_bytes
        size_from: nbytes
        lifetime: copied_before_return
```

The exact schema may change during implementation, but the policy must remain explicit. The
generator should not guess semantic pointer/count pairs blindly.


## Wrapper Behavior

For annotated functions, generated wrappers should:

1. accept NumPy arrays, memoryviews, and other compatible Python buffer objects where practical;
2. require or create C-contiguous arrays before passing a pointer;
3. infer item counts and byte sizes from the array object;
4. encode Python `str` values for configured `const char*` arguments;
5. keep converted temporary arrays alive until the raw call returns;
6. raise clear Python exceptions for unsupported dtype, shape, contiguity, or lifetime contracts;
7. return the raw function result without inventing ownership semantics.

For unannotated functions, the facade should expose the raw function directly or use a generated
trivial passthrough. This keeps the top-level namespace broad without pretending every pointer is
safe to adapt.


## Non-Goals

The array-aware facade must not provide:

1. prefixless aliases such as `scene()` or `visual_set_data()`;
2. Python scene, figure, panel, visual, or plot classes;
3. high-level constructors such as `scatter()`, `imshow()`, or `mesh()`;
4. automatic ownership or context-manager semantics for C handles;
5. implicit adaptation for unknown pointer arguments;
6. a compatibility layer for the v0.3 Python object model.


## Validation

The first implementation slice should include focused tests for:

1. importing `datoviz` and `datoviz.raw` with distinct documented roles;
2. preserving `dvz_*` names in the top-level facade;
3. Python `str` conversion for declared string arguments;
4. NumPy array conversion for pointer/count groups;
5. NumPy array conversion for pointer/byte-size groups;
6. non-contiguous input handling;
7. temporary lifetime during raw calls;
8. passthrough behavior for unannotated functions;
9. generated wrapper coverage reports from the policy manifest.

Suggested commands once implemented:

```sh
just ctypes
just ctypes-check
just ctypes-python-smoke
just ctypes-render-smoke
```


## Documentation Consequences

Public docs should stop presenting Python support as only raw `ctypes`. The accurate distinction is:

1. `datoviz` is the recommended direct-engine Python API and accepts NumPy arrays for declared data
   arguments;
2. `datoviz.raw` is the exact `ctypes` layer;
3. neither layer is a high-level plotting API;
4. high-level Python scientific visualization belongs in GSP/VisPy2.

Raw examples may remain for ABI and low-level integration proof. User-facing Python examples should
prefer the top-level array-aware facade once it exists.

Mechanically generated Python tabs for C examples should use the facade and policy described here;
the example-generation policy is recorded in
[EXAMPLE_PYTHON_GENERATION.md](EXAMPLE_PYTHON_GENERATION.md).
