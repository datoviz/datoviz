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
6. axes and picking,
7. invalidation and redraw requests,
8. frame planning and DRP2 emission boundaries.


## Position

This sketch sits:

1. above `OBJECT_MODEL.md`,
2. above `VISUAL_CONTRACT.md`,
3. above `RESOURCE_MODEL.md`,
4. above `TRANSFORM_PIPELINE.md`,
5. above `FRAME_PLAN_IR.md`,
6. below any final public C API decision.

It is a design aid, not a mandate for names or signatures.


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
6. invalidation and redraw.


## Core Rule

The scene API should be declarative at the object level and explicit at the invalidation boundary.

In practice:

1. users create scene objects and attach logical data,
2. scene objects expose semantic properties and resources,
3. the scene layer derives normalized resources and per-frame plans,
4. the runtime only sees planned work, not high-level scene semantics.


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
6. `Controller`
7. `Animation`
8. `PickRequest`

This matches the existing scene object model while adding an explicit axis-facing object.


## Construction Model

The sketch should assume object creation by descriptors or constructors, not by low-level slot wiring.

The important semantic operations are:

1. create a `Scene`,
2. create one or more `Panel` objects,
3. create a `Visual` with a chosen family,
4. create or import `Resource` objects,
5. attach resources and parameters to visuals,
6. attach visuals and axes to panels,
7. mark objects dirty or request redraw,
8. ask the runtime to present or export.


## Scene

`Scene` should be the top-level owner of:

1. panels,
2. shared resources,
3. visuals,
4. controllers,
5. animations,
6. scene-global invalidation state.

Conceptually, the user should be able to do things like:

```text
scene = scene_create()
panel = scene_panel(scene, panel_desc)
visual = scene_visual(scene, visual_desc)
resource = scene_resource(scene, resource_desc)
```

The exact call spelling is open.


## Panels

`Panel` should expose:

1. layout or viewport placement,
2. panel dimensionality or camera family,
3. panel-local controller state,
4. panel-local axes and overlays,
5. target mode such as onscreen or offscreen.

Panels should not privately own their own unrelated copies of scene resources by default.

The important separation is:

1. scene resources may be shared,
2. panel transforms are panel-local,
3. panel-local derived resources are allowed when planning requires them.


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


## Picking

Picking should be panel-aware and visual-aware, but still scene-level in semantics.

The API needs to expose:

1. enabling picking on a visual or family instance,
2. issuing a pick request for a panel position,
3. receiving a result containing scene object identity,
4. distinguishing item identity from group identity when appropriate.

Conceptually:

```text
pick = panel_pick(panel, x, y)
result = scene_pick_result(scene, pick)
```

The final result shape should be able to report:

1. panel id,
2. visual id,
3. family id,
4. item id when present,
5. group id when present,
6. auxiliary payload if the family defines one.


## Invalidation Surface

The scene API should expose invalidation semantically rather than implicitly.

The important invalidation scopes are:

1. `VisualPropsDirty`
2. `ResourceDataDirty`
3. `PanelTransformDirty`
4. `AxisLayoutDirty`
5. `FramePlanDirty`

Not every API call needs a manual dirty flag, but the model should be explicit enough that the scene
layer can reason deterministically about what must be rebuilt.

Examples:

1. writing new point positions invalidates the normalized point resource and likely the plan,
2. panning a panel invalidates panel transforms but not the source point table,
3. panning far enough may also invalidate axis layout,
4. toggling picking may invalidate visual variant selection and the plan.


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


## Planning Boundary

The sketch should make clear that the scene API does not directly emit DRP2 from setters.

Instead:

1. setters mutate scene state,
2. invalidation is tracked,
3. frame build derives a `FramePlan`,
4. DRP2 emission happens from that plan.

This keeps the scene API declarative and testable.


## Error Surface

The final public API may use status codes, diagnostics, or callbacks.

At the spec level, the important requirement is that scene validation should be able to report:

1. missing mandatory resources,
2. incompatible family and variant choices,
3. malformed grouped data,
4. unsupported capability requests,
5. invalid transform or domain configurations.


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

The scene layer is then responsible for:

1. deriving normalized visual-space positions,
2. deriving panel-local transforms,
3. planning any picking participation,
4. constructing the `FramePlan`,
5. emitting DRP2 through the runtime-facing boundary.


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

The current spec pressure suggests the following preferences:

1. prefer family-aware creation over one giant untyped visual constructor,
2. prefer semantic resource roles over slot numbers,
3. prefer explicit grouped-resource concepts over anonymous flat buffers,
4. prefer panel-owned navigation over visual-owned navigation,
5. prefer explicit invalidation boundaries over hidden backend updates.


## Open Choices

The sketch leaves several API-shape choices intentionally open:

1. whether family creation uses one generic constructor or one constructor per family,
2. whether style data lives mostly in typed setters or typed style blocks,
3. whether grouped data is one object or an item table plus a grouping resource,
4. whether pick requests are synchronous, asynchronous, or both,
5. how much of validation is eager versus deferred to frame build.

These choices matter, but they do not change the main architecture.


## Recommended Next Step

The next spec iteration should define invalidation and caching explicitly.

This API sketch already depends on that missing document for:

1. resource update behavior,
2. axes regeneration thresholds,
3. redraw scheduling,
4. `FramePlan` rebuild policy,
5. cached normalization versus live panel transforms.
