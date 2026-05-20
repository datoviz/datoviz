> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve remaining transform/controller staging decisions after controller
>   routing, camera families, and binding rules moved into canonical specs.

# Transform and Controller Design

## Decision Addressed

The v0.4 scene model separates object transforms, panel camera state, controller routing, and
overlay placement.

The remaining proposal-stage decision is to keep model-space arcball as the active default while
camera orbit, fly, and FPS behavior remain explicit camera-controller families.


## Short Summary

The active model is:

1. geometry stores local/object-space data;
2. visuals own object/model transforms;
3. panels own view/projection/camera state;
4. model-space arcball mutates participating visual/object transforms while the camera remains
   fixed;
5. panzoom mutates 2D panel navigation;
6. camera controllers are distinct from model-space arcball;
7. screen-space overlays and world/object-space annotations declare controller participation
   explicitly.


## Chosen Direction

| Topic | Direction |
|---|---|
| Transform layers | Keep local/object, model, view, projection, and viewport transforms distinct. |
| Geometry | Generation/import never bakes controller state into vertex data. |
| Visuals | Mesh, world-space text, and world-space annotations own or reference model transforms. |
| Panels | Panels own camera/view/projection and panel-local navigation state. |
| Arcball | The active `arcball` mode rotates object/model state, not the camera. |
| Panzoom | 2D navigation remains view/domain-oriented and should not simulate 3D manipulation. |
| Controller participation | Use `APPLY` versus `FIXED` consistently across visual families. |
| Placement | Screen-space placement is fixed to panel/figure coordinates; world/object-space placement follows scene transforms when opted in. |


## Controller Families To Keep Distinct

1. `panzoom_2d` for 2D panel navigation;
2. `arcball_model` for object/model rotation;
3. `camera_orbit` for camera motion around an explicit target;
4. `camera_fly` for free 3D camera navigation;
5. `camera_fps` for yaw/pitch movement with application-defined ground or lift constraints.

Exact public names can change, but one controller name must not ambiguously mean both model
rotation and camera orbit.


## Canonical Migration Links

The authoritative rules now live in:

1. [Controllers And Interaction](../../interaction/CONTROLLERS.md) for routing, state, capture,
   invalidation, controller families, and diagnostics;
2. [Camera Controllers](../../interaction/CAMERA_CONTROLLERS.md) for orbit/fly/turntable camera
   behavior;
3. [Controller Binding Model](../../decisions/CONTROLLER_BINDING_MODEL.md) for scene-owned
   controller handles and panel binding/linking;
4. [Transform Pipeline](../../pipeline/TRANSFORM_PIPELINE.md) for matrix ownership and upload
   behavior;
5. [Picking](../../interaction/PICKING.md), [Selection](../../interaction/SELECTION.md), and
   [Annotations](../../semantics/ANNOTATIONS.md) for interaction consumers.

Do not duplicate routing, invalidation, or camera-family math here.


## Remaining Unresolved Points

1. Final public C names for visual/object transform setters and transform descriptors.
2. How model-space arcball targets one visual, a panel-attached set, or a future selection group.
3. Public target/pose helpers for camera-orbit, fly, and FPS controllers.
4. Pivot picking and whether it updates arcball center, camera target, or both depending on mode.
5. Serialization shape for controller state snapshots and visual transform state.
6. Exact rules for mixed fixed overlays, world-space labels, and measurement annotations during
   linked-panel navigation.
