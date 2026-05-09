> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 scene-facing `mesh` API, resource ownership model, and
>   mutation/update contract before the first implementation lands.

# Mesh API Design

This note defines how the future scene `mesh` visual family should sit between:

1. CPU-side geometry in `geom`,
2. scene-owned reusable resources,
3. panel-attached visuals,
4. frame-plan / DRP2 emission,
5. runtime-side buffer and texture updates.


## Objective

Land a first `mesh` visual family that is narrow enough for the active 3D slice, but structured so
future texturing, instancing, picking, transparency, and richer material work do not force a
redesign.


## Current Scene Surface

The active scene surface already provides:

1. scene/figure/panel/visual ownership in [include/datoviz/scene.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene.h)
2. per-panel visual attachment via `dvz_panel_add_visual()`
3. retained attribute data on visuals via `dvz_visual_set_data()`
4. retained attribute subrange updates via `dvz_visual_set_data_range()`
5. panel panzoom / arcball controller attachment
6. active visual families `point`, `primitive`, `path`, and `image`

The `mesh` family should fit this retained scene model rather than reintroducing a v0.3-style
batch/request layer.


## Recommended Ownership Split

The key design rule is to separate:

1. CPU geometry source data,
2. uploaded scene mesh resources,
3. visual instances attached to panels.

Recommended object roles:

1. `DvzGeometry`
   - CPU-side geometry container from the `geom` module
   - caller-owned
   - used for generation, import, preprocessing, and staging
2. scene mesh resource
   - scene-owned uploaded geometry asset
   - reusable across multiple visuals/panels
   - tracks GPU-facing buffer state and dirty ranges
3. `mesh` visual
   - scene-owned visual instance
   - attached to one or more panels like current visuals
   - references a mesh resource plus shading/material state

Do not collapse these into one object.


## Core Recommendation

The mesh visual should not own its geometry buffers directly by default.

Instead:

1. create or upload a mesh resource from `DvzGeometry`,
2. create a `mesh` visual that references that resource,
3. let multiple visuals reuse the same mesh resource with different transforms/materials if needed.

This is the clean retained-mode design and will scale better to:

1. shared meshes across panels,
2. repeated use and explicit instancing,
3. shared imported assets,
4. browser/runtime portability,
5. efficient partial updates.


## Proposed Public Concepts

The exact naming can still move, but the API should expose concepts in this shape:

1. `DvzGeometry` for CPU-side mesh data
2. `DvzMeshResource` for uploaded reusable scene geometry
3. `DvzVisual*` for the visual instance

The public scene-facing workflow should read naturally as:

1. generate/load a `DvzGeometry`,
2. upload it to a scene mesh resource,
3. create a mesh visual bound to that mesh resource,
4. attach the visual to a panel,
5. mutate transform/material/visibility/picking state on the visual,
6. mutate geometry through the mesh resource API when needed.


## Instancing Requirement

Instancing is an active mesh requirement, not just a future optimization.

Important use case:

1. many objects share one geometry resource,
2. each object has its own model transform,
3. each object may also need its own pick identity,
4. the runtime should be free to realize this as natural Vulkan instancing.

Typical example:

1. hundreds of squares or cubes sharing one mesh resource,
2. one transform per instance,
3. possibly one color or small style override per instance later.

The scene API should leave a direct place for this instead of forcing the caller to duplicate the
same geometry into many mesh resources.


## Visual Versus Resource State

The mesh resource and the mesh visual should own different kinds of state.

Mesh resource owns:

1. vertex and index data
2. vertex-format/attribute availability
3. resource-level dirty ranges
4. optional shared textures later only if the data is truly intrinsic to the mesh asset

Mesh visual owns:

1. visibility
2. object/model transform
3. material state
4. transparency mode
5. shading variant selection
6. picking/logical id state
7. per-panel attachment options such as z-layer and controller behavior

This split avoids making geometry replacement and visual styling the same operation.


## Visual Versus Instance State

Instancing adds one more explicit layer of ownership.

Recommended conceptual split:

1. mesh resource
   - shared geometry
2. mesh visual
   - shared shading/material/render-mode state for one rendered population
3. mesh instances within that visual
   - per-instance model transform
   - per-instance logical/pick identity
   - later small per-instance overrides if truly needed

This means the first mesh API should not hard-code the assumption that one mesh visual always means
exactly one object transform.


## Geometry Input Contract

The first `mesh` visual should consume geometry that matches the Phase-1 mesh shading note.

Expected baseline geometry:

1. positions
2. normals
3. vertex colors
4. indices

Deferred but planned:

1. UVs
2. tangents
3. texture bindings
4. contour/isoline extra payloads
5. PBR-specific vertex/material expansion

The mesh API should validate what attributes are present and select or reject shader variants
accordingly.


## Resource Creation Model

Recommended scene API direction:

1. create an empty mesh resource under a scene,
2. upload from a `DvzGeometry`,
3. optionally replace or partially update resource contents later.

Suggested conceptual API shape:

1. `DvzMeshResource* dvz_mesh_resource(DvzScene* scene, uint32_t flags);`
2. `int dvz_mesh_resource_upload(DvzMeshResource* mesh, const DvzGeometry* geom);`
3. `void dvz_mesh_resource_destroy(DvzMeshResource* mesh);`

The exact names can change, but the resource object should be explicit and scene-owned.


## Geometry Update Model

The `mesh` family needs both full replacement and partial updates from the beginning.

Required Phase-1 update classes:

1. full geometry replacement,
2. vertex subrange updates,
3. index subrange updates.

Recommended contract:

1. updates target the mesh resource, not the visual,
2. updates are retained and turned into dirty ranges for the next emit,
3. callers do not need to think in Vulkan buffer handles or staging buffers.

Suggested conceptual API shape:

1. `int dvz_mesh_resource_update_vertices(...)`
2. `int dvz_mesh_resource_update_indices(...)`
3. `int dvz_mesh_resource_replace(...)`

This aligns with the already-active scene rule that retained data mutations are rejected while an
emitted borrowed stream is still live.


## Instance Data Model

Instancing should have an explicit retained data model at the visual level.

Recommended first per-instance payload:

1. model transform
2. stable instance id

Good early optional additions:

1. per-instance base color multiplier
2. visibility/mask flag

The first public API does not need to expose a giant open-ended instance schema. A narrow retained
instance table is enough.


## Instance Update Model

The retained update contract should include instance data updates from the beginning.

Required update classes:

1. full instance-table replacement,
2. instance subrange updates for transforms and ids.

Recommendation:

1. geometry updates remain on the mesh resource,
2. instance updates remain on the mesh visual or on a visual-owned instance resource,
3. both follow the same retained dirty-range principles as the rest of the scene.

This keeps geometry sharing and instance churn separate.


## Texture Ownership

Texturing is deferred in the first narrow mesh slice, but the ownership boundary should be defined
now.

Recommendation:

1. textures should be separate scene resources,
2. samplers should be separate scene resources,
3. mesh visuals should reference texture/sampler resources through material or visual bindings,
4. textures should not be baked into the mesh resource identity unless a future asset layer really
   demands it.

This keeps the scene resource model consistent with the broader scene direction.


## Material State

The mesh visual should own material state, not the mesh resource.

Why:

1. one geometry asset may be shown with different materials in different panels,
2. material edits are visual styling changes, not geometry mutations,
3. this fits later transparency and picking overlays better.

Phase-1 material scope is the classic-lit baseline described in
[spec/scene/proposals/MESH_SHADING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/MESH_SHADING_DESIGN.md):

1. base color
2. ambient
3. diffuse
4. specular
5. shininess
6. emission
7. opacity

Do not encode material semantics as raw attribute arrays on the visual.


## Transform Ownership

For the active 3D slice:

1. geometry remains local/object-space,
2. the mesh visual owns its object/model transform,
3. panels own fixed camera/view/projection state,
4. panel arcball mutates the model transform for attached visuals that opt into controller
   application.

This matches the current active decision in
[spec/scene/proposals/HIGH_PRIORITY_SPEC_DECISIONS.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/HIGH_PRIORITY_SPEC_DECISIONS.md).

The mesh API should not require geometry rewrites just to rotate or reposition an object.


## Panel Attachment Semantics

The existing panel attachment model in `scene.h` already carries:

1. `z_layer`
2. `controller_mode`

The mesh family should use that directly.

Recommended behavior:

1. default mesh visuals attach with `controller_mode = APPLY`,
2. overlays related to the mesh (for example future labels or helper visuals) may attach as
   `FIXED` or via separate overlay mechanisms,
3. the mesh family itself should not reinvent panel-attachment semantics.


## Picking Contract

The mesh visual needs first-class picking integration.

Required mesh picking precision:

1. object-level picking,
2. face-level picking.

Deferred for mesh:

1. mesh vertex-level picking unless a concrete need appears later.

Recommended visual-owned picking state:

1. stable logical visual/object id,
2. opt-in pickable flag or mode,
3. face-level result routing through scene-owned lookup tables,
4. instance-aware result routing when the visual is instanced.

Recommended pick result for mesh visuals:

1. visual id
2. optional instance id
3. pick kind = mesh face
4. face id
5. optional hit position later

The mesh resource should provide the stable primitive ordering needed for face-id resolution.


## Scene-Facing Instancing Direction

The public API should support both:

1. one-instance mesh visuals for simple cases,
2. explicit instanced mesh visuals for shared-geometry populations.

Recommended conceptual direction:

1. a mesh visual may hold zero, one, or many instances,
2. one-instance use remains the trivial case,
3. many-instance use maps naturally to backend instancing where supported.

I would avoid creating a completely separate top-level visual family just for “instanced mesh”.
This is better treated as one capability of the mesh family.


## Transparency Contract

The mesh API should be ready for transparency modes even if the first implementation starts with
opaque rendering.

Recommendation:

1. transparency mode belongs to the visual/material side,
2. pass structure belongs to scene/frame-plan/runtime,
3. WBOIT should be treated as the intended high-priority transparent mode.

Immediate visual render modes to reserve:

1. opaque
2. transparent_wboit

Do not encode transparency only as “alpha less than one”.


## Shader Variant Selection

The mesh visual should choose a deliberate pipeline/shader family variant based on:

1. available geometry attributes,
2. visual shading/material mode,
3. transparency mode,
4. texture bindings if present.

Recommended first variants:

1. `mesh_lit`
2. later `mesh_lit_textured`
3. later `mesh_contour`
4. later `mesh_isoline`
5. later `mesh_pbr`

The mesh API should not expose all of these as backend-specific details, but it should be designed
so variant selection is explicit in the implementation rather than hidden in one oversized shader.


## Initial Public API Direction

The exact signatures can still evolve, but the intended scene-facing API should look roughly like
this conceptually.

Resource creation:

1. create scene mesh resource
2. upload/replace from `DvzGeometry`
3. update vertex/index subranges
4. destroy resource

Visual creation:

1. `DvzVisual* dvz_mesh(DvzScene* scene, uint32_t flags);`
2. bind a mesh resource to the visual
3. set instance data on the visual
4. set material/shading state on the visual
5. attach the visual to a panel

Suggested conceptual calls:

1. `int dvz_mesh_set_resource(DvzVisual* visual, DvzMeshResource* mesh);`
2. `int dvz_mesh_set_instances(...)`
3. `int dvz_mesh_set_instance_range(...)`
4. `int dvz_mesh_set_material(...)`
5. `int dvz_mesh_set_render_mode(...)`
6. `int dvz_mesh_set_pickable(...)`

The exact naming is less important than keeping resource mutation and visual styling separate.


## Why Not Reuse `dvz_visual_set_data()`

The active generic `dvz_visual_set_data()` API is the right fit for first-slice point/path/image,
but it is not the best long-term fit for scene meshes.

Reasons:

1. mesh geometry is more naturally a reusable resource than one private per-visual attribute blob,
2. mesh picking needs stable face ordering on a shared resource,
3. mesh resources need explicit vertex/index update APIs,
4. later texturing/material/resource sharing is cleaner with a distinct resource layer,
5. imported/generated geometry should not need to be copied independently into every visual,
6. explicit instancing is cleaner when shared geometry and per-instance state are not conflated.

So the mesh family should introduce an explicit scene resource model rather than forcing everything
through generic visual attribute setters.


## Lifetime Rules

Mesh resource lifetime should follow scene-owned resource rules.

Recommended rules:

1. visuals hold weak or retained references to mesh resources owned by the scene,
2. destroying a visual does not destroy shared resources,
3. destroying a mesh resource should be rejected while any visual still references it or while any
   emitted borrowed stream is still live,
4. resource mutation should be rejected while a borrowed emitted stream is live, just like current
   retained visual data mutation.


## Serialization Expectations

The current scene JSON model already serializes visuals and retained visual data.

For mesh resources, the intended direction should be:

1. mesh resources become explicit serializable scene-owned objects,
2. visuals reference them by stable scene id,
3. shared geometry should remain shared across serialization instead of being duplicated through
   every visual.

This does not need to be implemented in the first code slice, but it is the right contract.


## Immediate Scope Recommendation

For the first implementation, the narrowest useful mesh API slice is:

1. explicit mesh resource object,
2. upload from `DvzGeometry`,
3. full replace and vertex/index subrange updates,
4. mesh visual bound to one mesh resource,
5. one-instance and multi-instance paths using one shared geometry resource,
6. per-instance model transforms,
7. classic-lit material state on the visual,
8. opaque mode first,
9. face-picking-ready resource ordering,
10. one offscreen and one GLFW example using a colored cube.


## Explicit Non-Goals For The First Slice

1. multiple materials per one mesh resource
2. skeletal animation
3. per-instance arbitrary material structs
4. contour/isoline visual variants
5. texture-material combinatorics
6. PBR implementation
7. a public asset-management framework broader than what scene resources already need
