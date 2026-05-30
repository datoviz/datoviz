# Raw `ctypes`

Status: experimental low-level binding.

The raw `ctypes` layer exposes the v0.4 C engine to Python as directly as possible. It is useful for
low-level integration, smoke tests, backend work, and automation that needs exact access to
`libdatoviz`.

It is not the v0.3 Python plotting API, not a compatibility layer, and not the recommended
Pythonic scientific visualization surface. High-level plotting and object-oriented Python workflows
belong above Datoviz, currently in the GSP/VisPy2 layer.


## Import Surface

Use the explicit raw module:

```python
import datoviz.raw as dvz
```

The raw module preserves C names:

```python
scene = dvz.dvz_scene()
dvz.dvz_scene_destroy(scene)
```

Do not treat these as part of the raw-binding contract:

```python
import datoviz as dvz
scene = dvz.scene()
```

`datoviz.raw` is the public raw binding module. `datoviz._ctypes` is generated implementation
detail and should not be imported directly in examples or documentation.


## Generated Pipeline

The binding is generated from public C API metadata:

```text
public C headers -> build/bindings/datoviz_api.json -> datoviz/_ctypes.py -> datoviz.raw
```

`datoviz/_ctypes.py` is generated output. Do not edit it by hand. Regenerate it with:

```sh
just ctypes
```

The extraction and generation tools live under `tools/bindings/`. Binding policy lives under
`spec/bindings/`.


## API Reference Generation

This page defines the raw-binding scope, import style, ownership expectations, and validation
commands. It is not the exhaustive symbol catalog.

Exhaustive C and raw-binding symbol references should be generated from parsed public headers rather
than maintained by hand. The first source of truth for that generated outline is expected to be:

```text
build/bindings/datoviz_api.json
```

The generated reference should cover public C symbols, headers, structs, enums, constants, callback
typedefs, and raw-binding availability. Any skipped or opaque symbol should be explained from
source-controlled binding policy where possible.


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
