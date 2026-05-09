> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 transform ownership model, controller behavior, and the
>   relationship between model-space interaction, panel camera state, and overlay placement.

# Transform and Controller Design

This note records the intended v0.4 transform and controller model after the first controller pass
has already landed.


## Objective

Keep the transform model simple, explicit, and compatible with:

1. model-space arcball interaction,
2. fixed panel cameras for the current object-inspection use cases,
3. pan/zoom for 2D views,
4. future camera-orbit and fly/FPS camera modes,
5. world-space text and annotations,
6. future measurement overlays and picking.


## Existing Grounding In The Repo

The active branch already has controller primitives and an implemented first pass:

1. arcball header:
   [include/datoviz/scene/arcball.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene/arcball.h)
2. panzoom header:
   [include/datoviz/scene/panzoom.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene/panzoom.h)
3. earlier implementation record:
   [agents/done/CONTROLLER_TRANSFORM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/CONTROLLER_TRANSFORM_DESIGN.md)

This note narrows the public and architectural intent around those pieces.


## Core Recommendation

The active v0.4 model should be:

1. geometry stores local/object-space data,
2. visuals own model/object transforms,
3. panels own camera/view/projection state,
4. model-space arcball rotates objects while the camera remains fixed,
5. panzoom adjusts panel navigation for 2D-style views,
6. camera-centric controllers are distinct from model-space arcball,
7. overlays and annotations declare explicitly whether they participate in controller transforms.


## Why Model-Space Arcball

For the active use cases, arcball should mutate the model transform while the camera remains fixed.

Reasons:

1. it matches the intended scientific object-inspection workflow,
2. it keeps the world and camera conventions stable,
3. it avoids making scene labels and measurements feel like the camera is orbiting around them,
4. it is already consistent with the current arcball implementation vocabulary in the branch.

Recommendation:

1. keep model-space arcball as the primary arcball mode,
2. treat camera-orbit behavior as a later distinct controller mode rather than the same API.

This remains the right default for the current mesh and annotation work, but it should not block a
real camera-controller family.


## Transform Layers

The scene transform model should be expressed as distinct layers:

1. geometry local/object space
2. visual object/model transform
3. panel view transform
4. panel projection transform
5. panel viewport transform

This is the right place to stay explicit instead of letting transforms leak across layers.


## Geometry Space

`DvzGeometry` should hold local/object-space positions.

Recommendation:

1. geometry generation/import never bakes in controller state,
2. geometry mutation is for data changes, not for interactive view changes,
3. object placement and rotation belong above the geometry layer.

This is important for reuse, picking, and partial updates.


## Visual Transform Ownership

Each visual should conceptually own an object/model transform.

Why:

1. multiple visuals may share one geometry resource but need different placements,
2. world-space annotations and labels need their own placement transforms,
3. model-space arcball should act on visual/object transforms, not rewrite geometry.

Recommendation:

1. mesh visuals own model transforms,
2. future world-space text and annotations also own transforms,
3. geometry resources remain transform-free reusable data assets.


## Panel Camera Ownership

Panels should own the camera/view/projection state.

For the current active design:

1. the camera is fixed by default,
2. the model-space arcball rotates visuals,
3. panzoom modifies panel navigation for 2D or quasi-2D views,
4. camera-orbit behavior is not the default arcball meaning.

This gives a clear separation:

1. object manipulation via model transforms,
2. panel navigation via panel camera/panzoom state.


## Camera Controller Family

Camera manipulation is also important and should be recognized explicitly now.

Recommended camera-controller family:

1. orbit camera
2. fly camera
3. FPS camera

These should be treated as camera controllers, not as alternate meanings of model-space arcball.


## Orbit Camera

The orbit camera is the most likely next 3D controller after model-space arcball.

Recommended behavior:

1. camera orbits around a target point,
2. target point remains explicit,
3. dolly/zoom changes distance to target,
4. pan translates target and camera together when supported.

This is the best camera-centric complement to the current object-centric arcball.


## Fly Camera

Fly camera is also worth reserving now.

Recommended behavior:

1. camera position moves freely through 3D space,
2. orientation changes from mouse/drag or equivalent input,
3. translation uses forward/right/up motion,
4. no fixed orbit target is required.

This is useful for immersive inspection and internal navigation.


## FPS Camera

FPS camera is close to fly camera but not identical in expected constraints.

Recommended behavior:

1. movement is primarily along a ground or application-defined plane,
2. yaw/pitch semantics are explicit,
3. roll is typically suppressed,
4. applications may choose whether vertical lift is allowed.

This may matter for certain anatomy, microscopy, or architectural walkthrough-style scenes.


## Arcball Contract

The current arcball object already reflects the intended model-space behavior:

1. it maintains an accumulated model matrix,
2. it exposes a computed model matrix,
3. it applies in-flight rotation during drag and commits on drag end.

Public contract recommendation:

1. panel arcball applies model transforms to attached visuals that opt into controller application,
2. visuals with `controller_mode = FIXED` do not receive that model transform,
3. arcball does not imply camera orbit.

The public vocabulary should say “rotate object/model”, not “orbit camera”, for the active mode.


## Panzoom Contract

Panzoom remains the right controller for 2D navigation.

Recommendation:

1. panzoom controls panel navigation in view/projection terms,
2. it is the default controller for domain-style 2D panels,
3. it should not be overloaded to simulate object-space 3D manipulation.

This keeps 2D and 3D interaction semantics easy to reason about.


## Controller Participation

The scene already has the important concept of whether a visual participates in controller motion.

Recommendation:

1. keep `controller_mode = APPLY` and `controller_mode = FIXED`,
2. use this consistently across mesh, image, text, and annotation visuals,
3. do not create family-specific hidden controller exceptions.

Typical examples:

1. mesh visual: `APPLY`
2. panel background: `FIXED`
3. screen-space annotation label: `FIXED`
4. world-space 3D label attached to an object: `APPLY`


## 2D and 3D Placement

The controller/transform model must support both 2D and 3D placement semantics.

Recommended distinction:

1. screen-space placement
   - panel/figure-relative
   - unaffected by model-space arcball
2. world/object-space placement
   - transformed through model/view/projection
   - participates in controller motion when attached accordingly

This distinction will matter especially for:

1. text,
2. scale bars,
3. dimensions,
4. bounding-box annotations,
5. mixed scenes with overlays and 3D content.


## World-Space Text and Annotation Implications

World-space labels and dimensions should be able to move with the object/model transform.

Recommendation:

1. world-space text anchored to an object uses the same model-space controller path as the object,
2. screen-space overlays stay fixed,
3. the API should expose placement mode clearly instead of hiding it in one transform setter.

This is one reason the transform model must stay explicit now.


## Fixed Camera Policy

For the current active branch direction, the fixed camera policy should be treated as deliberate,
not incidental.

Recommended behavior:

1. panels start with a fixed camera state suitable for the view,
2. model-space arcball manipulates objects,
3. camera manipulation is introduced through explicit camera-controller modes when needed.

This avoids mixed semantics where the same controller sometimes rotates the object and sometimes the
camera with no explicit contract.


## Future Camera-Orbit Mode

A camera-orbit controller may still be useful later, but it should be a distinct mode.

Recommended rule:

1. do not overload the current `arcball` concept to mean both model rotation and camera orbit,
2. if camera orbit is added later, make it an explicit camera controller choice.


## Public Controller Modes

The public model should eventually be able to express at least these distinct controller choices:

1. `panzoom_2d`
2. `arcball_model`
3. `camera_orbit`
4. `camera_fly`
5. `camera_fps`

The exact names can still move, but the semantic split should remain explicit.


## Picking Interaction

Picking needs consistent transform semantics.

Recommendation:

1. picking should resolve identities in the same transformed scene that the user sees,
2. model-space arcball affects pickable object positions because it changes model transforms,
3. screen-space overlays use screen-space picking semantics instead.

This keeps picking coherent across mesh, text, and annotations.


## Measurement and Annotation Interaction

Measurement overlays need explicit transform behavior.

Recommended categories:

1. screen-space measurement overlays
   - fixed to panel
   - not affected by arcball
2. world-space measurement overlays
   - anchored to world/object geometry
   - transformed with the object when appropriate

Examples:

1. adaptive scale bar: usually screen-space/fixed
2. 3D bounding box: world-space/apply
3. 3D dimension labels: world-space/apply


## API Direction

The existing controller API shape in `scene.h` is broadly the right one:

1. attach controller to panel,
2. visuals declare whether they participate,
3. controller state feeds panel-level uniform data and visual transforms.

The next scene-facing API work should focus on:

1. visual/object transform setters,
2. clear placement modes for text/annotations,
3. explicit camera helpers and target/pose setters,
4. controller selection per panel,
5. keeping controller semantics stable across visual families.


## Relationship To UBOs And Runtime

The implemented controller infrastructure already uses panel-level MVP/viewport data.

Recommendation:

1. keep panel-level view/proj state in panel-scoped uniform data,
2. treat per-visual model transforms as visual-scoped data or derived state layered on that panel
   transform contract,
3. do not move back toward ad hoc geometry rewriting for interaction.


## Initial Public Rules To Keep Stable

The following rules should be treated as active contract:

1. geometry data is local/object-space,
2. arcball rotates the object/model, not the camera,
3. panels own camera/view/projection state,
4. camera-orbit, fly, and FPS modes are separate camera-controller choices,
5. `controller_mode` decides whether a visual participates in panel navigation,
6. screen-space and world-space placement are distinct concepts.


## Explicit Non-Goals For The Current Slice

1. unify model arcball and camera orbit under one ambiguous controller API,
2. rewrite geometry for interactive transforms,
3. hide overlay versus world-space placement distinctions,
4. collapse all camera semantics into one generic “3D controller” with fuzzy behavior.
