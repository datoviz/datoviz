# Scene API Sketch

This document sketches a possible future scene-side API surface for Datoviz v0.4.

It is not a frozen public C header.

Its purpose is to pressure-test the existing scene specifications by showing how a user-facing scene
API could express:

1. scene and panel creation,
2. visual-family instantiation,
3. resource attachment,
4. grouped and sampled data,
5. parameter and variant selection,
6. axes, annotations, and explanatory objects,
7. picking, validation, and capability-aware behavior,
8. invalidation and redraw requests,
9. frame planning and DRP2 emission boundaries.


## Position

This sketch sits:

1. above `OBJECT_MODEL.md`,
2. above `VISUAL_CONTRACT.md`,
3. above `RESOURCE_MODEL.md`,
4. above `TRANSFORM_PIPELINE.md`,
5. above `FRAME_PLAN_IR.md`,
6. below any final public C API decision.

It is a design aid, not a mandate for names or signatures.


## Current Preferred Direction

Until a narrower public API proposal replaces this sketch, the preferred interpretation should be:

1. one scene-level `FramePlan` is built per frame,
2. panels contribute panel-local state, targets, and node groups inside that plan,
3. validation runs after invalidation resolution and before planning,
4. capability adaptation runs after validation and before planning,
5. family identity should follow the preferred v0.4 set in `VISUAL_FAMILIES.md`,
6. generic scene-level constructors with typed descriptors are preferred over a large family of
   unrelated creation entry points,
7. explicit style blocks and explicit mapping identity should be treated as the default sketch
   direction unless a later API pass replaces them.


## Reading Guide

This sketch is not the authoritative owner of every rule it touches.

When several scene documents overlap, read them with the following priority:

1. `VISUAL_FAMILIES.md` owns preferred family taxonomy,
2. `VISUAL_CONTRACT.md` and `VISUAL_MINI_CONTRACTS.md` own family-level contract details,
3. `RESOURCE_MODEL.md` owns logical resource classes and dirty-shape expectations,
4. `TRANSFORM_PIPELINE.md` owns normalization versus panel-transform boundaries,
5. `SCENE_VALIDATION.md` and `CAPABILITY_ADAPTATION.md` own pre-planning rejection and fallback
   rules,
6. `INVALIDATION_AND_CACHING.md` owns dirty-scope semantics,
7. `FRAME_PLAN_IR.md` owns the producer-side execution artifact,
8. `RUNTIME_BOUNDARY.md` owns the scene-to-runtime service contract.


## Core Goals

The scene API should make it easy to express scientific visuals without exposing:

1. backend object handles,
2. pipeline and descriptor concepts,
3. slot-based binding mechanics,
4. swapchain or windowing internals,
5. DRP2 command details.

At the same time, it should remain explicit about:

1. scene ownership,
2. panel-local versus scene-global state,
3. visual family identity,
4. resource schemas,
5. grouped data,
6. annotations and explanatory objects,
7. validation and capability adaptation,
8. invalidation and redraw.


## Core Rule

The scene API should be declarative at the object level and explicit at the invalidation boundary.

In practice:

1. users create scene objects and attach logical data,
2. scene objects expose semantic properties and resources,
3. the scene layer derives normalized resources and per-frame plans,
4. the runtime only sees planned work, not high-level scene semantics.

This also implies:

1. setters mutate scene-owned state rather than emitting DRP2 directly,
2. uploads and lazy materialization belong to planned frame work rather than an execution-time side
   path,
3. derived resources are transient by default unless the scene surface declares persistence
   explicitly.


## Non-Goals

This sketch does not define:

1. final naming,
2. final ownership semantics in C,
3. the exact callback model,
4. the exact threading model,
5. the final runtime/session entry points,
6. the final error-reporting surface.


## High-Level Object Surface

The minimum useful scene-side surface is:

1. `Scene`
2. `Panel`
3. `Visual`
4. `Resource`
5. `Axis`
6. `Annotation`
7. `Legend` / `Colorbar`
8. `Controller`
9. `Animation`
10. `PickRequest`
11. `ValidationReport`
12. `CapabilityPolicy`

This matches the existing scene object model while making room for the newer annotation, validation,
and adaptation surface.


## Construction Model

The sketch should assume object creation by descriptors or constructors, not by low-level slot wiring.

The important semantic operations are:

1. create a `Scene`,
2. create one or more `Panel` objects,
3. create a `Visual` with a chosen family,
4. create or import `Resource` objects,
5. attach resources and parameters to visuals,
6. create axes, annotations, legends, or colorbars,
7. attach visuals and explanatory objects to panels or scene layout,
8. validate or inspect scene state,
9. mark objects dirty or request redraw,
10. ask the runtime to present or export.


## Scene

`Scene` should be the top-level owner of:

1. panels,
2. shared resources,
3. visuals,
4. scene-global annotations,
5. scene-shared legends or colorbars when needed,
6. controllers,
7. animations,
8. scene-global invalidation state,
9. validation and capability-adaptation policy.

It should also be the natural owner of:

1. shared mapping or scale identity when explanatory objects are shared,
2. pending scene-level pick routing state,
3. capability snapshots or the active adapted policy state.

If the final public API introduces a separate runtime/session handle, that handle should remain below
the scene semantic layer and should follow `RUNTIME_BOUNDARY.md` rather than changing the ownership
model described here.

Conceptually, the user should be able to do things like:

```text
scene = scene_create()
panel = scene_panel(scene, panel_desc)
visual = scene_visual(scene, visual_desc)
resource = scene_resource(scene, resource_desc)
annotation = scene_annotation(scene, annotation_desc)
```

The exact call spelling is open.


## Panels

`Panel` should expose:

1. layout or viewport placement,
2. panel dimensionality or camera family,
3. panel-local controller state,
4. panel-local axes and overlays,
5. panel-local legends or annotations when appropriate,
6. target mode such as onscreen or offscreen.

Panels should not privately own their own unrelated copies of scene resources by default.

The important separation is:

1. scene resources may be shared,
2. panel transforms are panel-local,
3. panel-local derived resources are allowed when planning requires them,
4. panel-local explanatory objects may coexist with scene-shared ones.

Panels should contribute panel-local planning inputs, but they should not each own a separate
top-level `FramePlan`.


## Visual Creation

Visual creation should require at least:

1. a family id,
2. an optional initial variant selection,
3. an optional transform policy,
4. optional picking enablement,
5. optional initial parameter block.

Conceptually:

```text
visual = scene_visual(scene, {
    family = POINT,
    variant = default,
    picking = true,
})
```

The public surface should not force the user to think in terms of backend pipelines or descriptor
sets.


## Annotations And Explanatory Objects

The sketch should expose annotations by semantic role rather than by low-level drawing primitive.

The important object classes are:

1. labels,
2. guides,
3. probes,
4. crosshairs,
5. callouts,
6. legends,
7. colorbars.

Conceptually:

```text
annotation = panel_annotation(panel, {
    kind = CROSSHAIR,
    interaction = hover_linked,
})

legend = panel_legend(panel, legend_desc)
colorbar = scene_colorbar(scene, colorbar_desc)
```

The final API may choose:

1. one generic annotation object with typed descriptors, or
2. several specialized constructors.

But the semantic model should remain explicit either way.


## Visual Families

The API sketch should align with the current preferred first-class families:

1. `basic`
2. `pixel`
3. `point`
4. `marker`
5. `segment`
6. `path`
7. `glyph`
8. `image`
9. `mesh`
10. `sphere`
11. `volume`

And with current family decisions:

1. `basic` remains first-class,
2. `pixel` is simpler than `point`,
3. `sphere` is first-class and impostor-first,
4. `slice` is not a top-level family but an `image`-family mode backed by volumetric sampling,
5. `wiggle` is path-related rather than a required top-level family.


## Resource Attachment

The API should attach resources by semantic role, not backend slot number.

That means the user should say things like:

1. this visual uses this item table as its primary items,
2. this visual uses this grouped item table as its path data,
3. this visual uses this sampled field as its image payload,
4. this visual uses this style block for family parameters.

Conceptually:

```text
visual_set_resource(visual, ITEMS, points_table)
visual_set_resource(visual, STYLE, point_style)
visual_set_resource(visual, FIELD, image_field)
```

The final API may use function families or descriptor fields rather than a single generic call, but
the semantic binding model should remain role-based.


## Scene Resource Creation

Resource creation should expose scene-facing classes rather than only backend-shaped kinds.

The most important classes are:

1. `ItemTable`
2. `GroupedItemTable`
3. `IndexedGeometry`
4. `SampledField`
5. `StyleBlock`
6. `DerivedField`
7. `ReadbackTarget`

Conceptually:

```text
points = scene_item_table(scene, schema)
paths = scene_grouped_item_table(scene, schema)
field = scene_sampled_field(scene, schema)
style = scene_style_block(scene, schema)
```

The resource surface should also leave room for explicit persistence policy on derived resources.

Conceptually:

```text
derived = scene_derived_field(scene, {
    purpose = SLICE_INTERMEDIATE,
    persistence = TRANSIENT,
})
```

The preferred default is:

1. authored resources are authoritative,
2. derived resources are transient unless declared reusable,
3. compute-produced outputs do not silently become long-lived scene state.


## Flat And Grouped Data

The API sketch should preserve the distinction between:

1. flat independent items,
2. grouped logical objects stored in one batch-friendly resource.

This is especially important for:

1. many paths rendered together,
2. many labels rendered together,
3. grouped traces or wiggle-like path specializations.

The scene API therefore needs a direct way to express:

1. item payloads,
2. group boundaries,
3. optional group metadata,
4. item-level and group-level dirtiness.

The final API may choose between:

1. one grouped-table object,
2. one item-table object plus one grouping object,

but the semantics should remain explicit either way.


## Data Upload Surface

The API should make data updates explicit without leaking transport details.

Users should be able to express:

1. replace an entire resource,
2. update a subrange,
3. update one logical item,
4. update one logical group,
5. update a style block,
6. update a sampled field region when appropriate.

Conceptually:

```text
resource_write(points, range, data)
grouped_resource_write_group(paths, group_id, data)
style_write(style, params)
```

Those calls should mutate scene-owned CPU data and mark the correct invalidation scope.

They should not be read as “immediate backend upload” calls.

The intended model is:

1. the call updates scene-owned data,
2. invalidation is recorded,
3. the next frame build inserts the required upload or materialization nodes into `FramePlan`.


## Parameter Surface

Visual parameters should be exposed as scene properties or style-block writes, not backend uniforms.

The sketch should support:

1. family-wide parameters,
2. variant selectors,
3. transform-related visual properties,
4. picking enablement,
5. quality or fallback selectors.

The exact split between:

1. property-style setters,
2. structured style blocks,

is still open, but the scene semantics should remain clear.


## Variant Selection

Variants should be selected semantically.

Examples:

1. `image` color mode or slice-like mode,
2. `sphere` impostor-first versus mesh-backed mode,
3. `path` open or closed path behavior,
4. `mesh` lit or textured mode.

The API should avoid backend language such as:

1. pipeline ids,
2. shader module ids,
3. binding layouts.


## Mapping And Explanation Surface

The sketch should leave room for explicit scene-side mapping or scale objects where needed.

This matters for:

1. legends,
2. colorbars,
3. size scales,
4. categorical encodings,
5. domain-aware explanatory objects.

Conceptually:

```text
scale = scene_scale(scene, {
    kind = COLOR,
    domain = scalar_domain,
    palette = VIRIDIS,
})

visual_set_mapping(visual, COLOR_SCALE, scale)
colorbar_set_scale(colorbar, scale)
```

The final API may derive mappings from visual parameters instead of exposing first-class scale
objects, but the semantic relationship should still be representable.

Whether mappings are explicit objects or derived views, the API sketch should preserve stable mapping
identity.

That matters because:

1. a shared legend or colorbar should attach to semantic mapping identity rather than visual
   resemblance,
2. implicit aggregation is only valid when the mappings are semantically identical,
3. different meanings should not collapse into one explanatory object merely because they look
   similar.


## Transform Surface

The scene API should reflect the two-stage transform model already defined in
`TRANSFORM_PIPELINE.md`.

That means it should expose:

1. data-space configuration or domain mapping,
2. visual-local transforms when needed,
3. panel-local panzoom or camera transforms.

The important boundary is:

1. data-to-visual normalization belongs to scene-side resource preparation,
2. visual-to-panel movement belongs to panel-local live transforms.

So a visual should not force the user to pre-bake camera state into resource payloads.


## Panels, Cameras, And Controllers

The sketch should treat panel navigation as controller-owned state, not visual-local state.

Panels therefore need a surface for:

1. assigning a camera family,
2. attaching panzoom or 3D controllers,
3. reading or mutating visible domain,
4. requesting panel-local redraw.

Conceptually:

```text
panel_set_controller(panel, PANZOOM_2D)
panel_set_camera(panel, camera_desc)
panel_set_visible_domain(panel, domain)
```


## Axes

Axes should be explicit scene objects attached to panels.

The API should reflect the semantic model already defined in `AXES.md`:

1. axis semantics live in data space,
2. ticks and labels are derived scene content,
3. axis geometry is regenerated only when layout coverage or density thresholds require it,
4. live panel transforms still move the axis content every frame.

Conceptually:

```text
axis = panel_axis(panel, axis_desc)
axis_set_domain(axis, x_domain)
axis_set_scale(axis, LINEAR)
axis_set_formatter(axis, formatter)
```

The implementation may emit segment and glyph contributions internally, but the public scene-side
surface should not require the user to assemble those manually.


## Legends And Colorbars

Legends and colorbars should behave as annotation-side explanatory objects rather than as visual
families.

The API should allow:

1. panel-attached legends,
2. scene-shared consolidated legends,
3. panel-attached or axis-attached colorbars,
4. optional interactive legend behavior,
5. explicit attachment to one or more visual mappings.

Conceptually:

```text
legend = panel_legend(panel, { placement = RIGHT })
legend_add_visual(legend, visual)

colorbar = scene_colorbar(scene, { placement = SHARED_RIGHT })
colorbar_set_scale(colorbar, scale)
colorbar_attach_panels(colorbar, [panel_a, panel_b])
```

The scene-side API should not force the user to assemble the ramp, tick marks, and labels manually.

The important aggregation rule is:

1. a shared legend or colorbar should only aggregate semantically identical mappings by default,
2. if aggregation spans several visuals, the shared mapping identity should be explicit or
   unambiguous from scene state.


## Picking

Picking should be panel-aware and visual-aware, but still scene-level in semantics.

The API needs to expose:

1. enabling picking on a visual or family instance,
2. issuing a pick request for a panel position,
3. receiving a result containing scene object identity,
4. distinguishing item identity from group identity when appropriate.

Conceptually:

```text
pick = panel_pick(panel, {
    x = x,
    y = y,
    kind = HOVER,
})
result = scene_pick_result(scene, pick)
```

The sketch should assume that a pick request carries enough freshness information to support
asynchronous result handling safely.

Conceptually, the request should include:

1. stable request identity,
2. requesting panel identity,
3. request kind such as hover, click, or query,
4. scene or panel generation data sufficient to reject stale results.

The default hover rule should be:

1. latest request wins,
2. stale hover results may be discarded,
3. click and explicit query requests may choose stronger delivery guarantees.

The final result shape should be able to report:

1. panel id,
2. visual id,
3. family id,
4. item id when present,
5. group id when present,
6. auxiliary payload if the family defines one,
7. annotation or legend-entry identity when the picked object is explanatory rather than primary
   data.


## Validation And Capability Surface

The API sketch should make room for:

1. eager semantic validation,
2. pre-plan validation,
3. capability-aware simplification or rejection,
4. scene-visible diagnostics.

Conceptually:

```text
report = scene_validate(scene)
scene_set_capabilities(scene, runtime_caps)
scene_set_capability_policy(scene, policy)
scene_adapt(scene)
```

The final API may fuse some of these operations into frame build entry points, but the logical stages
should remain visible at the spec level.

The important rule is:

1. invalid scene semantics should fail as scene validation,
2. unsupported runtime paths should fail or simplify as capability adaptation,
3. neither should be deferred into backend-specific surprises.

The sketch should also leave room for explicit capability-dirty or adaptation-dirty consequences when
the active capability set changes.

The preferred frame-build semantics are:

1. invalidation is resolved first,
2. validation runs on the affected scope,
3. capability adaptation chooses an explicit outcome,
4. one scene-level `FramePlan` is then built from the validated and adapted scene state.

The normative behavior for those stages lives in:

1. `SCENE_VALIDATION.md`,
2. `CAPABILITY_ADAPTATION.md`,
3. `INVALIDATION_AND_CACHING.md`,
4. `FRAME_PLAN_IR.md`.


## Invalidation Surface

The scene API should expose invalidation semantically rather than implicitly.

The important invalidation scopes are:

1. `VisualPropsDirty`
2. `ResourceDataDirty`
3. `PanelTransformDirty`
4. `AxisLayoutDirty`
5. `AnnotationDirty`
6. `ExplanationLayoutDirty`
7. `CapabilityDirty`
8. `FramePlanDirty`

Not every API call needs a manual dirty flag, but the model should be explicit enough that the scene
layer can reason deterministically about what must be rebuilt.

Examples:

1. writing new point positions invalidates the normalized point resource and likely the plan,
2. panning a panel invalidates panel transforms but not the source point table,
3. panning far enough may also invalidate axis layout,
4. moving a linked crosshair may invalidate panel-local annotation layout only,
5. changing a color scale may invalidate a colorbar without rebuilding unrelated visuals,
6. toggling picking may invalidate visual variant selection and the plan,
7. changing active capabilities may invalidate fallback choice and possibly the plan.


## Redraw Requests

The scene API should distinguish:

1. state mutation,
2. invalidation,
3. frame scheduling.

That means the scene should support calls conceptually like:

```text
scene_request_redraw(scene)
panel_request_redraw(panel)
scene_step(scene, dt)
scene_build_frame(scene)
```

The final runtime may fuse some of these, but the separation is useful at the spec level.

`scene_build_frame(scene)` should be read as a compound scene operation that may perform validation,
capability adaptation, and plan construction before any runtime submission occurs.

If the final API separates build and submit more explicitly, this sketch should still preserve:

1. state mutation before build,
2. validation and adaptation before plan finalization,
3. runtime submission after the plan exists.


## Planning Boundary

The sketch should make clear that the scene API does not directly emit DRP2 from setters.

Instead:

1. setters mutate scene state,
2. invalidation is tracked,
3. frame build derives a `FramePlan`,
4. DRP2 emission happens from that plan.

This keeps the scene API declarative and testable.

It also means:

1. pick requests become planned picking work rather than direct backend calls,
2. upload work is derived into the plan rather than emitted by setters,
3. capability adaptation should be resolved before or during plan construction, not after backend
   failure.


## Error Surface

The final public API may use status codes, diagnostics, or callbacks.

At the spec level, the important requirement is that scene validation should be able to report:

1. missing mandatory resources,
2. incompatible family and variant choices,
3. malformed grouped data,
4. unsupported capability requests,
5. invalid transform or domain configurations,
6. invalid annotation or legend attachments,
7. capability-driven simplification or deactivation outcomes.


## Example: 2D Point Scatter

Conceptually, a user should be able to express:

```text
scene = scene_create()
panel = scene_panel(scene, { camera = PANZOOM_2D })

points = scene_item_table(scene, point_schema)
points_write(points, all_rows, xy_data)

style = scene_style_block(scene, point_style_schema)
style_write(style, { size = 4.0 })

visual = scene_visual(scene, { family = POINT, picking = true })
visual_set_resource(visual, ITEMS, points)
visual_set_resource(visual, STYLE, style)

panel_add_visual(panel, visual)
panel_add_default_axes(panel)
scene_request_redraw(scene)
```


## Example: Linked Panels With Shared Colorbar

Conceptually, a user should also be able to express a richer multi-panel case like:

```text
scene = scene_create()
panel_a = scene_panel(scene, { camera = PANZOOM_2D })
panel_b = scene_panel(scene, { camera = PANZOOM_2D })

field = scene_sampled_field(scene, scalar_field_schema)
field_write(field, full_extent, scalar_data)

scale = scene_scale(scene, {
    kind = COLOR,
    domain = data_domain,
    palette = VIRIDIS,
})

image = scene_visual(scene, { family = IMAGE, picking = true })
visual_set_resource(image, FIELD, field)
visual_set_mapping(image, COLOR_SCALE, scale)

panel_add_visual(panel_a, image)
panel_add_visual(panel_b, image)

scene_link_panels(scene, panel_a, panel_b, { mode = SHARED_PROBE })
panel_add_annotation(panel_a, { kind = CROSSHAIR, interaction = hover_linked })
panel_add_annotation(panel_b, { kind = CROSSHAIR, interaction = hover_linked })

colorbar = scene_colorbar(scene, { placement = SHARED_RIGHT })
colorbar_set_scale(colorbar, scale)
colorbar_attach_panels(colorbar, [panel_a, panel_b])

scene_request_redraw(scene)
```

This is intentionally still only a sketch.

The important pressure it adds is:

1. scene-shared resource ownership,
2. panel-local transforms,
3. scene-shared explanatory objects,
4. linked interaction,
5. capability-aware picking behavior.

The scene layer is then responsible for:

1. deriving normalized visual-space positions,
2. deriving panel-local transforms,
3. tracking one current hover request per panel and dropping stale hover results,
4. planning any picking participation,
5. constructing the `FramePlan`,
6. preserving the shared colorbar only while the mapping identity remains semantically identical
   across both panels,
7. emitting DRP2 through the runtime-facing boundary.


## Example: Grouped Paths

Conceptually:

```text
paths = scene_grouped_item_table(scene, path_schema)
paths_write_items(paths, item_rows, vertices)
paths_write_groups(paths, group_ranges, groups)

visual = scene_visual(scene, { family = PATH })
visual_set_resource(visual, GROUPED_ITEMS, paths)
panel_add_visual(panel, visual)
```

The important point is that the user expresses many logical paths while the scene remains free to
batch them into one efficient GPU-facing representation.


## Example: Image Slice Mode

Conceptually:

```text
field = scene_sampled_field(scene, volume_field_schema)
field_write(field, volume_data)

visual = scene_visual(scene, {
    family = IMAGE,
    variant = SLICE_MODE,
})
visual_set_resource(visual, FIELD, field)
visual_set_param(visual, slice_plane, plane_desc)
panel_add_visual(panel, visual)
```

This keeps `slice` under `image` while still exposing volumetric sampling semantics at the scene
level.


## API Shape Preferences

The current spec pressure suggests the following preferred defaults:

1. prefer family-aware creation over one giant untyped visual constructor,
2. prefer semantic resource roles over slot numbers,
3. prefer explicit grouped-resource concepts over anonymous flat buffers,
4. prefer panel-owned navigation over visual-owned navigation,
5. prefer explicit invalidation boundaries over hidden backend updates,
6. prefer explicit mapping identity over implicit explanation aggregation,
7. prefer transient derived resources by default unless persistence is declared explicitly.


## Deferred API Choices

The sketch still leaves several API-shape choices open, but the current preferred default is listed
first in each case:

1. use generic scene-level constructors with typed descriptors, though family-specific helpers may be
   layered on later,
2. use explicit style blocks by default, though typed property setters may wrap them,
3. use explicit grouped-resource concepts by default, whether that is one grouped object or a table
   plus grouping descriptor internally,
4. support asynchronous pick handling by default, while leaving room for synchronous helpers,
5. keep eager validation for local semantic checks and allow frame-build validation for plan-shaped
   checks,
6. expose mapping identity explicitly by default, while allowing some derived convenience surfaces.

These choices matter, but they do not change the main architecture.

The remaining choice surface should be read narrowly:

1. these are API-shape choices,
2. they are not intended to reopen the already-settled plan-scope, lifecycle-ordering, or
   runtime-boundary decisions established elsewhere in the scene spec.


## Recommended Next Step

The next spec iteration should refine this sketch into a smaller number of preferred construction
patterns.

The main remaining pressure points are:

1. how much the final public surface should use generic constructors versus family-specific helpers,
2. whether scale or mapping identity should usually be explicit or mostly derived,
3. how pick request and result objects should look in a C-friendly API,
4. where persistent derived caches deserve explicit scene-facing surface area.
