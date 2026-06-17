# Python Raw `ctypes` Binding

Status: supported low-level Python binding.

The Python raw `ctypes` layer exposes the v0.4 C engine to Python as directly as possible. It is
useful for low-level integration, smoke tests, backend work, and automation that needs exact access
to `libdatoviz`.

It is not the v0.3 Python plotting API, not a compatibility layer, and not the recommended
direct-engine Python import. High-level plotting and object-oriented Python workflows belong above
Datoviz, currently in the GSP/VisPy2 layer.

| Use this page when | Use another page when |
| --- | --- |
| You need exact generated `ctypes` signatures, pointers, callbacks, or ABI behavior. | You want normal direct-engine Python calls with NumPy arrays. Use `import datoviz as dvz`. |
| You are debugging binding generation, package loading, or C/Python lifetime rules. | You want a high-level plotting API. Use the GSP/VisPy2 layer when available. |


## Import Surface

For normal direct-engine Python use, prefer the array-aware top-level facade:

```python
import datoviz as dvz
```

The facade preserves C names and accepts NumPy arrays for policy-declared data arguments:

```python
dvz.dvz_visual_set_data(points, "position", positions)
```

Use the explicit raw module when exact `ctypes` behavior is required:

```python
import datoviz.raw as raw
```

The raw module preserves C names:

```python
scene = raw.dvz_scene()
raw.dvz_scene_destroy(scene)
```

Do not treat these as part of the raw-binding contract:

```python
scene = dvz.scene()
```

`datoviz.raw` is the public raw binding module. `datoviz._ctypes` is generated implementation
detail and should not be imported directly in examples or documentation. `datoviz._array_facade` is
also generated implementation detail; use `import datoviz as dvz` instead.


## Generated Pipeline

The binding is generated from public C API metadata:

```text
public C headers -> build/bindings/datoviz_api.json
               -> datoviz/_ctypes.py -> datoviz.raw
               -> datoviz/_array_facade.py -> datoviz
```

`datoviz/_ctypes.py` and `datoviz/_array_facade.py` are generated output. Do not edit them by hand.
Regenerate them with:

```sh
just ctypes
```

The extraction and generation tools live under `tools/bindings/`. Binding policy lives under
`spec/bindings/`.


## API Reference Generation

This page defines the raw-binding scope, import style, ownership expectations, and validation. It is
not the exhaustive symbol catalog.

The exhaustive C, facade, and raw-binding symbol references are generated from parsed public headers
rather than maintained by hand. The generated outline starts from:

```text
build/bindings/datoviz_api.json
```

The generated reference covers public C symbols, headers, structs, enums, constants, callback
typedefs, and raw-binding availability. Skipped or opaque symbols should point back to
source-controlled binding policy where possible.


## Facade Versus Raw

The top-level facade and raw module intentionally share C-shaped names:

| Need | Import |
| --- | --- |
| Pass NumPy arrays to declared data uploads | `import datoviz as dvz` |
| Match exact `ctypes` signatures, pointers, and counts | `import datoviz.raw as raw` |
| Debug generated FFI implementation internals | `datoviz._ctypes`, rarely and not in docs examples |

The facade only adapts policy-declared pointer/count, pointer/byte-size, and string relationships.
Unannotated calls remain raw-shaped passthroughs.

For example, the facade call:

```python
dvz.dvz_visual_set_data(points, "position", positions)
```

maps to explicit raw pointer and count arguments:

```python
import ctypes
import numpy as np
import datoviz.raw as raw

positions = np.asarray(positions, dtype=np.float32, order="C")

raw.dvz_visual_set_data(
    points,
    b"position",
    positions.ctypes.data_as(ctypes.c_void_p),
    positions.shape[0],
)
```

Keep `positions` alive until the raw call has returned. If a function documents borrowed storage
rather than copied storage, keep the array alive for the documented borrowed lifetime.


## Naming And Types

The raw binding keeps exact C names:

| C surface | Raw Python surface |
| --- | --- |
| `dvz_*` functions | `dvz.dvz_*` functions |
| `Dvz*` structs and handles | generated `ctypes` classes or pointer-like handles |
| `DVZ_*` constants and enum values | generated constants or enum-compatible values |
| callback typedefs | generated `ctypes.CFUNCTYPE` types where supported |

The first raw layer is intentionally conservative. Opaque handles remain opaque, verified records
may get `ctypes.Structure` or `ctypes.Union` layouts, and layout-sensitive records may remain opaque
until the generator has an explicit alignment policy.


## Ownership And Lifetime

Follow the C API ownership rules. A raw Python handle does not turn a C object into a Python-owned
object with automatic semantic cleanup.

Use these rules unless a specific function documents a narrower contract:

1. destroy objects with the matching C destroy function;
2. treat returned borrowed pointers and strings as temporary;
3. copy borrowed payloads before the next mutating call when the C API says the payload is reused;
4. keep Python callback objects alive while they are registered with C;
5. do not rely on `datoviz._ctypes` internals for ownership or lifetime behavior.

Callback and host-helper behavior is still experimental. Thin Python helpers may exist for event
loop and callback ergonomics, but they do not replace the raw C-shaped API.


## Examples

Raw examples are intentionally small:

| Example | Purpose |
| --- | --- |
| `examples/python/raw/lifecycle.py` | import, timer call, scene create/destroy |
| `examples/python/raw/offscreen_point.py` | offscreen point render through raw handles |
| `examples/python/raw/async_click.py` | callback and host-helper smoke path |

These examples are low-level integration proof. They should not grow into Pythonic plotting
tutorials.


## Validation

Use the narrowest command that matches the change:

```sh
just ctypes
just ctypes-check
just ctypes-smoke
just ctypes-python-smoke
just ctypes-render-smoke
just ctypes-package-smoke
```

The full raw-binding validation path is:

```sh
just bindings
```

`ctypes-render-smoke` may skip graphics work when runtime support is not available.
