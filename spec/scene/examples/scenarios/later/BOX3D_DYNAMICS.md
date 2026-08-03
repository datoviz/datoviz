# Box3D Dynamics Showcase

> **Example status:** informative later-stage pressure test
> **Target:** optional standalone C showcase
> **Data:** deterministic generated simulation
> **Validation:** bounded smoke, screenshot, interaction checklist, and timing counters
> **Agent copy-safe:** no
> **Role:** showcase


## Summary

`showcases_box3d_dynamics` should present Datoviz as an interactive visualization and diagnostics frontend for a Box3D rigid-body simulation. The first useful result is a bounded container filled with dynamic boxes, rendered from one retained cube mesh with one transform per body. The integration remains example-local: Box3D owns simulation state, Datoviz owns presentation state, and neither library gains a generic physics abstraction.

Box3D is a young external API; the implementation should pin and record the tested release or commit rather than silently tracking its default branch. Use the upstream [Box3D documentation](https://box2d.org/documentation3d/) and [repository](https://github.com/erincatto/box3d) as the API sources of truth.


## User-Visible Result

The scene contains 500 to 2,000 identical boxes falling inside a floor-and-wall enclosure, with an arcball camera and controls for pause, resume, reset, and one fixed simulation step. A compact overlay reports body count, awake-body count, contact or hit activity, Box3D step time, and Datoviz frame time. Render the enclosure as a solid floor plus an unobtrusive frame for the first slice; translucent walls are optional polish and must not obscure contact motion.


## Ownership Boundary

| Owner | Responsibilities |
| --- | --- |
| Box3D | World, bodies, shapes, collision detection, contact resolution, sleeping, transforms, simulation step, queries, and physics diagnostics. |
| Datoviz | Cube geometry, instanced mesh state, enclosure visuals, camera and interaction, GUI, diagnostic glyphs, timing display, and capture. |
| Example adapter | Stable body-to-instance mapping, coordinate conversion, fixed-step accumulator, transform staging, and optional contact-glyph extraction. |

The adapter is private to the example. Do not introduce an entity-component layer, a public physics bridge, a second renderer/runtime path, or ownership of Box3D handles inside Datoviz.


## Integration Contract

Create one `dvz_mesh()` visual from one cube geometry and populate its `instance_transform` attribute with one column-major `mat4` per dynamic body. Keep stable example-owned records containing the opaque `b3BodyId`, the Datoviz instance index, the box half-extents, and the previous/current physics transforms needed by the selected render-sampling policy.

After each `b3World_Step()`, consume transient `b3World_GetBodyEvents()` data immediately and update the CPU transform array through each event's `userData` mapping. Do not retain pointers into Box3D event arrays. The baseline may upload the complete `instance_transform` array once per rendered frame; a measured optimization may coalesce changed instance indices into `dvz_visual_set_data_range()` calls when sparse awake-body updates make that faster.

Keep position, quaternion, scale, handedness, and matrix-layout conversion in one pure helper. Box3D exposes quaternion vector components followed by the scalar component; the adapter must test that ordering, the body-origin convention, and Datoviz's column-major transform convention explicitly. Apply shape-local offsets if a body's visual origin differs from its center of mass.

The current Datoviz mesh path has retained `instance_transform` support but no general public per-instance color attribute. Therefore the minimal slice uses one material color. Speed coloring is a follow-up pressure point: add it only through an approved per-instance mesh styling contract, or use a small measured set of speed buckets without reverting to one draw per body.


## Timing And Frame Policy

Advance Box3D at a fixed physics rate, initially 60 Hz with the upstream-recommended four substeps, while the Datoviz host continues to process input and render independently. Cap accumulated catch-up work to prevent a stalled debugger or dragged window from causing an unbounded simulation spiral, and report dropped simulation time when the cap is reached.

The deterministic diagnostic mode renders the latest completed solver state with no interpolation. A later presentation mode may interpolate between previous and current transforms, but it must not feed interpolated state back into Box3D or label interpolated poses as solver output. Pause and single-step operate on the physics clock; camera interaction remains live while paused.


## Diagnostics

Use `b3World_GetAwakeBodyCount()`, Box3D counters/profile data, and application timers for the initial overlay. Contact visualization is a separate, opt-in stage because Box3D contact events are transient and event enablement has overhead.

For impact diagnostics, consume hit events immediately after the step and render bounded marker/segment buffers for event points and normals. For a persistent touching-contact view, retrieve contact data deliberately and deduplicate shape pairs or manifold points; do not mistake begin/end/hit event counts for the number of currently touching contacts. Resize dynamic diagnostic visuals in batches rather than creating a Datoviz object per contact.


## Minimal Target

1. Add Box3D as an optional example-only CMake dependency, disabled in the default build and absent from `libdatoviz`.
2. Create 100 deterministic boxes, a static floor, and a fixed-step Box3D world.
3. Render all bodies through one retained cube mesh and `instance_transform` array.
4. Implement reset, pause/resume, single-step, an arcball camera, and body/awake/step-time counters.
5. Prove correct translation, quaternion conversion, non-uniform box scale, sleeping updates, reset lifetime, and clean shutdown.


## Follow-Up Stages

1. Scale through fixed 100, 1,000, and 10,000-body profiles; record Box3D step time, transform staging time, upload time, frame time, awake bodies, and draw count.
2. Add speed or sleep-state styling after choosing a supported per-instance styling path.
3. Add bounded contact/hit markers and normals, then optional impulse-scaled glyphs and historical plots.
4. Add Datoviz camera-ray generation, `b3World_CastRayClosest()` selection, and optional dragging through a Box3D constraint or kinematic target.

Joints, articulated bodies, arbitrary convex-mesh rendering, Python bindings, GPU physics synchronization, and a reusable bridge API remain outside this scenario.


## Validation

1. Build succeeds both with the optional dependency disabled and with the pinned Box3D dependency enabled.
2. A fixed seed and fixed step produce the same bounded-frame body transforms across repeated runs on the same supported platform and Box3D build configuration.
3. A screenshot shows correctly translated, rotated, and scaled boxes inside the enclosure with no per-body draw path.
4. Pause freezes physics while camera input remains responsive; single-step advances exactly one physics tick; reset restores the deterministic initial state without leaking either library's objects.
5. The 1,000-body profile performs no per-frame heap allocation in the transform synchronization path and reports separate physics, staging/upload, and frame timings.
6. Contact overlays, when enabled, stay within configured capacity and expose truncation instead of overrunning buffers or silently dropping diagnostics.


## Related Contracts

- [Mesh visual](../../../visuals/MESH.md) defines the retained `instance_transform` path.
- [Example policies](../../POLICIES.md) define optional data/dependency and shared example rules.
- [Picking](../../../interaction/PICKING.md) owns the Datoviz query boundary for the later selection stage.
- [`selection_mesh_instances.c`](../../../../../examples/c/features/selection_mesh_instances.c) is the current runnable proof for retained cube instancing, arcball interaction, and per-instance queries.


## Open Questions

1. Should the first implementation pin a Box3D release through `find_package()`, `FetchContent`, or a separately installed developer dependency?
2. Is a public per-instance mesh color attribute warranted independently of this showcase, or are a few shared-material speed buckets sufficient?
3. Should deterministic capture use exact solver poses only, with interpolation restricted to interactive presentation?
