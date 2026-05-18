> **Execution Status**
> - **Status:** `PARTIALLY IMPLEMENTED MATERIAL SLICE; LIGHT OBJECTS STILL PROPOSED`
> - **Updated on:** `2026-05-17`
> - **Purpose:** define the intended v0.4 scene-facing material and lighting object model for mesh
>   and future lit visual families.

# Material and Lighting API

This note narrows the larger lighting and mesh-shading discussion into the active scene-facing API
decisions needed before the first mesh family lands.


## Objective

Define a material and lighting model that:

1. supports the first lit mesh slice now,
2. stays compatible with future lit spheres and similar 3D families,
3. avoids dragging the old v0.3 light/material packing into v0.4,
4. leaves room for texture bindings, WBOIT, and later PBR growth.


## Current Implementation State

As of `2026-05-17`, the first material slice is active. `DvzMaterialDesc` and
`dvz_visual_set_material()` provide a value-descriptor material API for primitive, mesh, and sphere
visuals; Phong and standard models lower through shared material shader helpers; and
`dvz_visual_set_primitive_shading()` now forwards through the unified material path. Depth cueing
remains a separate typed setter that composes with the material payload. Scene-owned light objects
and panel light sets described below remain future design; the current runtime still uses compact
material/light-direction fields rather than a full reusable light-object API.


## Existing Grounding In The Repo

Relevant context already exists here:

1. mesh shading direction:
   [MESH_SHADING_DESIGN.md](MESH_SHADING_DESIGN.md)
2. mesh visual ownership split:
   [MESH_API_DESIGN.md](MESH_API_DESIGN.md)
3. older broad lighting note:
   [spec/scene/semantics/LIGHTING.md](../../semantics/LIGHTING.md)
4. v0.3 shader reference:
   [v0.3/include/datoviz/scene/glsl/lighting.glsl](../../../../v0.3/include/datoviz/scene/glsl/lighting.glsl)

This note records the active recommendation where those sources are still too broad or too tied to
older assumptions.


## Core Recommendation

Separate:

1. light objects,
2. panel light sets,
3. visual-owned material state.

Recommended ownership:

1. lights are explicit scene-owned reusable objects,
2. panels choose which lights affect them,
3. lit visuals own materials and shading mode,
4. visuals do not own lights directly by default.

This is the cleanest model for multi-panel scenes.


## Why Panel-Local Light Sets

The active light set should be panel-local, not implicitly one global scene lighting state.

Reasons:

1. different panels may show the same object from different scientific presentation angles,
2. one panel may want neutral inspection lighting while another wants more dramatic depth cues,
3. this matches panel-owned camera state better than global scene-only light binding,
4. it avoids making later browser/export adaptation depend on hidden global state.

Recommended rule:

1. lights are scene-owned objects,
2. each panel selects a light set from those scene-owned lights,
3. panels may fall back to a default light set if not configured explicitly.


## Material Ownership

Material state belongs to the visual, not the mesh resource and not the panel.

Why:

1. one geometry resource may be reused by multiple visuals,
2. different panels may want different material presentation for the same geometry,
3. opacity and shading mode are visual concerns that interact with picking and transparency.

This matches the mesh API design note and should remain stable across lit visual families.


## First Lit Material Model

The first active material model should be classic lit, explicit, and small.

Recommended Phase-1 fields:

1. `base_color`
2. `ambient`
3. `diffuse`
4. `specular`
5. `shininess`
6. `emission`
7. `opacity`

Recommendation:

1. keep this as an explicit semantic material object or descriptor,
2. do not encode it as raw `mat4` blocks in the public API,
3. do not pretend it is already a full asset-material system.


## Base Color Policy

The role of `base_color` should be explicit.

Recommended rule:

1. `base_color` modulates the primary surface color contribution,
2. vertex color and texture albedo, when present, combine with `base_color`,
3. specular color remains explicit and is not inferred from `base_color`,
4. opacity comes from material opacity multiplied by any alpha present in surface color inputs.

This keeps the first model simple without baking in one monolithic color-source rule forever.


## Light Types

The first active light types should be:

1. ambient
2. directional
3. point

That is enough for the current scientific 3D slice.

I would not introduce spot lights in the first pass unless an actual use case appears.


## Light Data Shape

The public light model should use explicit semantic fields.

Recommended conceptual shape:

1. light type
2. color
3. intensity
4. direction for directional lights
5. position for point lights
6. attenuation parameters for point lights later if needed

Do not preserve v0.3-style `mat4` packing as a public-facing model.


## Default Lighting Policy

The scene should provide useful defaults without forcing user configuration for every 3D example.

Recommended default:

1. one ambient light,
2. one directional light,
3. each new 3D panel inherits a default light set unless overridden.

This gives a sane out-of-the-box result for the first colored cube example while still allowing
panel-level control.


## Light Count Policy

Keep the first active light count intentionally small.

Recommendation:

1. guarantee support for at least one ambient plus a small fixed array of directional/point lights,
2. start with a panel light set size of 4 practical lights if that fits current runtime wiring,
3. reject or diagnose overflow rather than silently changing semantics.

This is enough for the active rendering goals and keeps the first UBO model straightforward.


## Coordinate Space Convention

Lighting conventions should be explicit now.

Recommended first implementation:

1. lighting is computed in world space,
2. object/model transforms move geometry into world space,
3. panel camera/view/projection transforms then apply normally,
4. directional lights are expressed in world-space direction,
5. point lights are expressed in world-space position.

Do not recompute camera inverse per vertex if panel camera position can be supplied directly.


## Shading Model Selection

The scene-facing API should distinguish material state from shading model choice.

Recommended first shading modes:

1. `unlit`
2. `classic_lit`

Deferred:

1. `pbr`
2. contour/isoline-specific surface modes

This matters because not every mesh-like visual should be forced into lit shading, and some
annotation or helper geometry may intentionally be unlit.


## Relationship To Transparency

Opacity is material data, but transparency is also a render-mode decision.

Recommended split:

1. material owns `opacity`,
2. visual/render state owns `opaque` versus `transparent_wboit`,
3. frame-plan/runtime decide the pass structure.

Do not make “opacity < 1” the whole transparency API.


## Relationship To Textures

Texture bindings should be separate resources referenced by the visual/material path.

Recommended near-term policy:

1. Phase 1 can be color-only,
2. later add albedo texture binding cleanly,
3. sampler and texture remain explicit scene resources,
4. material/shading state decides how those resources are interpreted.

This avoids making textures part of mesh resource identity.


## Relationship To Picking And Highlighting

Material/lighting state should not own picking identity, but it should be compatible with
highlighting overlays.

Recommendation:

1. picking results resolve to visual/object and face/item identities,
2. highlight or selection styling can modify material-like presentation,
3. the base material object remains distinct from transient highlight state.

This avoids coupling selection mechanics to permanent material mutation.


## Future PBR Growth

The API should leave room for PBR without pretending PBR is implemented now.

Recommended reserved growth path:

1. add `metallic`
2. add `roughness`
3. add `emissive_color`
4. later add normal-map and metallic-roughness texture bindings

Recommendation:

1. reserve semantic room in the material model now,
2. do not freeze the first classic-lit struct in a way that makes PBR a second incompatible API,
3. do not overbuild full PBR machinery before the current 3D slice and WBOIT path are working.


## Relationship To Existing `spec/scene/semantics/LIGHTING.md`

The broad lighting note is still useful, but I would tighten one design point for active work:

1. keep scene-owned light objects,
2. move the active light-set binding decision to the panel level,
3. keep material ownership on visuals,
4. treat the broad note as strategic context rather than the exact active contract.

This is the main place where I would be stricter than the earlier high-level document.


## Initial Public API Direction

The exact names can still move, but the conceptual surface should look like:

1. create/update/destroy light objects,
2. assign a set of lights to a panel,
3. set a visual material,
4. set a visual shading mode.

Likely conceptual calls:

1. `dvz_light_*`
2. `dvz_panel_set_lights(...)`
3. `dvz_mesh_set_material(...)`
4. `dvz_mesh_set_shading_mode(...)`

For other future lit families, the same pattern should hold.


## Immediate Scope Recommendation

The narrowest useful first implementation slice is:

1. classic-lit material object on mesh visuals,
2. ambient plus directional default light set,
3. panel-local selection of the active light set,
4. explicit `unlit` and `classic_lit` shading modes,
5. opacity field present from the start even if the first mesh example is opaque.


## Explicit Non-Goals For The First Slice

1. full multi-material assets,
2. every possible light type,
3. automatic physically based energy calibration,
4. texture/material combinatorics beyond the first actual need,
5. full PBR implementation.
