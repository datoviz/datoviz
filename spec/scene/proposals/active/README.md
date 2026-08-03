# Active Scene Proposals

These files are current proposal-stage design addenda. They still carry staging authority when a
specialized spec has not absorbed the topic, but specialized files under `spec/scene/` remain the
primary implementation-facing source when they exist.

Promoted or partially absorbed records live in [`../promoted/`](../promoted/). Future or
non-v0.4-blocking roadmaps live in [`../future/`](../future/).


## Interaction, Events, And UI

1. [ASYNC_CALLBACKS.md](ASYNC_CALLBACKS.md): async callback and host-post policy not yet absorbed
   into `EVENT_CALLBACKS.md` and `THREAD_SAFETY.md`.
2. [CONTROLLER_INSPECTORS_AND_GIZMOS.md](CONTROLLER_INSPECTORS_AND_GIZMOS.md): orientation gizmo,
   controller widget, and inspector pressure still split across API, interaction, geometry, and UI
   specs.
3. [SCREEN_SPACE_OVERLAY_LAYOUT.md](SCREEN_SPACE_OVERLAY_LAYOUT.md): overlay layout pressure not
   yet fully absorbed by annotation, panel reserve, and external UI specs.
4. [THEMING_STYLING_SYSTEM.md](THEMING_STYLING_SYSTEM.md): theme/style token plan not yet
   canonicalized.


## Resources, Transforms, Validation, And API Boundaries

1. [ASSET_BOUNDARY_DESIGN.md](ASSET_BOUNDARY_DESIGN.md): asset/source/cache boundary still
   influencing resource and data policy.
2. [CAPABILITY_FALLBACK_DESIGN.md](CAPABILITY_FALLBACK_DESIGN.md): fallback knobs and diagnostics
   still need final promotion into validation/adaptation specs.
3. [SCIENTIFIC_COORDINATE_NORMALIZATION.md](SCIENTIFIC_COORDINATE_NORMALIZATION.md): shared
   normalization-frame and coordinate readback gaps still need final promotion.
4. [UNITS_AND_TIME_FORMAT_API.md](UNITS_AND_TIME_FORMAT_API.md): shared unit ladders, duration
   formatting, datetime axes, and scale-bar/axis unit API.
5. [POLYGON_PSLG_API_DESIGN.md](POLYGON_PSLG_API_DESIGN.md): polygon/PSLG API remains unsettled.
6. [VISUAL_COMMAND_STREAM.md](VISUAL_COMMAND_STREAM.md): visual command stream remains active
   design pressure.
7. [VISUAL_LOCAL_TRANSFORM_AND_ARCBALL_TARGET.md](VISUAL_LOCAL_TRANSFORM_AND_ARCBALL_TARGET.md):
   retained visual-local transform and arcball model/camera target semantics for the textured
   planet cleanup.
8. [ARBITRARY_CLIP_MASKS.md](ARBITRARY_CLIP_MASKS.md): post-RC1 retained polygon clip masks,
   stencil lowering, query parity, and backend-neutral scene API direction.


## Visual And Render-Mode Pressure

1. [BATCHED_TEXT_API_REFACTOR.md](BATCHED_TEXT_API_REFACTOR.md): pre-RC public text API refactor
   to make retained semantic text batched by default.
2. [MATERIAL_LIGHTING_API.md](MATERIAL_LIGHTING_API.md): material/light API still has unresolved
   public shape and ownership questions.
3. [PARTICLE_SYSTEM_DESIGN.md](PARTICLE_SYSTEM_DESIGN.md): particle-system design remains
   exploratory pressure on visuals, resources, and frame planning.
4. [DEPTH_OF_FIELD_POSTPROCESS.md](DEPTH_OF_FIELD_POSTPROCESS.md): DoF remains active
   postprocess/showcase design pressure.
