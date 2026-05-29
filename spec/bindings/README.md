# Raw ctypes Bindings

This document is the source of truth for the Datoviz v0.4 raw Python binding
architecture. The binding is a low-level FFI surface over `libdatoviz`, not a Pythonic
plotting API and not a compatibility layer for the v0.3 Python package.

High-level Python plotting, object-oriented convenience APIs, and scientific workflow sugar belong
above Datoviz, currently in the GSP/VisPy2 layer. Datoviz v0.4 owns the C engine and a generated raw
binding that honestly exposes that C engine to Python.

This boundary is also an AI-facing contract: coding agents should not infer a Python plotting API
from the existence of raw bindings. Generated Python examples must use `datoviz.raw` for low-level
FFI or route high-level plotting requests to GSP/VisPy2.


## Goals

1. Generate a pure Python `ctypes` binding from the v0.4 public C API.
2. Keep the raw binding close enough to C that C examples, headers, docs, and Python calls map
   directly to the same symbols.
3. Make the extracted C API metadata inspectable, diffable, and reusable by tests, docs, and future
   generators.
4. Keep hand-written binding policy separate from extracted C truth.
5. Validate struct layout, ownership-sensitive returns, and shared-library loading with focused
   automated checks.


## Non-Goals

1. Do not recreate the v0.3 object-oriented Python plotting API inside Datoviz.
2. Do not optimize the raw layer for short Python names at the cost of C traceability.
3. Do not bake NumPy convenience conversion into every generated pointer signature.
4. Do not make the generated Python file the only source of API metadata.


## Pipeline

The binding generation remains a two-stage process:

```text
C public headers + configured build definitions
        |
        v
build/bindings/datoviz_api.json
        |
        v
datoviz/_ctypes.py
```

The first stage extracts normalized C API metadata. The second stage consumes that metadata and
emits the Python `ctypes` module.

This separation is intentional:

1. C extraction and Python code generation are different problems.
2. `datoviz_api.json` can be inspected and reviewed during v0.4 API churn.
3. API extraction can be tested without importing Python bindings or loading `libdatoviz`.
4. The same metadata can later feed documentation, ABI reports, CFFI experiments, or other tooling.
5. Binding policy can be applied in the second pass without corrupting the extracted C model.


## Recommended Files

Use these locations for the v0.4 binding rewrite:

```text
tools/bindings/
  extract_api.py          # C/Clang -> normalized JSON
  generate_ctypes.py      # JSON -> datoviz/_ctypes.py
  generate_ctypes_abi.py  # C probe -> build/bindings/ctypes_abi.json
  validate_ctypes_abi.py  # ctypes sizeof/offset checks

spec/bindings/
  README.md               # this binding architecture and policy
  ctypes.yml              # machine-readable binding policy, later
  API_JSON_SCHEMA.md      # JSON schema notes, later

build/bindings/
  datoviz_api.json        # extracted C API metadata
  ctypes_abi.json         # generated C sizeof/alignof/offsetof facts
  ctypes_report.json      # optional validation report

datoviz/
  __init__.py             # documented package entry point
  raw.py                  # explicit public raw API module
  _ctypes.py              # generated implementation, do not edit by hand
```

The old v0.3-style names `parse_headers.py`, `build_ctypes.py`, and `headers.json` are not part of
the v0.4 design. The v0.4 names state the actual stage and artifact, and git history remains the
reference for the removed v0.3 generator.


## Just Commands

The binding workflow should be exposed through narrow commands:

```text
just api-json        # generate build/bindings/datoviz_api.json
just ctypes          # generate datoviz/_ctypes.py from JSON
just ctypes-abi      # generate build/bindings/ctypes_abi.json
just ctypes-check    # validate generated ctypes against C ABI facts
just ctypes-smoke    # import/load/create-destroy smoke
just ctypes-render-smoke    # offscreen render/capture smoke, skips without runtime support
just ctypes-package-smoke   # editable and wheel install smoke
just bindings        # run the full raw-binding workflow
```


## Extractor

The v0.4 extractor should use Clang AST metadata instead of extending the old pyparsing header
parser. The old parser proved the approach, but it only handles a subset of C and is too brittle for
the current public headers.

The extractor should parse the public Datoviz headers with the same include paths and feature
definitions used by the configured build. It should emit C facts, not Python binding decisions.

The extracted JSON should include at least:

1. exported functions and their exact C names;
2. concrete structs and unions, including fields, arrays, size, alignment, and offsets when
   available;
3. opaque structs and handle-like pointer targets;
4. enums and enum values;
5. typedefs, including callback typedefs;
6. constants intended for public binding use;
7. header path, feature guard, and documentation metadata where available.

Top-level public headers such as `datoviz/scene.h`, `datoviz/app.h`, and `datoviz/canvas.h` must be
included in the extraction scope. The old `include/datoviz/*/*.h`-only traversal is insufficient.


## JSON Policy

`build/bindings/datoviz_api.json` describes the C API. It must not pre-decide `ctypes` details such
as whether a pointer becomes `ctypes.c_void_p`, `ctypes.c_char_p`, `ctypes.POINTER(T)`, or a helper
wrapper.

Binding-specific policy belongs in `spec/bindings/ctypes.yml` or an equivalent source-controlled
manifest. That manifest can describe decisions that are not safely inferable from C syntax:

1. included and excluded headers;
2. excluded symbols;
3. ownership-sensitive return values;
4. functions used to release owned returns;
5. pointer arguments that should accept arrays;
6. byte-size, count, stride, and shape argument relationships;
7. callback lifetime rules;
8. feature-gated APIs that should or should not appear in a given build.

For example, a function returning an owned `char*` must not blindly use `ctypes.c_char_p` as its
`restype`, because that can lose the original pointer and make the matching destroy function unsafe.
The policy layer should mark the ownership rule and the emitter should choose a safe representation.


## Python Import Surface

The documented import style for the raw v0.4 binding is:

```python
import datoviz.raw as dvz

scene = dvz.dvz_scene()
dvz.dvz_scene_destroy(scene)
```

`datoviz/raw.py` is the explicit public raw module:

```python
from ._ctypes import *  # noqa
```

`datoviz/_ctypes.py` is generated and private. It should not be documented as the preferred import
path and should not be edited by hand.

The intended roles are:

1. `datoviz.raw`: stable explicit raw binding module;
2. `datoviz.__init__`: curated package entry point, not a blanket raw re-export;
3. `datoviz._ctypes`: generated implementation detail.

The top-level package may expose selected helper APIs for hosted Python integration, but it should
not promise that `import datoviz as dvz` is the raw binding. This prevents raw C symbols and Python
helper objects from competing for one namespace.


## Naming Policy

The raw binding preserves exact C symbol names. Do not remove the `dvz_` prefix in the generated raw
API.

This is canonical:

```python
dvz.dvz_scene()
dvz.dvz_scene_destroy(scene)
```

This is not part of the raw-binding contract:

```python
dvz.scene()
```

Keeping exact names is preferred because:

1. Python calls map directly to C headers and examples.
2. Search, debugging, stack traces, and docs all use the same symbol names.
3. Prefixless aliases create collision risk with structs, enum values, helper functions, and future
   package-level utilities.
4. Many C names only make sense with their namespace intact, such as `dvz_grid_panel`,
   `dvz_panel_axis`, and `dvz_axis_style`.

Prefixless aliases may be considered later as a separate convenience layer, but they are not the raw
binding and should not drive generation policy.


## Type Mapping Policy

The first generated raw layer should be conservative:

1. opaque `Dvz*` handles become empty `ctypes.Structure` classes passed by pointer;
2. concrete structs and unions become `ctypes.Structure` or `ctypes.Union` with verified layout;
3. enums become integer-compatible Python enum classes or integer constants, as decided by the
   emitter;
4. `void*` stays `ctypes.c_void_p` unless binding policy marks a safer special case;
5. `const char*` may use `ctypes.c_char_p` for borrowed input strings;
6. owned `char*` returns require explicit ownership policy and a matching destroy function;
7. callbacks require generated `ctypes.CFUNCTYPE` definitions and documented lifetime rules.

NumPy support should be added as thin helpers around the raw layer, not as the default treatment for
every pointer argument. The raw layer should remain faithful and predictable before becoming
ergonomic.


## Validation

Raw binding work is not complete until the generated binding is checked against the C build.

Required validation should include:

1. import and shared-library load smoke;
2. simple non-graphics function smoke, such as `dvz_time_monotonic_ns`;
3. create/destroy smoke for `DvzScene`;
4. narrow scene/app smoke when runtime support is available;
5. ABI layout checks comparing `ctypes.sizeof`, alignment, and field offsets with C-generated facts;
6. ownership smoke for owned returns such as scene JSON when those symbols are included.

The ABI facts should be generated from C into `build/bindings/ctypes_abi.json`, then compared with
the generated Python classes. Do not rely on blanket assumptions such as `_pack_ = 8`.

The current generator emits `_fields_` only for layout-safe records it can validate with `ctypes`.
Records that depend on cglm vector/matrix alignment are intentionally kept opaque until the raw
binding has an explicit aligned-structure policy.


## Examples

Keep raw Python examples small and close to the generated C surface:

1. `examples/python/raw/lifecycle.py` proves import, timer calls, and scene create/destroy.
2. `examples/python/raw/offscreen_point.py` builds a tiny point scene with raw `ctypes` arrays,
   renders offscreen when a runtime is available, and verifies PNG capture.

These examples are low-level integration proof, not a Pythonic plotting API.


## Migration From v0.3 Tooling

The removed v0.3 tooling demonstrated a useful architecture:

```text
tools/parse_headers.py -> build/headers.json -> tools/build_ctypes.py -> datoviz/_ctypes.py
```

For v0.4, keep the two-stage architecture but use the active `tools/bindings/` implementation:

1. replace pyparsing extraction with Clang-based extraction;
2. rename scripts to reflect their roles;
3. move binding scripts under `tools/bindings/`;
4. move generated JSON under `build/bindings/`;
5. keep source-controlled policy under `spec/bindings/`;
6. keep `datoviz/_ctypes.py` as generated output;
7. add `datoviz/raw.py` as the explicit public raw module;
8. make `import datoviz.raw as dvz` the documented raw import;
9. preserve exact `dvz_*`, `Dvz*`, and `DVZ_*` names in the raw surface.
