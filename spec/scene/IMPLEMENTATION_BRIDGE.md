# Scene Implementation Bridge

This document maps the current scene-spec decisions onto tentative implementation-facing C concepts.

It does not define the final public API or the final runtime ABI.


## Normative Status

This document is informative.

Its role is:

1. to pressure-test the current spec against implementation reality,
2. to propose one coherent translation path from the scene docs into C-facing types and entry points,
3. to surface naming or layering problems before headers are frozen.

The normative sources remain:

1. `PREFERRED_API_PROFILE.md`,
2. `RUNTIME_SERVICE_SKETCH.md`,
3. the specialized contract docs referenced by those two documents.


## Purpose

The implementation bridge should answer questions such as:

1. what scene-facing object handles probably exist,
2. what descriptor structs probably exist,
3. which operations likely belong on the scene side versus the runtime side,
4. where diagnostics and completion routing likely cross boundaries.

The draft headers under `headers/` are the concrete pressure-test output of this bridge.


## Scene-Side Object Mapping

The current spec most naturally maps onto implementation-facing scene objects such as:

1. `DvzScene`
2. `DvzScenePanel`
3. `DvzSceneVisual`
4. `DvzSceneResource`
5. `DvzSceneAxis`
6. `DvzSceneAnnotation`
7. `DvzSceneLegend`
8. `DvzSceneColorbar`
9. `DvzSceneController`
10. `DvzSceneScale`
11. `DvzSceneFramePlan`

The exact names remain open, but the split should stay semantic rather than backend-shaped.


## Descriptor Mapping

The preferred API profile suggests descriptor structs such as:

1. `DvzSceneCreateDesc`
2. `DvzScenePanelDesc`
3. `DvzSceneVisualDesc`
4. `DvzSceneResourceDesc`
5. `DvzSceneAxisDesc`
6. `DvzSceneAnnotationDesc`
7. `DvzSceneLegendDesc`
8. `DvzSceneColorbarDesc`
9. `DvzSceneScaleDesc`

The scene-facing constructor layer should therefore look more like:

```text
dvz_scene()
dvz_scene_panel(scene, &panel_desc)
dvz_scene_visual(scene, &visual_desc)
dvz_scene_resource(scene, &resource_desc)
```

than a family of backend-flavored creation calls.


## Resource Role Mapping

Semantic resource roles from the scene spec likely map onto an implementation enum such as:

1. `DVZ_SCENE_ROLE_ITEMS`
2. `DVZ_SCENE_ROLE_GROUPED_ITEMS`
3. `DVZ_SCENE_ROLE_STYLE`
4. `DVZ_SCENE_ROLE_FIELD`
5. `DVZ_SCENE_ROLE_GEOMETRY`
6. `DVZ_SCENE_ROLE_INDICES`
7. `DVZ_SCENE_ROLE_READBACK`
8. `DVZ_SCENE_ROLE_DERIVED`

This keeps the bridge aligned with `RESOURCE_MODEL.md` and `VISUAL_CONTRACT.md`.


## Family And Variant Mapping

Visual family identity likely maps onto:

1. a `DvzSceneVisualFamily` enum,
2. a variant flag set or structured variant descriptor,
3. an optional family-specific parameter schema layered on top of generic creation.

This suggests an implementation split such as:

1. `family` is mandatory in `DvzSceneVisualDesc`,
2. `variant` is explicit but family-scoped,
3. family-specific helpers remain wrappers rather than a different object model.


## Parameter And Style Mapping

The preferred profile suggests:

1. structured `StyleBlock`-like resources for most family parameters,
2. optional typed setters as wrappers,
3. explicit mapping objects when explanatory identity matters.

That likely becomes implementation-facing operations such as:

```text
dvz_scene_style_block(scene, &style_desc)
dvz_scene_style_write(style, data, size)
dvz_scene_visual_set_param(visual, key, value)
dvz_scene_visual_set_mapping(visual, mapping_kind, scale)
```


## Validation, Adaptation, And Build Mapping

The current spec most naturally maps onto scene-side functions such as:

1. `dvz_scene_validate(scene, &report)`
2. `dvz_scene_set_capabilities(scene, &caps)`
3. `dvz_scene_set_capability_policy(scene, &policy)`
4. `dvz_scene_adapt(scene, &report)`
5. `dvz_scene_build_frame(scene, &frame_plan)`

The implementation may later fuse these operationally, but the semantic stages should remain
visible in the layering.


## `FramePlan` Mapping

One scene-level `FramePlan` per frame most naturally implies:

1. one `DvzSceneFramePlan` handle or struct,
2. panel-local node groups inside that plan rather than separate top-level panel plans,
3. plan inspection utilities for tests or debug tools,
4. a translation layer from `DvzSceneFramePlan` into runtime-facing submission work.


## Runtime Mapping

The runtime sketch most naturally maps onto implementation-facing runtime concepts such as:

1. `DvzRuntimeService`
2. `DvzCapabilitySnapshot`
3. `DvzRuntimeSubmissionResult`
4. `DvzRuntimeCompletion`
5. `DvzRuntimeDiagnostic`

Conceptually:

```text
caps = dvz_runtime_query_capabilities(runtime)
result = dvz_runtime_submit(runtime, frame_plan)
completion = dvz_runtime_poll(runtime)
```


## Diagnostics Mapping

Diagnostics likely need a shared data shape across:

1. scene validation,
2. capability adaptation,
3. frame planning,
4. runtime execution.

The implementation bridge should therefore assume a shared diagnostic record and refer to
`DIAGNOSTICS_SCHEMA.md` rather than inventing separate ad hoc payloads at each layer.


## Likely Module Boundaries

The current spec suggests implementation modules such as:

1. scene object ownership and descriptors,
2. scene resource system,
3. transform and normalization,
4. validation and adaptation,
5. frame planning,
6. runtime service adapter,
7. diagnostics aggregation,
8. example or fixture builders.


## Language Binding Architecture

The C scene API is the canonical source of truth.
Python (and any other language) binds to it; the Python layer does not reimplement scene logic.

The architecture has three tiers:

```
┌─────────────────────────────────┐
│  Python sugar layer             │  datoviz/*.py        (pure Python)
│  ergonomics, NumPy, defaults    │
├─────────────────────────────────┤
│  Generated ctypes binding       │  datoviz/_ctypes.py  (auto-generated)
│  1:1 with C API, no compilation │
├─────────────────────────────────┤
│  C core                         │  libdatoviz.so
│  all scene logic lives here     │
└─────────────────────────────────┘
```

The rule is strict: all logic lives in C, all ergonomics live in Python, the binding layer is
mechanical and contains no logic of its own.

### v0.3 Binding Pipeline (Carried Forward)

v0.3 established a code-generation approach that v0.4 continues.
It requires no compiled extension module — the binding layer is a generated pure Python file that
loads `libdatoviz.so` at runtime via `ctypes.CDLL()`.

The pipeline has two steps:

**Step 1 — `tools/parse_headers.py`**

A pyparsing-based parser reads all `include/datoviz/**/*.h` headers and emits
`build/headers.json` containing defines, enums, struct field layouts, and all `DVZ_EXPORT`-marked
function signatures with their doxygen docstrings.
Only `DVZ_EXPORT` functions are included — this acts as the public API gate.

**Step 2 — `tools/build_ctypes.py`**

Reads `headers.json` and generates `datoviz/_ctypes.py`:

1. emits Python `IntEnum` classes for all C enums,
2. emits `ctypes.Structure` subclasses for all C structs with correct field types,
3. emits `argtypes` and `restype` for every exported function,
4. maps C scalar types to ctypes equivalents and pointer arguments to NumPy `ndpointer`,
5. converts doxygen `@param`/`@returns` docstrings to NumPy-style docstrings.

The output is never edited by hand.
In v0.3 it was ~15 000 lines covering the full public API.

Key properties of this approach:

1. no C++ compilation step — regenerating bindings after a header change is just running the
   two scripts,
2. the C headers are the single source of truth; no annotations or wrapper code are needed on
   the C side beyond `DVZ_EXPORT` and doxygen comments,
3. the generated file is readable as ordinary Python,
4. ctypes per-call overhead is negligible for scene-level calls where the hot path is GPU work.

### v0.4 Adaptation

The same pipeline applies in v0.4.
The generator scripts may need updates to handle v0.4 header conventions — new type names, new
struct patterns, changes to `DVZ_EXPORT` usage — but the architecture stays the same.
Any function that should be accessible from Python must be exported with `DVZ_EXPORT` and
documented with a doxygen docstring.

### C API Design As An FFI Target

For the generated binding to work correctly the C scene API must be designed as an FFI target.

Required properties:

1. **Opaque handles** — public headers never expose struct internals; callers hold pointers to
   forward-declared types only.
2. **Descriptor structs for construction** — constructors take one `const Desc*` argument rather
   than many positional parameters; this is both ctypes-friendly and forward-compatible.
3. **Explicit lifecycle** — every allocating call has a paired `dvz_foo_destroy`; no implicit
   ownership transfer.
4. **No raw function pointers in public structs** — they are awkward across ctypes; use explicit
   event registration functions instead.
5. **Error callbacks or last-error query** — a registered error callback or
   `dvz_scene_last_error()` pattern is more natural for ctypes consumers than per-call return
   codes.

### Python Sugar Layer

The sugar layer is pure Python, imports `_ctypes`, and adds:

1. keyword arguments and sensible defaults for constructors,
2. NumPy dtype coercion and shape checks before passing arrays to ctypes,
3. context managers for scene and resource lifecycle,
4. `__repr__` and inspection helpers,
5. inline colormap and scale shortcuts (as described in `SCALES.md`),
6. Pythonic property setters instead of explicit setter calls.

The sugar layer must not contain scene logic.
Any new semantic capability must be added to the C API first, then surfaced through the sugar
layer.

### Consequences For Spec Work

1. The draft C headers under `headers/` should be designed as FFI targets (opaque handles,
   descriptor structs) — the ctypes generator needs clean, parseable, `DVZ_EXPORT`-marked headers.
2. The inline scale shortcut in `SCALES.md` maps to a sugar-layer convenience; it does not require
   a special code path in the C API.


## Immediate Pressure On Implementation

If implementation work starts from this bridge, the first high-value prototypes would be:

1. `DvzScene`, `DvzScenePanel`, `DvzSceneVisual`, and `DvzSceneResource` descriptors,
2. `DvzCapabilitySnapshot` and runtime query plumbing,
3. one inspectable `DvzSceneFramePlan`,
4. one shared diagnostic record type used by validation, adaptation, planning, and runtime.
