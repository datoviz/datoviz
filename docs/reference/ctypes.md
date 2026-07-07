# Python Binding Exact Call Form

Status: supported exact call form of the generated Python binding.

Datoviz has one generated `ctypes` binding. The normal import is `import datoviz as dvz`, which
keeps the C `dvz_*` names and adds policy-declared NumPy adaptation for selected array arguments.
`datoviz.raw` is the exact call form of the same binding: use it when you need to pass explicit
bytes, pointers, counts, byte sizes, callbacks, or other C-shaped arguments yourself.

It is not the v0.3 Python plotting API, not a compatibility layer, and not the recommended Python
import for ordinary scene code. High-level plotting and object-oriented Python workflows belong
above Datoviz, currently in the GSP/VisPy2 layer.

| Use this page when | Use another page when |
| --- | --- |
| You need to spell out pointers, counts, callbacks, or ABI-sensitive arguments yourself. | You want ordinary Python scene calls with NumPy arrays. Use `import datoviz as dvz`. |
| You are debugging binding generation, package loading, or C/Python lifetime rules. | You want a high-level plotting API. Use the GSP/VisPy2 layer when available. |


## Import Surface

For ordinary Python scene code, prefer the main `datoviz` package:

```python
import datoviz as dvz
```

The main package uses the same `dvz_*` function names as the C examples and accepts NumPy arrays for
supported visual-data uploads:

```python
dvz.dvz_visual_set_data(points, "position", positions)
```

Use the explicit exact-call module when exact `ctypes` arguments are required:

```python
import datoviz.raw as raw
```

The exact-call module preserves C names:

```python
scene = raw.dvz_scene()
raw.dvz_scene_destroy(scene)
```

Do not treat these as part of the Python binding contract:

```python
scene = dvz.scene()
```

`datoviz.raw` is the public exact-call module. `datoviz._ctypes` is generated implementation
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

The generated exact-call module tracks the exported C ABI: `DVZ_EXPORT` declarations only. Installed
headers may contain declarations that are not emitted by `libdatoviz`; those are not binding entry
points unless they become exported ABI.


## API Reference Generation

This page defines exact-call scope, import style, ownership expectations, and validation. It is
not the exhaustive symbol catalog.

The exhaustive C and Python binding symbol references are generated from parsed public headers rather
than maintained by hand. The generated outline starts from:

```text
build/bindings/datoviz_api.json
```

The generated reference covers public C symbols, headers, structs, enums, constants, callback
typedefs, and binding availability. Skipped or opaque symbols should point back to source-controlled
binding policy where possible.


## Main Package Versus Raw

The main `datoviz` package and `datoviz.raw` intentionally share the same `dvz_*` function names:

| Need | Import |
| --- | --- |
| Pass NumPy arrays to supported data uploads or capture RGBA arrays | `import datoviz as dvz`; see [Python binding with NumPy arrays](python-direct-engine.md) |
| Spell out exact `ctypes` pointers, counts, bytes, and callbacks | `import datoviz.raw as raw` |
| Debug generated FFI implementation internals | `datoviz._ctypes`, rarely and not in docs examples |

The main package adapts only the pointer/count, pointer/byte-size, and string relationships listed
in the binding policy. Unannotated calls may still require exact C-shaped arguments.

For example, this main-package call:

```python
dvz.dvz_visual_set_data(points, "position", positions)
```

maps to explicit pointer and count arguments:

```python
import ctypes
import numpy as np
import datoviz.raw as raw

positions = np.array(
    [[-0.5, -0.5, 0.0], [0.5, -0.5, 0.0], [0.0, 0.5, 0.0]], dtype=np.float32
)

raw.dvz_visual_set_data(
    points,
    b"position",
    positions.ctypes.data_as(ctypes.c_void_p),
    positions.shape[0],
)
```

Keep `positions` alive until the `datoviz.raw` call has returned. If a function documents borrowed storage
rather than copied storage, keep the array alive for the documented borrowed lifetime.


## Naming And Types

The exact call form keeps C names:

| C surface | Exact Python surface |
| --- | --- |
| `dvz_*` functions | `raw.dvz_*` functions |
| `Dvz*` structs and handles | generated `ctypes` classes or pointer-like handles |
| `DVZ_*` constants and enum values | generated constants or enum-compatible values |
| callback typedefs | generated `ctypes.CFUNCTYPE` types where supported |

The exact call form is intentionally conservative. Opaque handles remain opaque, verified records
may get `ctypes.Structure` or `ctypes.Union` layouts, and layout-sensitive records may remain opaque
until the generator has an explicit alignment policy.


## Ownership And Lifetime

Follow the C API ownership rules. A Python binding handle does not turn a C object into a
Python-owned object with automatic semantic cleanup.

Use these rules unless a specific function documents a narrower contract:

1. destroy objects with the matching C destroy function;
2. treat returned borrowed pointers and strings as temporary;
3. copy borrowed payloads before the next mutating call when the C API says the payload is reused;
4. keep Python callback objects alive while they are registered with C;
5. do not rely on `datoviz._ctypes` internals for ownership or lifetime behavior.

Owned `char*` returns are represented as `ctypes.c_void_p`, not `ctypes.c_char_p`, so the original
pointer is preserved. Some generated destroy signatures currently accept `ctypes.c_char_p`, so cast
the pointer at the destroy call:

```python
ptr = raw.dvz_scene_json(scene)
try:
    text = ctypes.string_at(ptr).decode("utf8") if ptr else None
finally:
    if ptr:
        raw.dvz_scene_json_destroy(ctypes.cast(ptr, ctypes.c_char_p))
```

Callback and host-helper behavior is still experimental. Thin Python helpers may exist for event
loop and callback ergonomics, but they do not replace the exact-call API.


## Examples

Raw examples are intentionally small:

| Example | Purpose |
| --- | --- |
| `examples/python/raw/lifecycle.py` | import, timer call, scene create/destroy |
| `examples/python/raw/offscreen_point.py` | offscreen point render through exact-call handles |
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

The full binding validation path is:

```sh
just bindings
```

`ctypes-render-smoke` may skip graphics work when runtime support is not available.
