# Scene Implementation Bridge

This document maps scene-spec decisions onto implementation-facing C concepts and documents the
Python binding architecture.

It does not define the final public API or the final runtime ABI.

The draft headers under `headers/` are the concrete pressure-test output of this bridge.


## Normative Status

This document is informative.

Its role is to pressure-test the spec against implementation reality and propose one coherent
translation path from the scene docs into C-facing types.

The normative sources remain `api/API_DESIGN.md`, `core/RUNTIME_BOUNDARY.md`, and the
specialized contract docs.


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

**Public API vs implementation names**: the `DvzScene*`-prefixed names above are tentative
implementation-internal names. The public API types use shorter handles without the `Scene`
infix: `DvzVisual*`, `DvzPanel*`, `DvzController*`, etc. (see `api/API_DESIGN.md` and
`../semantics/VISUAL_CONTRACT.md`). Implementation code may use `DvzScenePanel` internally; users and Python
bindings see only `DvzPanel*`.


## Descriptor Mapping

Preferred descriptor structs:

1. `DvzSceneCreateDesc`
2. `DvzScenePanelDesc`
3. `DvzSceneVisualDesc`
4. `DvzSceneResourceDesc`
5. `DvzSceneAxisDesc`
6. `DvzSceneAnnotationDesc`
7. `DvzSceneLegendDesc`
8. `DvzSceneColorbarDesc`
9. `DvzSceneScaleDesc`

The active public constructor layer uses direct scene/figure helpers, with descriptors reserved for
lower-level or future grouped creation:

```text
dvz_scene()
dvz_figure(scene, width, height, flags)
dvz_panel(figure, ...)
dvz_scene_visual(scene, &visual_desc)
dvz_scene_resource(scene, &resource_desc)
```


## Resource Role Mapping

Semantic resource roles map onto an implementation enum:

1. `DVZ_SCENE_ROLE_ITEMS`
2. `DVZ_SCENE_ROLE_GROUPED_ITEMS`
3. `DVZ_SCENE_ROLE_STYLE`
4. `DVZ_SCENE_ROLE_FIELD`
5. `DVZ_SCENE_ROLE_GEOMETRY`
6. `DVZ_SCENE_ROLE_INDICES`
7. `DVZ_SCENE_ROLE_READBACK`
8. `DVZ_SCENE_ROLE_DERIVED`

See `pipeline/RESOURCE_MODEL.md` and `../semantics/VISUAL_CONTRACT.md` for the normative resource role definitions.


## Family And Variant Mapping

Visual type identity maps onto a `DvzVisualType` enum: `DVZ_VISUAL_POINT`, `DVZ_VISUAL_PATH`, etc.

The spec uses "family" as a taxonomic concept; the public C API uses `DvzVisualType` and does not
expose the word "family" to users.

Type-specific creation helpers (`dvz_point()`, `dvz_path()`, etc.) are the primary public
surface; a generic `dvz_visual_create()` may exist internally.


## Family-Specific Data Preparation (Rasterization Backends)

The scene spec does not define how user data becomes GPU-ready. On a rasterization backend, most
visual types require CPU-side data preparation steps before GPU upload. These steps are invisible
to the user and are not part of the scene API contract.

| Visual type | Preparation needed |
|---|---|
| `point`, `pixel`, `marker` | none — positions uploaded directly |
| `path` | join and cap geometry computed from polyline vertices |
| `glyph` | glyph instances packed against an atlas; atlas may be rebuilt |
| `sphere` | impostor quad layout derived from center + radius |
| `mesh` | normals computed if absent; tangents for textured variants |
| `image` | none for 2D; slice plane parameters for slice mode |
| `volume` | transfer function and traversal parameters packed |
| `segment` | cap geometry derived from endpoint pairs |

These steps must be encapsulated inside each type's write path and triggered by invalidation.
Implementors targeting non-rasterization backends should define their own preparation steps.


## App, Canvas, And Runtime Wiring

The active application path owns the presentation objects and wires them to the scene:

```c
scene  = dvz_scene();
figure = dvz_figure(scene, width, height, 0);
app    = dvz_app(scene);
win    = dvz_view_offscreen(app, figure, width, height);
dvz_app_run(app, 0);
```

`DvzView` owns or borrows the concrete target: a GLFW/offscreen canvas for Datoviz-owned
presentation, or an externally supplied Vulkan surface for hosted presentation. It creates and
reuses the canvas/vklite/DRP2 runtime objects needed to execute frames for the figure.

The active execution flow is:

```text
scene -> FramePlan -> DvzDrp2CommandStream -> DvzDrp2Runtime -> vklite/canvas/app
```

The scene API does not take a `DvzCanvas*` argument directly. App-window helpers expose the canvas
for capture, sinks, and integration services after the app layer has established ownership. See
`core/RUNTIME_BOUNDARY.md` for the normative ownership rules.


## Validation, Adaptation, And Build Mapping

```text
dvz_scene_validate(scene, &report)
dvz_scene_set_capabilities(scene, &caps)
dvz_scene_adapt(scene, &report)
dvz_scene_build_frame(scene, &frame_plan)
```

The implementation may fuse these operationally, and the current app path does so inside
`dvz_view_render_once()` / `dvz_app_render_once()`: figure state is synchronized, a frame plan
is emitted to DRP2, and the runtime executes through the view's canvas target.


## Diagnostics

All four layers — validation, capability adaptation, frame planning, and runtime execution —
share a single diagnostic record type. See `validation/DIAGNOSTICS.md`.


## Likely Module Boundaries

1. scene object ownership and descriptors,
2. scene resource system,
3. transform and normalization,
4. validation and adaptation,
5. frame planning,
6. runtime service adapter,
7. diagnostics aggregation,
8. example or fixture builders.


## Language Binding Architecture

The C scene API is the canonical source of truth. Python (and any other language) binds to it;
the Python layer does not reimplement scene logic.

Three tiers:

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

All logic lives in C, all ergonomics live in Python, the binding layer is mechanical and contains
no logic of its own.


### Binding Pipeline

v0.3 established a code-generation approach that v0.4 continues. No compiled extension module is
needed — the binding layer is a generated pure Python file that loads `libdatoviz.so` at runtime
via `ctypes.CDLL()`.

**Step 1 — `tools/parse_headers.py`**: A pyparsing-based parser reads all
`include/datoviz/**/*.h` headers and emits `build/headers.json` containing defines, enums,
struct field layouts, and all `DVZ_EXPORT`-marked function signatures with their doxygen
docstrings. Only `DVZ_EXPORT` functions are included — this is the public API gate.

**Step 2 — `tools/build_ctypes.py`**: Reads `headers.json` and generates `datoviz/_ctypes.py`:
emits Python `IntEnum` classes for C enums, `ctypes.Structure` subclasses with correct field
types, `argtypes`/`restype` for every exported function, and converts doxygen docstrings to
NumPy-style. The output is never edited by hand (in v0.3 it was ~15 000 lines).

In v0.4 the generator scripts may need updates for new header conventions, but the architecture
stays the same.


### C API Design As An FFI Target

For the generated binding to work correctly:

1. **Opaque handles** — public headers never expose struct internals; callers hold forward-declared
   pointers only.
2. **Descriptor structs** — constructors take one `const Desc*` argument rather than many
   positional parameters; ctypes-friendly and forward-compatible.
3. **Explicit lifecycle** — every allocating call has a paired `dvz_foo_destroy`.
4. **No raw function pointers in public structs** — awkward across ctypes; use explicit event
   registration functions instead.
5. **Error callbacks or last-error query** — `dvz_scene_last_error()` is more natural for ctypes
   consumers than per-call return codes.


### Python Sugar Layer

Pure Python, imports `_ctypes`, adds:

1. keyword arguments and sensible defaults for constructors,
2. NumPy dtype coercion and shape checks before passing arrays to ctypes,
3. context managers for scene and resource lifecycle,
4. `__repr__` and inspection helpers,
5. inline colormap and scale shortcuts (see `semantics/SCALES.md`),
6. Pythonic property setters instead of explicit setter calls,
7. visual layer/group conveniences for z ordering, show/hide toggles, and batch styling.

The sugar layer must not contain scene logic. Any new semantic capability must be added to the C
API first, then surfaced through the sugar layer.

The inline scale shortcut in `semantics/SCALES.md` maps to a sugar-layer convenience; it does not require
a special code path in the C API.


## Implementation Plan

The first implementation sequence is recorded in
`agents/done/SCENE_DRP2_IMPLEMENTATION.md`; current follow-up work lives in
`agents/now/NEXT_STEPS.md`.

This document remains an implementation bridge for scene concepts and binding architecture. It
should not duplicate the module bring-up order, canvas/runtime integration order, or validation
matrix.
