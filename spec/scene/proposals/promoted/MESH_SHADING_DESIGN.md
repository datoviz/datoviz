> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the first v0.4 `mesh` shading contract so Phase 1 implementation stays
>   narrow without blocking later contour/isoline, transparency, or PBR work.

# Mesh Shading Design

This note scopes the first v0.4 `mesh` slice and records the intended growth path.


## Objective

Land one native 3D mesh path that is simple enough to implement now, but structured so later mesh
features do not require replacing the entire visual family.

The first implementation should prove:

1. indexed triangle geometry,
2. depth-tested opaque rendering,
3. world/view/projection transform correctness,
4. arcball-driven 3D interaction,
5. a small light/material contract that can grow later.


## Design Rules

1. Keep geometry, surface/material, lighting model, and pass/compositing concerns separate.
2. Do not treat the v0.3 mesh shader as the public contract; treat it as reference material only.
3. Do not make contour, isoline, transparency, or PBR requirements part of the baseline mesh path.
4. Do not make the first mesh implementation so narrow that future shader families need a new mesh
   object model.
5. Prefer explicit pipeline variants over one monolithic shader with many latent modes.


## v0.3 Reuse Decision

What is worth carrying forward from `./v0.3`:

1. the basic lighting model shape from
   [v0.3/include/datoviz/scene/glsl/lighting.glsl](../../../../v0.3/include/datoviz/scene/glsl/lighting.glsl)
2. the practical mesh feature surface proven by old examples/tests: indexed geometry, normals,
   optional texturing, configurable light/material defaults
3. shape/OBJ expectations from the old mesh path

What should not be ported directly:

1. the old scene/visual plumbing
2. the old `mat4`-packed light/material API shape
3. the overloaded vertex format that mixes baseline mesh with contour/isoline payloads
4. the single mesh shader that multiplexes too many modes via specialization constants


## Phase 1 Baseline

The first v0.4 `mesh` path should support only:

1. indexed triangle-list rendering
2. vertex `position`
3. vertex `normal`
4. vertex `color`
5. one opaque lit pass
6. depth compare/write
7. per-panel viewport/scissor
8. per-panel transform state

Explicitly deferred from the first implementation:

1. UV texturing
2. contour/wireframe overlay
3. isolines
4. transparency
5. multi-material meshes
6. normal maps and tangent-space work
7. PBR shading


## Geometry Contract

The baseline mesh geometry contract should be stable across future shader families.

Required baseline attributes:

1. `position : vec3`
2. `normal : vec3`
3. `color : rgba8` or equivalent packed vertex color

Deferred optional attributes:

1. `uv : vec2`
2. `tangent : vec4`
3. scalar field / isoline value
4. barycentric or edge metadata for contour-style shading
5. skinning or instancing payloads

Indexed geometry should be the default path for Phase 1. Non-indexed support may exist later, but
it should not shape the first implementation.


## Transform and Shading Space

The first mesh lighting path should be explicit about coordinate spaces.

Rules:

1. mesh vertex positions are object/local-space inputs,
2. model, view, and projection transforms remain panel/scene controlled,
3. lighting calculations happen in world space for the first implementation,
4. normals are transformed with an explicit normal matrix or equivalent inverse-transpose model
   handling,
5. camera position should be supplied intentionally by the runtime/scene path rather than recovered
   indirectly in every shader stage unless there is a clear need.

The baseline should not inherit ambiguous v0.3 conventions around flipped axes or implicit shader
space fixes.


## Material Contract

The public material concept should be broader than the first implementation.

Phase 1 classic-lit material fields:

1. `base_color`
2. `ambient`
3. `diffuse`
4. `specular`
5. `shininess`
6. `emission`
7. `opacity`

Recommended design rule:

1. expose an explicit mesh surface/material object or descriptor,
2. do not encode material semantics as raw `mat4` blocks in the public-facing API,
3. keep the internal GPU layout free to use std140/std430-friendly packing as needed.

Why this shape:

1. it maps cleanly to the old v0.3 lighting baseline,
2. it is enough for the first lit mesh slice,
3. it leaves an obvious place for future PBR fields without redefining what a mesh is.

Reserved future material growth:

1. `metallic`
2. `roughness`
3. `emissive_color`
4. `occlusion`
5. texture slots for albedo, normal, metallic-roughness, and emissive maps


## Light Contract

The light object should not be named or structured as a Phong-only type even if the first
implementation uses simple Blinn-Phong shading.

Phase 1 light fields:

1. `position_or_direction : vec4`
2. `color : vec4`
3. optional explicit `intensity` if not folded into color

Behavior:

1. `w == 0` means directional light
2. `w == 1` means point/positioned light

Implementation recommendation:

1. support one light first if that keeps the runtime smaller,
2. keep the struct layout ready for a small fixed array such as four lights,
3. avoid v0.3-style dependence on alpha as the only enable/brightness control if a clearer
   intensity field is practical.


## Shader Family Strategy

Do not grow a single mesh shader into a permanent mode switchboard.

Preferred family progression:

1. `mesh_unlit`
2. `mesh_lit`
3. `mesh_lit_textured`
4. `mesh_contour`
5. `mesh_isoline`
6. `mesh_pbr`

Shared code should live in common GLSL includes where it really is shared:

1. transform helpers
2. light/material structs
3. classic lighting helpers
4. later PBR helpers

This keeps the DRP2/runtime surface explicit: a mesh visual should select a deliberate pipeline
variant rather than hiding broad behavior behind shader branching.


## Contour and Isoline Return Path

Contour and isoline are legitimate future mesh capabilities, but they should return as dedicated
shader variants rather than as baseline mesh requirements.

Why:

1. they require extra vertex payloads such as scalar values, barycentric data, or edge metadata,
2. they use fragment-stage antialiasing logic that is unrelated to the baseline opaque lit path,
3. they may want a different draw path or preprocessing choice later.

Current intent:

1. keep the Phase 1 mesh geometry contract free of contour/isoline-specific baggage,
2. allow a future mesh variant or overlay path to reintroduce the old single-pass fragment-shader
   approach if it still makes sense,
3. do not commit now to whether contour/isoline remains single-pass forever.


## PBR Growth Path

PBR is deferred, but the current design should leave it room.

Rules for future compatibility:

1. the mesh object model should not assume classic-lit material semantics are the only material
   semantics,
2. the shader family split should make `mesh_pbr` a new pipeline variant, not a rewrite of mesh,
3. light and material descriptors should be extensible without inventing a second mesh API.

What the first implementation should avoid if PBR is expected later:

1. hard-coding `ambient/diffuse/specular` names into every mesh-facing object layer,
2. coupling texture support to only one classic shader path,
3. hiding shading model choice in ad hoc flags that cannot scale cleanly.

Pragmatic recommendation:

1. implement classic lighting first,
2. keep the public material container broad enough for later PBR fields,
3. add PBR only after the native 3D baseline, browser feasibility pass, and transparency pressure
   have clarified the right seams.


## Improvements Over v0.3 To Take During Port

The first port should deliberately improve the old mesh lighting path in the following ways:

1. replace `mat4`-encoded light/material API shapes with explicit semantic structs,
2. remove contour/isoline payloads from the baseline mesh vertex format,
3. make world-space lighting conventions explicit,
4. avoid recomputing camera state in shaders when it can be supplied directly,
5. keep ambient/diffuse/specular/emission contributions explicit instead of preserving old ad hoc
   mixes by accident,
6. use pipeline families instead of one feature-heavy mesh shader,
7. keep the implementation visually close enough to v0.3 that old examples remain a useful quality
   reference.


## Recommended Implementation Order

1. Define the first mesh geometry and material/light structs in scene-facing code.
2. Add the DRP2/runtime support needed for per-panel depth attachments and depth state.
3. Implement one opaque `mesh_lit` pipeline variant with position/normal/color.
4. Land `hello_mesh` offscreen and one live arcball example.
5. Add focused `scene` and `drp2` tests for depth, viewport/scissor, and basic lit mesh replay.
6. Revisit UV texturing next.
7. Revisit contour/isoline only after the baseline 3D path is stable.


## Explicit Non-Goals For Phase 1

1. a broad mesh feature zoo
2. a full material system
3. PBR implementation
4. transparency implementation
5. preserving the exact v0.3 mesh shader architecture
