# Scene Transform Pipeline

This document defines how coordinate transforms should work in the future scene layer.

It exists to make explicit a boundary that is already implied elsewhere in the scene spec:

1. data-space normalization is a scene concern,
2. panel navigation and camera transforms are panel concerns,
3. DRP2 should not need to understand scientific coordinate semantics.


## Purpose

The transform pipeline should:

1. keep scientific data coordinates visible at the scene level,
2. make normalization into visual-ready coordinates explicit,
3. separate data normalization from panel-local navigation,
4. support multiple panels viewing the same visual differently,
5. work for both 2D panzoom and 3D camera families.


## Core Rule

The scene layer should distinguish at least two transform stages:

1. data-to-visual normalization,
2. visual-to-panel viewing transform.

Those stages are not the same and should not be collapsed into a single vague “transform”.


## Why This Matters

In a scientific visualization application, the usual flow is:

1. the user defines data in a domain-specific coordinate system,
2. that data is normalized into a visual-ready coordinate space,
3. the normalized data is uploaded,
4. panel-local transforms such as panzoom or 3D camera movement are applied later.

This split is important because:

1. data normalization often changes rarely,
2. panel navigation may change every frame,
3. the same normalized visual data may be reused across many panels,
4. backend runtimes should not be asked to understand domain coordinate systems.


## Coordinate Spaces

The scene spec should recognize the following conceptual spaces.


### 1. Data Space

`DataSpace` is the user or domain coordinate system.

Examples:

1. measurement coordinates,
2. voxel coordinates,
3. geographic coordinates,
4. simulation coordinates,
5. abstract plot coordinates.

This is where the user’s data semantically lives.


### 2. Visual Space

`VisualSpace` is the visual-family-ready coordinate space derived from data space.

This is the coordinate space that a visual family expects for its primary renderable geometry or item
positions before panel-local navigation is applied.

In many current Datoviz-style cases, this will often be normalized around `[-1, 1]`, but the key
contract is semantic, not numeric:

1. it is visual-ready,
2. it is family-aware,
3. it is shared across panels unless a family explicitly needs panel-local derivation.


### 3. Panel Space

`PanelSpace` is the coordinate space after panel-local navigation or viewing transforms.

Examples:

1. 2D panzoom-adjusted coordinates,
2. 3D camera/view-adjusted coordinates,
3. panel-specific framing of the same shared visual data.

This is where one panel may differ from another even when they display the same visual.


### 4. Clip/NDC Space

`ClipSpace` or `NDC` is the final render-facing normalized device coordinate space.

This is not a scene-semantic space.
It is the final render-space result of panel-local view/projection logic.


## Two Main Stages


### Stage A: Data-To-Visual Normalization

This stage transforms data from `DataSpace` into `VisualSpace`.

This is typically where:

1. data ranges are normalized,
2. plot domains are mapped into visual-ready extents,
3. data-axis conventions are resolved,
4. visual-family-specific spatial preparation happens.

This stage is primarily a scene concern.

It should usually be:

1. CPU-side by default,
2. cached,
3. recomputed when data or normalization policy changes,
4. represented as scene resources rather than hidden shader behavior.


### Stage B: Visual-To-Panel Viewing Transform

This stage transforms `VisualSpace` into `PanelSpace` and finally into `ClipSpace`/`NDC`.

This is where:

1. 2D panzoom acts,
2. 3D camera/view/projection acts,
3. panel-local framing and navigation happen.

This stage is panel-local and typically dynamic.

It should usually be:

1. recomputed frequently,
2. independent from data normalization,
3. reusable across many visuals shown in the same panel.


## Ownership Boundary

The preferred ownership split is:

1. `Scene` owns data-space semantics and normalization policy,
2. `Visual` declares the transform inputs it needs,
3. `Resource` stores normalized or source data as needed,
4. `Panel` owns panzoom/camera state,
5. `FramePlan` consumes both normalized visual data and panel-local transform state.


## Caching Rules

The transform pipeline should support different invalidation behavior for different stages.


### Data Normalization Cache

Data-to-visual normalization should usually be recomputed when:

1. source data changes,
2. domain bounds change,
3. normalization policy changes,
4. a visual-family-specific interpretation changes.

It should usually not be recomputed when:

1. the user pans in 2D,
2. the user zooms in 2D,
3. the user moves a 3D camera,
4. another panel views the same visual differently.


### Panel Transform Cache

Panel-local transforms should usually be recomputed when:

1. panzoom changes,
2. camera state changes,
3. viewport size changes,
4. panel-local framing changes.

They should usually not force re-normalization of the underlying data resources.


## Relationship To Resources

The resource model should support this split directly.

Typical pattern:

1. source data lives in scene resources in `DataSpace`,
2. normalized visual-ready data lives in scene resources in `VisualSpace`,
3. panel-local transforms are separate panel-derived state or parameter resources,
4. final render-facing transforms are emitted during planning or draw contribution assembly.

This means that the scene resource system should be comfortable holding both:

1. source semantic data,
2. derived visual-ready data.


## Relationship To `FramePlan`

`FramePlan` is where the two transform stages come together for a given frame.

The expected order is:

1. source data or normalization policy changes are resolved first,
2. normalized visual-ready resources are updated if needed,
3. panel-local transforms are updated,
4. visual contributions are assembled using normalized resources plus current panel transforms,
5. DRP2 emission follows from that planned result.

`FramePlan` should not need to rediscover domain semantics.
It should consume already-decided normalized resources and current panel-local transform state.


## 2D Panels

For 2D panels, the normal model is:

1. data in `DataSpace`,
2. normalization into a visual-ready 2D `VisualSpace`,
3. panzoom maps that visual space into panel view,
4. final projection produces render-facing coordinates.

The key rule is:

1. changing pan or zoom should usually not require rebuilding normalized visual resources.


## 3D Panels

For 3D panels, the normal model is:

1. data in `DataSpace`,
2. normalization into a visual-ready 3D `VisualSpace`,
3. camera/view/projection maps that visual space into panel view,
4. final projection produces render-facing coordinates.

Again, the key rule is:

1. camera movement should usually not require rebuilding normalized visual resources.


## Family-Specific Notes


### `pixel`, `point`, `marker`, `segment`

These families usually want:

1. source positions in `DataSpace`,
2. normalization into visual-ready item positions,
3. panel-local viewing on top of those normalized positions.


### `path`

`path` typically needs:

1. grouped source coordinates in `DataSpace`,
2. grouped normalization into visual-ready path coordinates,
3. panel-local navigation applied later.


### `glyph`

`glyph` is slightly more complex because:

1. layout semantics may be family-specific,
2. some placement may be derived after grouping,
3. panel-local viewing still remains a separate stage.

Even here, the principle remains:

1. family layout and normalization come before panel navigation.


### `image`

`image` typically combines:

1. sampled field content,
2. image placement in visual-ready coordinates,
3. panel-local viewing.

For slice-like image modes backed by volumetric sampling:

1. the sampling source may be volumetric,
2. the resulting placed image still belongs to the `image` family,
3. the panel-local transform stage stays separate from the volumetric sampling semantics.


### `mesh`

`mesh` typically wants:

1. source geometry in data coordinates,
2. normalized visual-ready geometry,
3. panel-local camera transforms applied later.


### `sphere`

For `sphere`, impostor-first semantics fit this model well:

1. sphere centers and radii may originate in `DataSpace`,
2. they are normalized into visual-ready sphere data,
3. panel-local transforms are applied later,
4. variant choice between impostor and mesh-backed path should not change the high-level transform
   pipeline.


### `volume`

`volume` typically involves:

1. volumetric data in source domain coordinates,
2. a visual-ready volume framing in `VisualSpace`,
3. panel-local camera/view transforms on top of that framing.

The family may still have richer internal traversal logic, but that should not collapse the distinction
between normalization and panel-local viewing.


## CPU Versus GPU Boundary

The default scene-side preference should be:

1. perform domain normalization on the CPU when practical,
2. upload normalized visual-ready data as scene resources,
3. use GPU-side transforms primarily for panel-local viewing and family render logic.

Exceptions are allowed when:

1. the data volume is too large,
2. a family explicitly benefits from compute-assisted preparation,
3. capability-driven fallback forces a different path.

But the semantic boundary should remain the same even when implementation shifts.


## What DRP2 Should See

DRP2 should generally see:

1. normalized visual-ready resources,
2. panel-local transform state,
3. final render work implied by the visual family and current panel state.

DRP2 should not need to know:

1. the original scientific coordinate system,
2. the domain normalization policy,
3. why some visual-ready data ended up in `[-1, 1]` or another normalized range.


## Rules

1. Data normalization belongs above DRP2.
2. Panel-local navigation belongs below family semantics but above DRP2 emission.
3. The same normalized visual data should be reusable across multiple panels whenever practical.
4. Panzoom or camera changes should usually not invalidate normalized visual resources.
5. Source-data changes may invalidate normalized resources, but need not invalidate panel-local state.
6. Family semantics should describe transform needs without leaking backend matrix or handle types.


## Follow-On Spec Work

This document should eventually be connected to:

1. worked examples showing data-to-visual-to-panel flow for several families,
2. family-specific notes where transform behavior is especially distinctive,
3. future scene API sketches for declaring normalization policy explicitly.
