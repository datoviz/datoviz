> **Execution Status**
> - **Status:** `MOSTLY PROMOTED`
> - **Updated on:** `2026-05-16`
> - **Purpose:** preserve picking design rationale and remaining precision-policy notes after
>   promotion into interaction, visual-family, and API specs.

# Picking Design

This promoted note preserves the picking rationale and backlog that existed before the active
picking rules moved into specialized interaction, visual-family, frame-plan, and API specs.


## Authority Note

Active scene picking rules now live primarily in
[`../../interaction/PICKING.md`](../../interaction/PICKING.md). Selection integration belongs in
[`../../interaction/SELECTION.md`](../../interaction/SELECTION.md), family-specific hit identity belongs
in the relevant `../../visuals/*.md` files, and public result/request shape belongs in
[`../../api/API_SURFACE.md`](../../api/API_SURFACE.md) plus
[`../../api/API_DESIGN.md`](../../api/API_DESIGN.md).

This proposal remains a rationale and backlog note for precision levels, multi-hit policy, and
future family extensions. When the active pick path changes, update the specialized specs first.
The detailed sections below are historical design material unless the owning specialized specs
explicitly cite or absorb them.


## Historical Objective

Support precise scientific picking with a coherent scene-level model for:

1. object-level picking,
2. mesh face-level picking,
3. mesh instance-aware picking,
4. vertex/item-level picking for point/scatter/path-like visuals,
5. image pixel-level picking,
6. configurable hit-selection semantics across transparency and mixed overlays,
7. scene-owned result routing and selection integration.


## Existing Grounding In The Repo

There is already meaningful picking-related material in the repo:

1. frame-plan support for picking render nodes and readback in
   [include/datoviz/scene/frame_plan.h](../../../../include/datoviz/scene/frame_plan.h)
2. active frame-plan tests in
   [src/scene/tests/test_scene.c](../../../../src/scene/tests/test_scene.c)
3. broader scene-spec material such as:
   - [spec/scene/semantics/VISUAL_CONTRACT.md](../../semantics/VISUAL_CONTRACT.md)
   - [spec/scene/interaction/SELECTION.md](../../interaction/SELECTION.md)
   - [spec/scene/pipeline/RESOURCE_MODEL.md](../../pipeline/RESOURCE_MODEL.md)
4. visual-family hints, for example mesh face picking in
   [spec/scene/visuals/MESH.md](../../visuals/MESH.md)

The detailed sections below preserve the design rationale behind the promoted implementation
contract.


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
4. grouped primitive picking where the authored family carries both parent-group and sub-primitive
   identity,
5. vertex/item-level picking for point/scatter/path-like visuals,
6. pixel-level picking for image visuals.

Deferred:

1. mesh vertex-level picking unless a concrete need appears,
2. sub-character text picking,
3. richer barycentric or within-segment geometric detail beyond stable authored identity.

Why:

1. scientific picking often needs precise item identity,
2. point/scatter/path workflows require per-item precision now,
3. retained primitive workflows sometimes need both group identity and sub-primitive identity,
4. mesh face picking is the right first high-resolution mesh mode.


## Picking Modes By Family

Recommended initial family expectations:

1. `point`
   - item/vertex picking
2. future `scatter`-style point families
   - item/vertex picking
3. `line_strip`
   - strip-group identity
   - line-segment-within-strip identity
4. `lines`
   - line-item identity
5. `triangle`
   - triangle-item identity
6. `triangle_strip`
   - strip-group identity
   - triangle-within-strip identity
7. `path`
   - item/vertex picking at the path point identity level, not just object-level
8. `marker`
   - item-level picking
9. `segment`
   - item-level picking
10. broader `primitive` families
   - expose the most specific stable authored identity they retain
   - may report both a parent-group identity and a sub-primitive identity when both are meaningful
11. `mesh`
   - object-level picking always available
   - face-level picking available when explicitly enabled
12. `image`
   - pixel/data-cell-level picking
13. `text`
   - object/string-level first; glyph-level not required initially


## Identity Model

Picking needs stable scene-owned logical identities.

Recommended identity layers:

1. visual/object id
2. optional parent payload kind
3. optional parent payload-local id
4. payload kind
5. payload-local id

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
3. `line_strip`:
   - visual id
   - parent payload kind = strip_group
   - parent payload-local id = strip index
   - payload kind = line_segment
   - payload-local id = segment index within strip
4. `triangle_strip`:
   - visual id
   - parent payload kind = strip_group
   - parent payload-local id = strip index
   - payload kind = triangle_item
   - payload-local id = triangle index within strip
5. image:
   - visual id
   - payload kind = image_pixel
   - payload-local id = pixel or cell index

Do not expose raw backend resource ids or encoded attachment integers as the public result.


## Pick Result Shape

Recommended conceptual pick result:

1. request id
2. explicit status (`hit`, true miss, unsupported target, GPU execution failure, readback failure)
3. panel id
4. panel pixel position
5. visual id
6. optional instance id
7. optional parent payload kind
8. optional parent payload-local id
9. payload kind
10. payload-local id
11. optional depth
12. optional world/data or local hit position
13. optional image UV/texel coordinate
14. optional mesh barycentric coordinate
15. optional extra metadata later if needed

Payload kind should be explicit rather than inferred by the caller from which visual family was
clicked.

`hit = false` must not be the only failure channel. A false hit may mean a real empty-space miss,
an unsupported visual/target precision, or a GPU/runtime failure. These cases have different UI and
debugging meaning, so public results should expose a status enum.

Recommended initial statuses:

1. `HIT`
2. `MISS`
3. `OUTSIDE_PANEL`
4. `UNSUPPORTED_TARGET`
5. `NO_CAPABLE_VISUAL`
6. `GPU_EXEC_FAILED`
7. `READBACK_FAILED`
8. `STALE_DROPPED` if stale drops are surfaced publicly
9. `INVALID_RESULT`

Useful payload kinds to reserve now:

1. object
2. mesh_face
3. item_vertex
4. image_pixel
5. primitive_item
6. strip_group
7. line_segment
8. triangle_item

Visual-family target conventions for the consistency pass:

1. `pixel`, `point`, `marker`, and `sphere`: target `ITEM`;
2. `segment`: target `SEGMENT`;
3. `path`: parent target `STRIP` or subpath when available, target `SEGMENT` or `VERTEX`;
4. `image`: target `ITEM` for image rectangles, `PIXEL`/`SAMPLE` when texel identity is requested;
5. `mesh`: target `FACE` or `TRIANGLE`, with `instance_id` filled when instanced.


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


## Hit-Selection Policy

Picking semantics cannot always be “return the frontmost fragment”.

Important example:

1. a transparent shell surrounds opaque structures,
2. the click first intersects the shell,
3. the user may actually want the interior opaque object,
4. in other workflows the shell itself may be the intended target.

Recommended solution:

1. define explicit hit-selection policy at the scene or request level,
2. do not hard-code one universal rule into the picking path.

Recommended initial policies to reserve:

1. `frontmost`
2. `opaque_preferred`
3. `all_hits_sorted`

`opaque_preferred` is especially important for mixed transparent-shell and interior-mesh workflows.


## Multi-Hit Results

Some picking workflows need more than one candidate result.

Recommended direction:

1. simple workflows may ask for one resolved hit,
2. richer workflows may request a sorted hit list,
3. each multi-hit candidate should preserve both the raw hit identity and the post-policy resolved
   target,
4. interaction policy may choose the first resolved candidate or the first raw candidate depending
   on the workflow,
5. a future custom filter or reducer callback is a valid extension point later,
6. sorting should be scene-semantic enough to explain how transparency and overlay priorities were
   handled,
7. ties between otherwise equivalent candidates should still be broken deterministically so the
   result order remains stable.

The first implementation can still return one result by default, but the spec should leave room for
multi-hit queries now.


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

1. object-level picking should remain available by default,
2. face-level picking should be opt-in at the visual or interaction-policy level,
3. when face-level picking is enabled, the default resolved pick should be the face,
4. object-level mesh picking should resolve to `visual id + optional instance id`,
5. the returned identity includes instance id when the visual is instanced,
6. the returned face identity is triangle/face index, not vertex index,
7. mesh resource ordering must preserve stable face ordering for result interpretation,
8. future world/local hit position is optional and can be added later.

Do not start with object-only mesh picking.


## Picking Policy And Input Mapping

Picking semantics should not be hard-wired to one mouse/keyboard convention.

Recommended direction:

1. the API should let applications map input gestures and modifiers to picking semantics,
2. this mapping may choose object-level versus face-level mesh picking,
3. this mapping may choose probe-only versus persistent selection behavior for images and slices,
4. defaults should stay simple, but applications must be able to replace them.

Mesh-specific default:

1. face picking disabled by default,
2. object-level mesh picking available by default,
3. if the application enables face picking, the default resolved mesh pick mode becomes face-level
   unless the input mapping requests object-level picking for that gesture.


## Instanced Mesh Picking

Instanced mesh visuals need instance-aware picking from the beginning.

Recommended behavior:

1. one shared mesh resource may back many instances,
2. pick result must identify which instance was hit,
3. face picking is resolved within that instance,
4. scene-level logical identity may therefore include both instance id and face id,
5. in `all_hits_sorted` mode, overlapping instanced objects should appear as separate hit
   candidates rather than being collapsed by visual.

Typical result interpretation:

1. visual id
2. instance id
3. payload kind = mesh_face
4. face id

This is important for repeated objects such as many squares, cubes, or glyph-like mesh markers.


## Point / Scatter / Path / Primitive Picking

These families require stable authored identity precision.

Recommended behavior:

1. point/scatter/path-like visuals return item or vertex identity directly,
2. the picked payload id maps back to the original retained item ordering,
3. marker and segment-like repeated-item families should follow the same item-identity rule,
4. `lines` should return the specific line item,
5. `triangle` should return the specific triangle item,
6. `line_strip` and `triangle_strip` should return both the parent strip identity and the specific
   segment or triangle within that strip,
7. repeated primitive families should follow the same stable-authored-identity rule when authored
   as retained per-item data,
8. this should work with large retained datasets and incremental updates.

Recommended interaction implication:

1. grouped primitive families should preserve the exact sub-primitive raw hit,
2. interaction policy may still resolve that raw hit to the parent strip/group for hover or
   selection behavior,
3. this follows the general raw-hit versus resolved-target split used elsewhere in the interaction
   model.

Why this is important:

1. scientific visualization often relies on probing one exact sample,
2. object-level picking is insufficient for dense point and grouped primitive data.


## Image Picking

Image visuals need pixel-level picking.

Recommended behavior:

1. returned result identifies the image visual,
2. result includes pixel or cell coordinate/index,
3. future result may also include sampled value if the image semantic layer can provide it,
4. image picking should work even when the image is one panel layer among other overlays.

This is important for scientific rasters, heatmaps, atlas slices, and image-backed probes.


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
3. pick identity rendering remains its own path even when visible rendering uses WBOIT,
4. hit-selection policy must remain explicit when transparent and opaque objects overlap.

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

1. per-visual pickability or capability declaration,
2. panel-level or scene-level pick request helpers,
3. `DvzPickResult` or equivalent result object,
4. result polling/callback routing,
5. separate interaction-policy controls that decide how supported pick identities are resolved and
   consumed.

Useful conceptual calls:

1. `dvz_visual_set_pick_capabilities(...)`
2. `dvz_panel_pick(...)`
3. `dvz_scene_poll_pick_result(...)`
4. `dvz_interaction_set_pick_policy(...)`
5. hover/click callbacks later as convenience layers

The internal implementation may use panel-local picking targets and readback buffers, but the
public contract should stay logical and scene-oriented.


## Initial Implementation Target

The first implementation target should cover:

1. panel-local picking target and readback path,
2. scene-owned result resolution,
3. point item-level picking,
4. line-item and triangle-item primitive picking,
5. line-strip and triangle-strip parent-group plus sub-primitive picking,
6. mesh face-level picking,
7. image pixel-level picking,
8. controller-facing request/result path suitable for hover and click.

This is enough to support:

1. scientific point/scatter probing,
2. primitive-aware picking for retained lines, strips, and triangles,
3. face selection on 3D meshes,
4. pixel probing on images and slices,
5. later selection/highlight integration.


## Explicit Non-Goals For The First Picking Slice

1. mesh vertex-level picking,
2. glyph-level text picking,
3. reuse of transparency composite outputs as pick identity surfaces,
4. exposing raw encoded ids to user-facing APIs,
5. family-specific bespoke picking systems that bypass the scene-level model.
