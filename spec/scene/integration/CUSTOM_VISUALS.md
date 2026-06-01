# Custom Visual Families

Status: deferred design direction. This is not an installed v0.4 public C API.

This document defines how user-defined visual families are registered and integrated
into the scene layer.


## Purpose

Built-in visual families cover the common scientific visualization cases. In v0.4, general custom
visual/render shaders are explicitly deferred; `DvzSceneCompute` is a narrower advanced/unstable
compute path and does not expose built-in visual shader replacement.

The future custom visual direction is that custom visual families allow users to define new
rendering primitives that:

1. participate fully in the scene machinery (picking, selection, invalidation,
   capability adaptation, transparency),
2. are treated identically to built-in visuals from the scene's perspective,
3. require only a descriptor declaration and shader sources — no scene internals exposure.


## Registration

A custom visual family is registered with a scene by providing a `DvzVisualDesc`:

```text
DvzVisual* visual = dvz_visual_custom(scene, &desc, user_data)
```

`DvzVisualDesc` is a flat descriptor struct that declares everything the scene needs
to integrate the visual.
Once registered, the visual is used exactly like a built-in visual:

```text
dvz_visual_set_data(visual, "position", positions, n)
dvz_panel_add_visual(panel, visual)
```


## Descriptor Fields

Status on 2026-05-17: this section is the focused home for the useful custom-visual descriptor
sketch from the retired broad scene API draft. The exact installed C names are still future API
work; the semantic fields below are the design contract.

### Attribute Schema

```text
desc.attributes      — array of DvzAttrDesc
desc.attribute_count — number of attribute slots
```

Each `DvzAttrDesc` declares:

| Field | Description |
|---|---|
| `name` | attribute name (e.g., `"position"`, `"color"`) |
| `format` | data type: `DVZ_FORMAT_VEC3_F32`, `DVZ_FORMAT_RGBA_U8`, etc. |
| `source` | allowed sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`, or a combination |
| `required` | whether the attribute must be set before the visual can render |

The scene uses the attribute schema to allocate GPU buffers, validate uploads,
and map `dvz_visual_set_data` calls to the correct buffer slots.


### Shader Sources

```text
desc.shader_language — WGSL, GLSL, or SPIR-V
desc.vertex_shader   — source or path, vertex stage
desc.fragment_shader — source or path, fragment stage
desc.compute_shader  — optional; compute stage for pre-pass work
```

Custom visual shader descriptors declare their source language explicitly.

Rules:

1. WGSL is the portable DRP2 contract language and the only language a conforming DRP2 `2.0`
   runtime must accept.
2. Built-in scene shaders may be authored in GLSL internally during the native Vulkan bring-up, but
   the scene-to-DRP2 converter must emit a DRP2-supported shader format (`wgsl` or a capability-gated
   native format).
3. GLSL custom visual sources are accepted only when the runtime capability snapshot advertises a
   GLSL ingestion path.
4. Browser-portable custom visuals should use WGSL.

This resolves the apparent discrepancy between native built-in shader authoring and the DRP2
transport contract: authoring language is an implementation choice; DRP2 shader module format is
the runtime-facing contract.

Scene compute interop is a narrower API than custom visuals. `DvzSceneCompute` may accept custom
compute shader source because its purpose is to run user-defined GPU work, but that does not imply
that built-in visuals expose their vertex or fragment shaders. The intended boundary is:

1. custom compute writes scene buffers;
2. normal visuals consume those buffers through declared attributes;
3. the FramePlan records dependencies and barriers;
4. custom visual shaders remain a separate, broader feature.

**Shader hot reload** is not supported in v0.4. To update a custom visual's shaders, the visual
must be destroyed and recreated.

The scene compiles and caches the shaders.
Shader variants (e.g., with/without picking, with/without item-state support) are generated
automatically by the scene's shader preprocessor using standard insertion points
(see Standard Injections below).


### Pipeline Descriptor

```text
desc.topology        — primitive topology: DVZ_TOPOLOGY_POINT_LIST, _LINE_LIST,
                       _TRIANGLE_LIST, _TRIANGLE_STRIP, etc.
desc.alpha_mode      — DVZ_ALPHA_OPAQUE, _BLENDED, _WBOIT, _DEPTH_PEEL, or _MASK
desc.depth_test      — bool (default true)
desc.cull_mode       — DVZ_CULL_NONE, _BACK, _FRONT
```


### Picking Support

```text
desc.pickable        — bool (default false)
```

When `true`, the scene generates a picking variant of the visual's shaders.
The picking variant writes the item ID to the picking attachment using the standard
convention: the item ID is `gl_InstanceIndex` (the item index). Custom picking strategies
(compound IDs, object-space picking) are not supported in v0.4 — the default instance-index
convention is the only available option.

The user does not write picking shader code explicitly — the scene inserts it.


### Selection Support

```text
desc.selectable      — bool (default false)
```

When `true`, the scene injects the `item_state` attribute and standard `item_state_style`
uniforms into the shader.
The shader receives a `uint dvz_item_state` bitfield and must apply it to produce the highlight
effect.

A standard item-state helper function is provided:

```glsl
vec4 dvz_apply_item_state_color(vec4 base_color, uint item_state);
```

The user calls this in their fragment shader to get highlight behavior consistent with
built-in visuals.


### Uniform Layout

```text
desc.uniforms        — array of DvzUniformDesc (user-defined uniform blocks)
desc.uniform_count   — number of user uniform blocks
```

**Automatically injected resources** — the scene provides these through reserved slots; the user
does not declare them:

| Resource | Content |
|---|---|
| panel transform | MVP matrix and viewport |
| `item_state` | per-item state bitfield attribute when `selectable = true` |
| `item_state_style` | selected/unselected/hovered style uniform when `selectable = true` |

User-declared uniforms are bound after the reserved scene-standard resources.
The user sets them via:

```text
dvz_visual_set_uniform(visual, slot_index, data, size)
```


### Texture Slots

```text
desc.sampled_fields      — array of sampled-field slot descriptors
desc.sampled_field_count — number of sampled-field slots
```

Each sampled-field slot descriptor declares the semantic slot name, shader binding, dimensionality,
format expectations, and sampler type. Fields are set via the ordinary visual field-binding API:

```text
dvz_visual_set_field(visual, "slot_name", field)
```

Backend texture and sampler objects remain runtime materialization details, not custom-visual API
objects.


### Capability Requirements

```text
desc.required_caps   — bitmask of DvzCapability flags
```

If a required capability is absent at runtime, the visual is deactivated and a
diagnostic is emitted.
Optional capabilities (e.g., exact OIT) should not appear here — declare them via
`alpha_mode` instead, which has its own fallback path.


### Invalidation Scope

```text
desc.invalidation_scope — DVZ_INVALIDATE_FULL (default) or DVZ_INVALIDATE_PARTIAL
```

`DVZ_INVALIDATE_FULL`: any attribute or uniform change marks the entire visual dirty.
`DVZ_INVALIDATE_PARTIAL`: the visual declares a `dvz_invalidation_fn` callback that
receives the changed attribute name and returns the dirty scope.
Partial invalidation is optional and only needed for performance-sensitive visuals.

The scene always fully invalidates a custom visual when any of its data or parameters change,
unless `DVZ_INVALIDATE_PARTIAL` is declared with a callback. Granular per-attribute dirty
tracking for custom visuals is a v0.4+ optimization concern.


## Standard Shader Injections

The scene's shader preprocessor inserts standard code at named injection points:

| Injection point | Inserted content |
|---|---|
| `// DVZ_INJECT_UNIFORMS` | panel transform and scene-standard uniform blocks |
| `// DVZ_INJECT_PICKING` | picking attachment output (picking variant only) |
| `// DVZ_INJECT_SELECTION` | item-state binding and `dvz_item_state` variable |

The user places these comments in their shader source at the appropriate locations.
If an injection point is absent, the scene appends the injected code at the end of the
relevant block (uniforms → top of shader; picking/selection → fragment output section).


## Lifecycle

A custom visual follows the same lifecycle as built-in visuals:

1. `dvz_visual_custom(scene, &desc, user_data)` — registers the family and allocates GPU resources,
2. `dvz_visual_set_data(visual, attr, data, n)` — uploads attribute data,
3. `dvz_panel_add_visual(panel, visual)` — adds to a panel,
4. normal frame lifecycle: invalidation, validation, capability adaptation, frame planning,
5. `dvz_visual_destroy(visual)` — frees GPU resources.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `../semantics/VISUAL_CONTRACT.md` | normative contract that custom visuals satisfy |
| `semantics/TRANSPARENCY.md` | alpha_mode and render pass assignment |
| `interaction/SELECTION.md` | item-state injection and highlight helper |
| `interaction/PICKING.md` | picking variant generation |
| `validation/ADAPTATION.md` | capability gating and deactivation |
| `pipeline/INVALIDATION_AND_CACHING.md` | dirty scope declaration |
| `semantics/NONLINEAR_TRANSFORMS.md` | custom compute shader registration for projections |
