> **Execution Status**
> - **Status:** `ACTIVE PICKING DESIGN NOTE`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 picking contract across visual families, identity
>   resolution, frame-plan/readback flow, and precision levels.

# Picking Design

This note records the intended v0.4 picking contract before mesh, text, and annotation work
hardens around inconsistent identity assumptions.


## Objective

Support precise scientific picking with a coherent scene-level model for:

1. object-level picking,
2. mesh face-level picking,
3. mesh instance-aware picking,
4. vertex/item-level picking for point/scatter/path-like visuals,
5. scene-owned result routing and selection integration.


## Existing Grounding In The Repo

There is already meaningful picking-related material in the repo:

1. frame-plan support for picking render nodes and readback in
   [include/datoviz/scene/frame_plan.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene/frame_plan.h)
2. active frame-plan tests in
   [src/scene/tests/test_scene.c](/home/cyrille/GIT/Viz/datoviz/src/scene/tests/test_scene.c)
3. broader scene-spec material such as:
   - [spec/scene/VISUAL_CONTRACT.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/VISUAL_CONTRACT.md)
   - [spec/scene/interaction/SELECTION.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/interaction/SELECTION.md)
   - [spec/scene/pipeline/RESOURCE_MODEL.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/pipeline/RESOURCE_MODEL.md)
4. visual-family hints, for example mesh face picking in
   [spec/scene/visuals/MESH.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/visuals/MESH.md)

This note narrows that larger material into the active implementation contract.


## Core Recommendation

Picking should be a scene-level identity system, not just a render trick.

The backend may render ids into targets and read them back, but the public contract should be:

1. visuals declare what precision of picking they support,
2. scene owns stable logical identities,
3. readback results are mapped back to scene-visible ids and payload types,
4. controllers and selection operate on scene identities, not raw backend ids.


## Required Picking Precision By Visual Family

The first active design should not force one picking granularity on every visual.

Recommended precision requirements:

1. object-level picking for all pickable visuals,
2. mesh face-level picking for mesh visuals,
3. mesh instance-aware picking for instanced mesh visuals,
4. vertex/item-level picking for point/scatter/path-like visuals.

Deferred:

1. mesh vertex-level picking unless a concrete need appears,
2. sub-character text picking,
3. exact edge/segment sub-primitive picking beyond what active workflows require.

Why:

1. scientific picking often needs precise item identity,
2. point/scatter/path workflows require per-item precision now,
3. mesh face picking is the right first high-resolution mesh mode.


## Picking Modes By Family

Recommended initial family expectations:

1. `point`
   - item/vertex picking
2. future `scatter`-style point families
   - item/vertex picking
3. `path`
   - item/vertex picking at the path point identity level, not just object-level
4. `primitive`
   - object-level by default; item-level only when semantics are defined clearly
5. `mesh`
   - face-level picking
6. `image`
   - object-level unless later a pixel/data-cell model is explicitly introduced
7. `text`
   - object/string-level first; glyph-level not required initially


## Identity Model

Picking needs stable scene-owned logical identities.

Recommended identity layers:

1. visual/object id
2. payload kind
3. payload-local id

Examples:

1. mesh:
   - visual id
   - optional instance id
   - payload kind = face
   - payload-local id = triangle index
2. point/scatter/path:
   - visual id
   - payload kind = item/vertex
   - payload-local id = item index

Do not expose raw backend resource ids or encoded attachment integers as the public result.


## Pick Result Shape

Recommended conceptual pick result:

1. panel id
2. visual id
3. optional instance id
4. payload kind
5. payload-local id
6. optional world or local hit position later
7. optional extra metadata later if needed

Payload kind should be explicit rather than inferred by the caller from which visual family was
clicked.

Useful payload kinds to reserve now:

1. object
2. mesh_face
3. item_vertex


## Scene-Owned Resolution Tables

The scene should own the mapping from readback values to logical pick results.

Recommended architecture:

1. render-time ids written to the picking target are compact backend-facing encodings,
2. scene maintains lookup/resolution tables that map those values back to:
   - visual id
   - payload kind
   - payload-local id
3. controllers and selections consume the resolved logical result.

This keeps picking stable even if runtime-side encoding changes.


## Frame-Plan Contract

The picking path should remain explicit in the frame plan.

Recommended flow:

1. panel-local picking render node,
2. copy to pick readback buffer,
3. readback request routing,
4. scene-side interpretation of the result.

The active frame-plan tests already point in this direction and should remain the source of truth
for sequencing expectations.

Recommendation:

1. keep picking visible in frame plans as a distinct render/readback path,
2. do not hide picking as a side effect of visible color rendering.


## Readback Model

Picking should be request/readback based.

Recommended contract:

1. scene or controller issues a pick request at panel pixel coordinates,
2. next suitable frame includes or reuses the picking path,
3. readback result is interpreted by the scene,
4. stale results are discarded using request identity or generation matching.

This should support both:

1. click/query picking,
2. hover picking.


## Synchronous Versus Asynchronous Use

Different interaction patterns need different behavior.

Recommended model:

1. hover picking:
   - asynchronous
   - latest-request-wins semantics
2. click/query picking:
   - may present a synchronous-facing helper at the app API level later,
   - but should still be implemented on top of the same explicit request/readback path rather than
     inventing a separate hidden pipeline.

The key point is architectural consistency, even if helper APIs later look synchronous.


## High-DPI and Coordinate Semantics

Picking should operate in logical panel/window coordinates at the scene boundary.

Recommendation:

1. input routers/controllers produce logical pixel coordinates,
2. runtime performs any required mapping to the actual picking target resolution,
3. scene/public APIs stay expressed in logical coordinates.

This keeps picking behavior aligned with the rest of the scene interaction model.


## Mesh Picking

Mesh face picking is required now.

Recommended mesh behavior:

1. the returned identity includes instance id when the visual is instanced,
2. the returned face identity is triangle/face index, not vertex index,
3. mesh resource ordering must preserve stable face ordering for result interpretation,
4. future world/local hit position is optional and can be added later.

Do not start with object-only mesh picking.


## Instanced Mesh Picking

Instanced mesh visuals need instance-aware picking from the beginning.

Recommended behavior:

1. one shared mesh resource may back many instances,
2. pick result must identify which instance was hit,
3. face picking is resolved within that instance,
4. scene-level logical identity may therefore include both instance id and face id.

Typical result interpretation:

1. visual id
2. instance id
3. payload kind = mesh_face
4. face id

This is important for repeated objects such as many squares, cubes, or glyph-like mesh markers.


## Point / Scatter / Path Picking

These families require item-level precision.

Recommended behavior:

1. point/scatter/path-like visuals return item or vertex identity directly,
2. the picked payload id maps back to the original retained item ordering,
3. this should work with large retained datasets and incremental updates.

Why this is important:

1. scientific visualization often relies on probing one exact sample,
2. object-level picking is insufficient for dense point and path data.


## Text and Annotation Picking

Text should be pickable at a coarser level initially.

Recommended first behavior:

1. text visual returns object/string identity,
2. annotation callouts return annotation object identity,
3. glyph-level or sub-character picking is deferred.

This is enough for labels and callouts without overcomplicating the first text implementation.


## Picking and Transparency

Picking must remain coherent with the transparency architecture.

Recommendation:

1. picking should not reuse transparent color-composite outputs as identity surfaces,
2. transparent visuals should still participate in picking according to their declared pick model,
3. pick identity rendering remains its own path even when visible rendering uses WBOIT.

This keeps identity semantics independent from transparency math.


## Picking and Selection

Picking results should feed scene-owned selection and hover state, not bypass them.

Recommended flow:

1. pick request resolves to logical identity,
2. controller/selection layer applies interaction policy,
3. selection/highlight state mutates at scene level,
4. visuals reflect that state through their styling/highlight paths.

Do not let individual visuals privately interpret pick ids without scene-level identity routing.


## Capability and Failure Model

Picking should be capability-aware.

Recommended behavior:

1. visuals declare whether they are pickable and at what granularity,
2. scene validation should reject or diagnose pickable visuals when the required picking path is
   unavailable,
3. unsupported high-resolution picking modes should be explicit diagnostics, not silent behavior
   changes.

Examples:

1. mesh face picking requested but no valid picking target/readback path exists
2. point visual declares item picking but lacks stable item identity


## Public API Direction

The exact names can still move, but the public scene-facing model should include:

1. per-visual pickability declaration,
2. panel-level or scene-level pick request helpers,
3. `DvzPickResult` or equivalent result object,
4. result polling/callback routing.

Useful conceptual calls:

1. `dvz_visual_set_pick_mode(...)`
2. `dvz_panel_pick(...)`
3. `dvz_scene_poll_pick_result(...)`
4. hover/click callbacks later as convenience layers

The internal implementation may use panel-local picking targets and readback buffers, but the
public contract should stay logical and scene-oriented.


## Initial Implementation Target

The first implementation target should cover:

1. panel-local picking target and readback path,
2. scene-owned result resolution,
3. point item-level picking,
4. mesh face-level picking,
5. controller-facing request/result path suitable for hover and click.

This is enough to support:

1. scientific point/scatter probing,
2. face selection on 3D meshes,
3. later selection/highlight integration.


## Explicit Non-Goals For The First Picking Slice

1. mesh vertex-level picking,
2. glyph-level text picking,
3. reuse of transparency composite outputs as pick identity surfaces,
4. exposing raw encoded ids to user-facing APIs,
5. family-specific bespoke picking systems that bypass the scene-level model.
