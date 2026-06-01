# v0.4 Shader And Transform Future Compatibility

Status: active pre-release handoff. Created: 2026-06-01. Last updated: 2026-06-01.

Implementation update: v0.4 now has ABI-prologued inert descriptor shells for future visual
transform and shader extension points. `DvzVisualTransformDesc` and `DvzVisualShaderDesc` have
canonical initializers and setter entry points, but v0.4 accepts only `NONE`; nonlinear transforms,
custom visual families, and built-in shader replacement are rejected explicitly as deferred.

This note captures a narrow release-stabilization concern from the v0.3 visible parity review:
Datoviz v0.4 has most visible roadmap capabilities, but dynamic/customizable shaders and nonlinear
coordinate transforms are intentionally not v0.4-final deliverables.

The pre-release task is not to implement those features. It is to avoid freezing public API or scene
semantics in a shape that makes them hard to add after v0.4.


## Decision Direction

1. Do not ship a general render custom-shader API in v0.4 final.
2. Do not ship scene-managed nonlinear coordinate transforms in v0.4 final.
3. Do preserve extension boundaries and status labels before feature freeze.
4. Prefer explicit deferral over placeholder public APIs that do not work end to end.


## Custom Shader Compatibility

Preferred future path: custom shaders arrive through custom visual families, not by replacing the
shader inside built-in point, mesh, image, volume, or annotation visuals.

Before v0.4 final:

1. Mark general custom visual/render shaders as `deferred`. Done in the public feature-status docs.
2. Mark `DvzSceneCompute` as `advanced/unstable`, not as the general custom shader API. Done in
   the public feature-status docs.
3. Document built-in shader ABI as internal unless a specific entry point is explicitly exported.
   Done in `spec/scene/api/API_SURFACE.md`.
4. Keep DRP2 shader identity based on transport format, source/hash, and optional built-in
   family/variant/version metadata.
5. Avoid promising shader hot reload or built-in shader replacement.
6. Keep standard future custom-visual bindings named in spec: panel transform, viewport, item
   state, scale/colormap resources, sampled fields, and user uniforms.

Useful source documents:

1. [../../spec/scene/integration/CUSTOM_VISUALS.md](../../spec/scene/integration/CUSTOM_VISUALS.md)
2. [../../spec/scene/pipeline/FRAME_PLAN.md](../../spec/scene/pipeline/FRAME_PLAN.md)
3. [../../spec/drp2/COMMANDS.md](../../spec/drp2/COMMANDS.md)


## Nonlinear Transform Compatibility

Preferred future path:

```text
raw positions -> optional persistent derived projected buffer -> normal visual rendering
```

The v0.4 supported pattern can remain CPU-side projection by the user, followed by ordinary scene
normalization and rendering. A later scene-managed path should insert compute work only when source
positions or projection parameters change. Pan/zoom should update only panel transform state.

Before v0.4 final:

1. Document CPU pre-projection as the supported v0.4 user pattern. Done in public feature-status
   docs and `spec/scene/semantics/NONLINEAR_TRANSFORMS.md`.
2. Mark scene-managed nonlinear transforms/projections as `deferred`. Done in public feature-status
   docs and transform specs.
3. Keep the transform pipeline distinction explicit:
   `DataSpace -> VisualSpace -> PanelSpace -> ClipSpace`.
4. Preserve the invariant that DRP2 receives visual-ready resources plus panel transform state, not
   scientific coordinate semantics.
5. Do not expose a no-op projection API. Current policy: rely on append-only growable descriptors
   such as `DvzVisualAttachDesc`, and reject unknown flags in v0.4.
6. Decide how future transform/domain data will extend the public API without breaking v0.4 users.
   Current direction: append fields to growable descriptors or add a new growable descriptor rather
   than changing existing defaults.

Useful source documents:

1. [../../spec/scene/pipeline/TRANSFORM_PIPELINE.md](../../spec/scene/pipeline/TRANSFORM_PIPELINE.md)
2. [../../spec/scene/semantics/NONLINEAR_TRANSFORMS.md](../../spec/scene/semantics/NONLINEAR_TRANSFORMS.md)
3. [../../spec/scene/pipeline/RESOURCE_MODEL.md](../../spec/scene/pipeline/RESOURCE_MODEL.md)
4. [../../spec/scene/pipeline/INVALIDATION_AND_CACHING.md](../../spec/scene/pipeline/INVALIDATION_AND_CACHING.md)


## Pre-Release API Audit Points

Check these before feature freeze:

1. `DvzDataDomain` in `include/datoviz/scene/types.h` currently exposes `min/max`, while the
   transform spec discusses domain scale. Either keep domain scale out of the v0.4 public ABI and
   use future setters/handles, or explicitly classify the relevant API as unstable.
2. `DvzVisualAttachDesc` currently exposes `z_layer/controller_mode`, while the transform spec
   discusses future coordinate space, pre-normalization transform, and per-visual domain override
   semantics. Prefer future setter/handle APIs or reserved/size-versioned descriptors over adding
   incompatible fields later.
3. Public docs and examples should not imply that built-in visuals can accept arbitrary replacement
   shaders.
4. Public docs and examples should not imply that panel domains already support nonlinear spatial
   scales or geographic projections.
5. The feature/status table should classify these surfaces consistently as `deferred`,
   `advanced/unstable`, or supported CPU-side patterns.


## Suggested Next Agent Action

During public API/status cleanup, add explicit rows for:

1. CPU-side user projection before upload: `supported pattern`.
2. Scene-managed nonlinear coordinate transforms: `deferred`.
3. Custom visual/render shaders: `deferred`.
4. Scene compute shaders: `advanced/unstable`.
5. Built-in shader replacement/hot reload: `deferred`.

Then reconcile any wording in public docs, examples, and release notes with those rows.
