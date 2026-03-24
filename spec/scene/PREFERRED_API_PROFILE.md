# Preferred Scene API Profile

This document selects the current preferred scene-facing API defaults for Datoviz v0.4.

It is derived from `SCENE_API_SKETCH.md`, but it is narrower and more decision-oriented.


## Position

This document sits:

1. above `OBJECT_MODEL.md`,
2. above `VISUAL_CONTRACT.md` and `RESOURCE_MODEL.md`,
3. below any final public C header decision,
4. alongside `SCENE_API_SKETCH.md` as the preferred current interpretation of the sketch.


## Normative Status

This document is normative for the current preferred API direction.

It should be read as:

1. selecting the default API-shape choices that `SCENE_API_SKETCH.md` intentionally left open,
2. constraining implementation-oriented exploration to one coherent profile,
3. leaving only a small set of explicitly deferred questions open.

Rationale paragraphs and examples in this document are informative.


## Core Rule

The preferred scene API profile should expose semantic scene objects and semantic resource roles,
while keeping planning, validation, adaptation, and runtime submission explicit in the model.


## Preferred Construction Model

The preferred construction model is:

1. generic scene-level constructors with typed descriptors,
2. family identity supplied explicitly at visual creation,
3. convenience helpers allowed later only as thin wrappers over the generic model.

Conceptually:

```text
scene = scene_create(scene_desc)
panel = scene_panel(scene, panel_desc)
visual = scene_visual(scene, visual_desc)
resource = scene_resource(scene, resource_desc)
```

The profile should not assume one completely different constructor family per visual type as the
primary surface.


## Preferred Ownership Model

The preferred ownership model is:

1. `Scene` owns visuals, resources, shared mappings, controllers, and scene-global invalidation
   state,
2. `Panel` owns panel-local view state, axes, and panel-local explanatory attachments,
3. one scene-level `FramePlan` is built per frame,
4. the runtime remains below the scene semantic layer.


## Preferred Visual And Resource Binding Model

The preferred binding model is:

1. visuals bind resources by semantic role,
2. resource roles come from scene contract vocabulary, not slot numbers,
3. grouped resources remain explicit scene concepts.

Conceptually:

```text
visual_set_resource(visual, ITEMS, points)
visual_set_resource(visual, STYLE, style)
visual_set_resource(visual, FIELD, field)
```

This profile prefers one grouped-resource concept in the public model, even if implementation later
splits storage and grouping metadata internally.


## Preferred Parameter Model

The preferred parameter model is:

1. explicit `StyleBlock`-like structured parameter resources by default,
2. typed property setters allowed as convenience wrappers,
3. variant selection kept semantically explicit.

Conceptually:

```text
style = scene_style_block(scene, style_desc)
style_write(style, style_data)
visual_set_resource(visual, STYLE, style)
visual_set_param(visual, name, value)
```


## Preferred Mapping And Explanation Model

The preferred explanation model is:

1. explicit mapping identity by default,
2. legends and colorbars attach to that mapping identity,
3. implicit aggregation is allowed only when the mapping identity is semantically identical.

Conceptually:

```text
scale = scene_scale(scene, scale_desc)
visual_set_mapping(visual, COLOR_SCALE, scale)
colorbar_set_scale(colorbar, scale)
```

Derived convenience surfaces are allowed later, but the preferred baseline should preserve explicit
mapping identity in the model.


## Preferred Validation And Adaptation Surface

The preferred validation and adaptation surface is:

1. explicit validation entry points remain representable,
2. explicit capability snapshot and policy entry points remain representable,
3. frame build may fuse them operationally, but not semantically.

Conceptually:

```text
report = scene_validate(scene)
scene_set_capabilities(scene, runtime_caps)
scene_set_capability_policy(scene, policy)
scene_adapt(scene)
```

The required ordering remains:

1. invalidation resolution,
2. validation,
3. capability adaptation,
4. scene-level `FramePlan` construction.


## Preferred Picking Model

The preferred picking model is:

1. asynchronous-capable request and completion semantics by default,
2. freshness-preserving request identity,
3. scene-visible result identity with panel, visual, item, group, or explanatory-object identity as
   needed.

Synchronous helper calls may be added later, but they should not become the defining semantic model.


## Preferred Build And Submission Surface

The preferred build and submission model is:

1. scene mutation is separate from redraw requests,
2. redraw requests are separate from frame build,
3. frame build is separate from runtime submission,
4. runtime submission stays below the scene API boundary.

Conceptually:

```text
scene_request_redraw(scene)
scene_build_frame(scene)
runtime_submit(runtime, frame_plan)
```

The public API may wrap some of these in convenience entry points, but this separation should remain
visible in the architecture.


## Preferred Error And Diagnostics Surface

The preferred diagnostics model is:

1. scene validation and adaptation produce scene-visible diagnostics,
2. runtime failures are mapped back to scene-visible plan, target, or resource identity,
3. backend handles never become required to interpret failures.


## Deferred Questions

The following questions remain intentionally open:

1. the exact spelling of constructors and descriptor structs in a C header,
2. whether some style parameters deserve dedicated typed setters in addition to style blocks,
3. whether mapping objects are always public handles or may sometimes be hidden behind derived
   convenience constructors,
4. how much of `FramePlan` inspection is public versus test-only.


## Relationship To Other Scene Docs

This document should be read together with:

1. `SCENE_API_SKETCH.md` for the broader design space,
2. `VISUAL_CONTRACT.md` and `VISUAL_MINI_CONTRACTS.md` for family contract details,
3. `RESOURCE_MODEL.md` for logical resource classes,
4. `SCENE_VALIDATION.md` and `CAPABILITY_ADAPTATION.md` for stage ordering and failure semantics,
5. `RUNTIME_BOUNDARY.md` and `RUNTIME_SERVICE_SKETCH.md` for the lower execution boundary,
6. `IMPLEMENTATION_BRIDGE.md` for one tentative implementation-facing translation of this profile.
