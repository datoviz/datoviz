# Scene Split Refactor Plan

> **Status:** active architecture plan
> **Created on:** 2026-05-27
> **Scope:** durable source-tree, public API, and test-structure direction for splitting reusable
> pieces out of the current `scene` subsystem.


## Purpose

`src/scene` is now a high-level retained visualization subsystem, not just another small utility
module. It owns figures, panels, visuals, retained resources, frame-plan emission, interaction
state, query state, and DRP2 integration. That scope is valid, but several parts of the current
scene layer are also useful without the retained scene graph.

The refactor goal is to make those reusable pieces scene-independent while keeping one aggregate
`datoviz` shared library. The source tree should express reusable lower layers through module
targets, public headers, and focused tests rather than by putting every helper under `scene`.

This document is the source-of-truth split plan. Slice-specific execution notes belong under
`agents/soon/` and completed implementation records belong under `agents/done/`.


## Non-Goals

1. Do not create multiple installed shared libraries in the first refactor. Keep `libdatoviz` as the
   installed aggregate.
2. Do not add broad directory buckets such as `src/runtime/` or `src/utils/` just to classify
   existing modules. Sibling top-level modules are acceptable when CMake and includes enforce the
   dependency graph.
3. Do not move scene-owned retained objects out of `scene` merely to shrink the file list.
4. Do not preserve v0.3 API compatibility when a cleaner v0.4 public boundary is available.
5. Do not turn VisPy2, GSP, or external UI needs into scene dependencies. Reusable primitives should
   remain useful without `DvzScene`.


## Layer Contract

The intended dependency direction is:

1. **Foundation:** `common`, `ds`, `math`, `thread`, `fileio`.
2. **Domain primitives:** scene-independent visualization/data primitives such as `geom`,
   `controller`, and future `color`, `field`, and `text` candidates.
3. **Runtime/backend:** `input`, `window`, `vk`, `vklite`, `canvas`, `stream`, `video`.
4. **Render protocol:** `drp2`.
5. **High-level systems:** `scene`, `app`, and optional GUI integration.

The current broad direction is also recorded in [MODULE_LAYERS.md](MODULE_LAYERS.md). This document
adds the concrete scene split plan.


## Promotion Test

A piece of code may move out of `scene` only when all of these are true:

1. it can be tested without constructing `DvzScene`, `DvzFigure`, or `DvzPanel`;
2. it does not include `_scene.h` and does not depend on retained scene object ownership;
3. it has a plausible non-scene consumer, such as VisPy2, GSP, app overlays, generated bindings,
   examples, or focused tests;
4. its public API can avoid scene handles, except for compatibility wrappers that remain under
   `datoviz/scene/*.h`;
5. scene can consume it as a client module;
6. its CMake target has explicit dependencies that do not create cycles.

If any of these fail, keep the code in `scene` and split only within `src/scene` by domain.


## What Must Remain Scene-Owned

These responsibilities should stay under `scene` unless a concrete non-scene producer appears:

1. `DvzScene`, `DvzFigure`, `DvzPanel`, `DvzVisual`, retained resources, and their registries.
2. Scene-owned `DvzController*` handles, panel binding, controller links, and invalidation.
3. Frame-plan construction, render-pass graph choices, render contracts, and DRP2 scene emission.
4. Visual-family retained APIs and their scene lifetime rules.
5. Scene query semantics, selection, hover, panel-local request queues, and readback/result
   ownership.
6. Axes, legends, colorbars, annotations, overlays, and layout reserve behavior when they depend on
   panels or retained visuals.
7. App-facing scene hosting and offscreen/window presentation behavior.


## Target Module Candidates

### `controller`

Status: first extraction slice.

Owns scene-independent camera and navigation state machines:

1. `DvzCamera`;
2. `DvzPanzoom`;
3. `DvzArcball`;
4. `DvzFly`;
5. `DvzTurntable`.

Boundary:

1. `controller` owns pure allocation, state mutation, event interpretation, matrix/camera updates,
   and destruction for individual controllers.
2. `scene` owns scene controller handles, panel attachment, controller links, invalidation, and
   app integration.
3. Public standalone users include `datoviz/controller.h` or `datoviz/controller/*.h`.
4. Existing `datoviz/scene/{camera,panzoom,arcball,fly,turntable}.h` headers remain wrappers for
   scene-owned constructors and panel camera APIs.

Required tests:

1. controller-only construction/state tests that do not link through scene ownership;
2. scene wrapper tests that verify panel binding, controller links, and invalidation still work.


### `color`

Status: candidate; not yet active.

Candidate contents:

1. colormap definitions and lookup;
2. categorical palettes;
3. scalar-to-color and label-to-color colorizers;
4. transfer-function tables that do not own scene resources;
5. sparse label color lookup helpers when they are plain CPU data transforms.

Keep in `scene`:

1. scale objects owned by a scene;
2. colorbar visuals, legend layout, and panel reserve integration;
3. retained GPU resource binding and upload scheduling;
4. cross-scene rejection and mutation-while-live checks.

Promotion trigger:

1. a standalone color API is needed by GSP, Python bindings, examples, or field conversion;
2. colorizer tests can run without `DvzScene`;
3. the public API can operate on plain descriptors, arrays, and output buffers.


### `field`

Status: candidate; not yet active.

Candidate contents:

1. sampled-field descriptors;
2. dimension, region, format, and sample-profile validation;
3. scalar/RGBA/label interpretation;
4. CPU-side subregion bounds checks;
5. value-domain and category-domain helper logic.

Keep in `scene`:

1. retained `DvzField` ownership and scene registries;
2. field-to-visual binding;
3. texture allocation, upload scheduling, partial-update dirty ranges, and live-stream guards;
4. cross-scene rejection.

Promotion trigger:

1. non-scene code needs to validate or interpret sampled arrays before handing them to scene;
2. field tests can run as a CPU-only module;
3. no public field primitive needs to mention panels, visuals, or DRP2 resources.


### `text`

Status: candidate; not yet active.

Candidate contents:

1. font defaults and font discovery policy that is not scene-owned;
2. glyph metrics and atlas CPU data structures;
3. text shaping and text block layout;
4. raster/MSDF atlas preparation that can be tested without scene visuals.

Keep in `scene`:

1. text and glyph retained visual families;
2. annotation and overlay layout tied to panels;
3. text atlas GPU resource ownership and upload scheduling;
4. scene invalidation from text changes.

Promotion trigger:

1. app overlays, docs/examples, or external UI need text layout without scene;
2. text tests can validate shaping/layout/glyph metrics without rendering a scene;
3. the public API can use explicit font/layout structs and output buffers.


### `geom`

Status: existing module; may absorb more scene-independent generators.

Candidate additions:

1. polygon tessellation and triangulation helpers;
2. arrows, tubes, ribbons, and vector geometry generation;
3. marker-shape CPU geometry;
4. orientation-gizmo axes or simple mesh generators;
5. contour/cut-surface extraction once CPU algorithms are introduced.

Keep in `scene`:

1. visual-family constructors and retained attribute uploads;
2. scene bounds propagation and panel attachment;
3. material/lighting state and render-contract decisions.

Promotion trigger:

1. a generator produces plain geometry arrays or `geom` containers;
2. it has no panel, visual, controller, or frame-plan dependency;
3. it is reusable by examples, importers, or external frontends.


## Internal Scene Split Directions

Some cleanup should remain internal to `src/scene` because the code is scene-specific but still too
large for one flat namespace.

Recommended internal grouping:

1. `core`: scene, figure, panel, layout, object ids, retained registries.
2. `visuals`: visual families, attributes, bounds, typed data, family metadata.
3. `fields`: retained fields, scales, colormaps as scene objects, colorbars, legends.
4. `interaction`: scene controller wrappers, links, selection, hover, query request ownership.
5. `pipeline`: frame plans, resources, render contracts, visual lowering, DRP2 emission.
6. `techniques`: transparency, MSAA, EDL, SSAO, gbuffer, material-model graph helpers.
7. `text`: scene-owned text visuals, annotations, overlay cards, text atlas runtime integration.
8. `tests`: focused scene test files by the same domain names.

This grouping may be implemented as subdirectories or as filename prefixes. Prefer subdirectories
only when CMake, includes, and test ownership become clearer; avoid churn that only changes paths.


## Public Header Strategy

For each promoted module:

1. add `include/datoviz/<module>.h` as the public aggregate;
2. add focused `include/datoviz/<module>/*.h` headers;
3. keep `include/datoviz/scene/*.h` wrappers only for scene-owned APIs or compatibility;
4. include the module aggregate from `include/datoviz/datoviz.h` when it is part of the default
   public surface;
5. avoid exposing scene-owned private structs from promoted headers;
6. prefer descriptor/state structs and explicit create/destroy functions for standalone ownership.

For scene-specific APIs, keep headers under `include/datoviz/scene/`.


## CMake Strategy

Each promoted module should have an object target named `datoviz_<module>`.

Rules:

1. add a root build option only when the module can be disabled meaningfully;
2. add a `DVZ_HAS_<MODULE>` compile definition;
3. make dependencies explicit through target links and include directories;
4. reject invalid configurations early with `message(FATAL_ERROR ...)`;
5. link promoted module targets into `datoviz`;
6. update examples and focused test binaries that link object targets explicitly.

Scene may depend on promoted modules, but promoted modules must not depend on scene.


## Test Strategy

Every extraction should add or preserve three validation lanes:

1. **Promoted module tests:** focused tests that do not construct scene objects.
2. **Scene wrapper tests:** existing scene tests that prove scene-owned constructors, panel binding,
   invalidation, and retained integration still work.
3. **Public header/API probe:** build coverage for the new aggregate and compatibility headers.

Minimum validation for extraction work:

```bash
git diff --check
just build
./build/testing/dvztest_<module>
direnv exec . just test scene
```

Use narrower scene filters while iterating, but run the broader scene suite before closing a
promotion slice.


## Staged Refactor Order

### Stage 1: Controller Primitive Extraction

Goal: make camera/controller primitives usable without `DvzScene`.

Required end state:

1. `src/controller` owns pure controller state machines;
2. `include/datoviz/controller.h` exposes the standalone API;
3. scene controller headers become wrappers;
4. controller tests run without scene;
5. scene tests still validate panel binding and controller links.


### Stage 2: Internal Scene Directory Cleanup

Goal: reduce the flat `src/scene` list without changing public behavior.

Preferred order:

1. group tests by domain if not already clear;
2. group internal headers by ownership;
3. move implementation files into domain subdirectories only when include paths stay readable;
4. preserve current comments and update them when paths or ownership change.

This stage should not introduce new public modules.


### Stage 3: Color Primitive Evaluation

Goal: decide whether `color` should become active before feature freeze.

Do this only if active work needs reusable color conversion outside scene. Otherwise leave color as
a future candidate and keep scene scale/colorbar behavior stable.


### Stage 4: Field Primitive Evaluation

Goal: decide whether sampled-field validation and interpretation should become scene-independent.

This is high value for Python/GSP/VisPy2 handoff, but it touches live retained resources in scene.
Split CPU descriptors and validation first; keep retained GPU ownership in scene.


### Stage 5: Text Primitive Evaluation

Goal: decide whether text shaping/layout should be reusable by app overlays or external UI.

Only promote pure CPU text primitives first. Keep atlas GPU resources and retained text visuals in
scene until there is a proven non-scene runtime consumer.


### Stage 6: Geometry Generator Additions

Goal: move pure shape/mesh/vector generation into `geom` as new visual families require it.

Prefer this when implementing vector, tube, contour, cut-plane, or gizmo geometry. Do not move
scene visual constructors to `geom`.


## Decision Record

| Area | Decision | Rationale |
| --- | --- | --- |
| Top-level hierarchy | Keep sibling modules such as `canvas`, `stream`, `geom`, `controller`, and `scene`. | Layering is enforced by targets and includes; adding `runtime/` and `utils/` buckets would create path churn without stronger boundaries. |
| First extraction | Promote controllers before color/field/text. | Controllers already have clear non-scene value and can be tested without retained scene objects. |
| Scene wrappers | Keep scene controller headers as wrappers. | Existing scene-owned APIs remain discoverable while new users get a scene-independent path. |
| `FramePlan` | Keep in `scene` for now. | It is tightly coupled to retained visual semantics and DRP2 scene emission. |
| Query/selection | Keep in `scene`. | Query routing, hover, selection, and result freshness are panel/scene semantics. |
| Visual families | Keep in `scene`. | They own retained object lifetime and frame-plan participation, even when they use lower-level primitives. |


## Open Questions

1. Should `color` become active before or after the v0.4 feature-freeze branch?
2. Should `field` expose only CPU validation helpers first, or also a public standalone sampled-field
   object?
3. Should internal scene subdirectories be introduced now, or should we wait until one more
   scene-independent promotion reduces the file count further?
4. Should controller public structs remain fully visible for v0.4, or should they move toward opaque
   handles plus state get/set functions for WASM/binding stability?
5. Should GSP/VisPy2 consume controller primitives directly, or through a smaller adapter API?


## References

1. [MODULE_LAYERS.md](MODULE_LAYERS.md)
2. [api/PYTHON_GSP_SCOPE.md](api/PYTHON_GSP_SCOPE.md)
3. [scene/interaction/CONTROLLERS.md](scene/interaction/CONTROLLERS.md)
4. [scene/interaction/CAMERA_CONTROLLERS.md](scene/interaction/CAMERA_CONTROLLERS.md)
5. [../agents/done/SCENE_CONTROLLER_PRIMITIVE_EXTRACTION.md](../agents/done/SCENE_CONTROLLER_PRIMITIVE_EXTRACTION.md)
