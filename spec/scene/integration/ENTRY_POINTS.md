# Integration entry points for advanced users (v0.4-dev)

This document defines practical low-level and hybrid integration lanes for users who do not want the
full scene-system overhead, while still leveraging Datoviz runtime building blocks.

It complements the v0.4 modular architecture and keeps one shared library target (`datoviz`) with
feature-gated module activation.


## Why this exists

Datoviz v0.4 currently exposes multiple public layers:

- low-level Vulkan primitives (`vk`)
- Vulkan convenience layer (`vklite`)
- canvas/window/stream runtime
- DRP2 protocol stream + runtime
- scene authoring and frame-plan emission
- app presentation layer

Advanced users typically need one of several entry points, not always the full stack.


## Integration tiers

## Tier A — Canvas runtime with custom renderer

**Target users:** people who want robust window/swapchain/frame-loop services and input routing, but
own all rendering logic.

**Use modules:** `window`, `canvas`, `stream` (plus `vk`/`vklite` as needed by the renderer).

**Primary API anchors:**

- `dvz_canvas_default_config()`
- `dvz_canvas_create()` / `dvz_canvas_destroy()`
- `dvz_canvas_set_draw_callback()`
- `dvz_canvas_frame()`
- `dvz_canvas_submit()`

**Notes:** this tier keeps presentation and frame orchestration in Datoviz while rendering payload and
resource policy remain user-defined.


## Tier B — vklite direct

**Target users:** Vulkan users who want less boilerplate but full explicit control.

**Use modules:** `vk` + `vklite`.

**Primary API anchors:**

- `include/datoviz/vk.h`
- `include/datoviz/vklite.h`
- vklite subheaders (`buffers`, `images`, `descriptors`, `commands`, `sync`, `graphics`, `compute`,
  `rendering`, `swapchain`, `surface`, `slots`, `sampler`, `shader`)

**Notes:** ideal when scene abstractions are unnecessary but the team still wants Datoviz Vulkan
utilities and conventions.


## Tier C — DRP2 direct

**Target users:** users who prefer protocol-level rendering streams and runtime execution without scene
authoring.

**Use modules:** `drp2` (+ runtime dependencies in `vklite`/`canvas` as needed).

**Primary API anchors:**

- `include/datoviz/drp2.h`
- DRP2 sublayers: `stream`, `recording`, `runtime`

**Notes:** this is the clean lane for command-stream generation from external tooling or custom engines.


## Tier D — Scene authoring + export for external renderer

**Target users:** users who want Datoviz scene creation ergonomics, but render with their own backend.

**Use modules:** `scene` as authoring/IR source.

**Primary API anchors:**

- `dvz_scene_json()` / `dvz_scene_json_destroy()`
- `dvz_figure_emit()` / `dvz_figure_emit_ex()`

**Notes:** scene state can be serialized or converted to DRP2 stream outputs and consumed by external
renderers/adapters.


## Tier E — Scene + DRP2 emission + Datoviz runtime

**Target users:** users who want the complete Datoviz high-level workflow.

**Use modules:** `scene` + `drp2` + runtime (`vklite`, `canvas`) optionally wrapped via `app`.

**Primary API anchors:**

- Scene lifecycle and figure/panel/visual APIs
- `dvz_figure_emit[_ex]()` and request processing helpers
- `dvz_app*` window/presentation entry points

**Notes:** this is the end-to-end maintained vertical slice in v0.4.


## Packaging and build profiles

Keep one shared library (`datoviz`) and provide profile-style build presets through feature options:

- `DVZ_BUILD_CORE`
- `DVZ_BUILD_VK`
- `DVZ_BUILD_CANVAS`
- `DVZ_BUILD_DRP2`
- `DVZ_BUILD_WEBGPU` (currently OFF by default)
- `DVZ_BUILD_SCENE`
- `DVZ_BUILD_APP`
- `DVZ_BUILD_GUI`

Recommended profile naming for distribution/documentation:

1. **core**: core-only (`common`, `ds`, `fileio`, `math`, `thread`)
2. **canvas**: core + Vulkan + canvas stack
3. **drp2**: canvas profile + DRP2
4. **scene**: drp2 profile + scene
5. **app**: scene profile + app (+ optional GUI)

These are documentation/packaging personalities over the same modular build graph.


## API documentation strategy

Document API by **authoring vs execution boundaries** rather than one monolithic list:

- **Authoring lane:** `scene`/figure/panel/visual APIs
- **Execution lane:** `canvas`, `vk`, `vklite`, `drp2 runtime`, `app`

For each lane, include:

- minimal bootstrap example
- ownership/borrowing rules
- lifecycle constraints (especially live stream/resource mutation constraints)
- required module toggles and platform caveats


## Integration guidance

1. Prefer targeted headers for advanced integration (for example `canvas.h`, `drp2.h`, `vklite.h`)
   rather than always including `datoviz.h`.
2. Treat scene emission as an explicit handoff boundary when integrating non-Datoviz renderers.
3. For hosted UI toolkits and externally-owned surfaces, prefer app external-surface entry points so
   ownership contracts stay explicit.
4. Keep runtime object ownership clear (owned vs borrowed Vulkan and stream resources) across module
   boundaries.


## Validation guidance per tier

Use the narrowest test loop that matches the touched subsystem:

- Canvas lane: canvas/window/stream focused tests
- vklite lane: vk/vklite focused tests
- DRP2 lane: DRP2 fixture + runtime tests
- Scene lane: scene focused tests + integration checks
- Full lane: unified `dvztest` run

Maintain the unified runner as end-to-end coverage, but keep per-tier smoke loops for iteration speed.
