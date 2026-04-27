# Custom Visual Families

This document defines how user-defined visual families are registered and integrated
into the scene layer.


## Purpose

Built-in visual families cover the common scientific visualization cases.
Custom visual families allow users to define new rendering primitives that:

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
dvz_visual_set_data(visual, DVZ_ATTR_POSITION, positions, n)
dvz_panel_add_visual(panel, visual)
```


## Descriptor Fields

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
desc.vertex_shader   — GLSL source or path, vertex stage
desc.fragment_shader — GLSL source or path, fragment stage
desc.compute_shader  — optional; compute stage for pre-pass work
```

The scene compiles and caches the shaders.
Shader variants (e.g., with/without picking, with/without selection mask) are generated
automatically by the scene's shader preprocessor using standard insertion points
(see Standard Injections below).


### Pipeline Descriptor

```text
desc.topology        — primitive topology: DVZ_TOPOLOGY_POINT_LIST, _LINE_LIST,
                       _TRIANGLE_LIST, _TRIANGLE_STRIP, etc.
desc.alpha_mode      — DVZ_ALPHA_OPAQUE, _BLENDED, _BLENDED_EXACT, or _MASK
desc.depth_test      — bool (default true)
desc.cull_mode       — DVZ_CULL_NONE, _BACK, _FRONT
```


### Picking Support

```text
desc.pickable        — bool (default false)
```

When `true`, the scene generates a picking variant of the visual's shaders.
The picking variant writes the item ID to the picking attachment using the standard
convention: the item ID is the vertex instance index unless the visual overrides it
via the `dvz_item_id` output variable in the vertex shader.

The user does not write picking shader code explicitly — the scene inserts it.


### Selection Support

```text
desc.selectable      — bool (default false)
```

When `true`, the scene injects the selection mask buffer binding and a standard
`dvz_selection_mask` uniform into the shader.
The fragment shader receives a `uint8 dvz_item_selected` variable (1 = selected,
0 = unselected) and must apply it to produce the highlight effect.

A standard highlight helper function is provided:

```glsl
vec4 dvz_apply_highlight(vec4 base_color, uint8_t selected, DvzHighlightDesc desc);
```

The user calls this in their fragment shader to get highlight behavior consistent with
built-in visuals.


### Uniform Layout

```text
desc.uniforms        — array of DvzUniformDesc (user-defined uniform blocks)
desc.uniform_count   — number of user uniform blocks
```

**Automatically injected uniforms** — the scene always provides these at fixed binding
points; the user does not declare them:

| Binding | Content |
|---|---|
| 0 | panel transform (MVP matrix, viewport) |
| 1 | selection mask buffer (when `selectable = true`) |
| 2 | highlight descriptor (when `selectable = true`) |

User-declared uniforms are bound at slots starting from 3.
The user sets them via:

```text
dvz_visual_set_uniform(visual, slot_index, data, size)
```


### Texture Slots

```text
desc.textures        — array of DvzTextureSlotDesc
desc.texture_count   — number of texture slots
```

Each `DvzTextureSlotDesc` declares the slot index and sampler type.
Textures are set via:

```text
dvz_visual_set_texture(visual, slot_index, texture)
```


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


## Standard Shader Injections

The scene's shader preprocessor inserts standard code at named injection points:

| Injection point | Inserted content |
|---|---|
| `// DVZ_INJECT_UNIFORMS` | panel transform and scene-standard uniform blocks |
| `// DVZ_INJECT_PICKING` | picking attachment output (picking variant only) |
| `// DVZ_INJECT_SELECTION` | mask buffer binding and `dvz_item_selected` variable |

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
| `VISUAL_CONTRACT.md` | normative contract that custom visuals satisfy |
| `TRANSPARENCY.md` | alpha_mode and render pass assignment |
| `SELECTION.md` | selection mask injection and highlight helper |
| `PICKING.md` | picking variant generation |
| `CAPABILITY_ADAPTATION.md` | capability gating and deactivation |
| `INVALIDATION_AND_CACHING.md` | dirty scope declaration |
| `NONLINEAR_TRANSFORMS.md` | custom compute shader registration for projections |


## Deferred Questions

1. shader language policy — GLSL for v0.4 and all built-in shaders; when the browser/WebGPU
   target is ready, shaders will be converted to WGSL once (manually, AI-assisted) rather than
   maintained in parallel or transpiled at runtime,
2. hot reload of custom visual shaders at runtime,
3. partial invalidation callback API spelling,
4. whether custom visuals can declare their own per-item picking ID strategy beyond
   the default instance-index convention.
