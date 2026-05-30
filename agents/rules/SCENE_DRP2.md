# Scene And DRP2 Rules

These rules cover the active v0.4 scene -> DRP2 -> runtime path.


## Active Vertical Slice

`drp2`, `scene`, and `app` are active v0.4 modules, not future scaffolding.

The current vertical slice is:

```text
scene/frame-plan emission -> DRP2 command stream -> vklite runtime ->
canvas/stream frame execution -> app presentation
```

Scene should emit frame plans and DRP2 streams. The native runtime should execute through `vklite`
and borrowed canvas frames without scene owning swapchains, command-buffer lifecycle, or sinks.


## Current Scene Coverage

Built-in scene visuals include point, pixel, marker, primitive, mesh, path/segment, image, volume,
and sphere impostors.

Scene support also covers retained sampled fields, image colormap scale binding, colorbar
bookkeeping, panzoom/arcball/fly/turntable controllers, narrow text/annotation bookkeeping,
GPU-backed point pick and image probe request paths, and graph-backed panel techniques.

The active scene slice covers retained visual rendering, repeated partial updates, multi-panel
figures, per-panel runtime viewport/scissor handling, depth-enabled 2D/3D passes, request
readbacks, descriptor refresh after stable resource recreation, and graph-backed postprocess,
transparency, and MSAA techniques.


## Visual Family Boundaries

When adding or changing scene visuals, isolate family-specific behavior behind visual descriptors
or lowering helpers.

Generic render-emission, visual descriptor, and pipeline plumbing must consume normalized facts
instead of adding concrete-family checks.

Do not add branches like these to generic visual, render-emission, or pipeline plumbing files:

```c
if (is_<family>)
if (<family>)
visual_type == DVZ_VISUAL_TYPE_<FAMILY>
```

If a family needs behavior that the generic path cannot express, extend the normalized
descriptor/lowering interface first and add focused tests for that reusable concept.


## Shader And ABI Work

For scene visual/shader work, read:

1. [../../spec/scene/implementation/VISUAL_SHADER_REFACTOR.md](../../spec/scene/implementation/VISUAL_SHADER_REFACTOR.md)
2. [../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md](../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md)
3. [../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md](../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)

Run `just shader-abi-check` whenever changing:

1. `src/scene/glsl`
2. `src/scene/wgsl`
3. Shader registry entries
4. Visual pipeline bind/layout rules
5. Visual shader ABI documentation


## DRP2 Specs

Before touching `spec/drp2/`, `src/drp2/`, or DRP2-emitting scene code, read:

1. [../../spec/drp2/README.md](../../spec/drp2/README.md)
2. [../../spec/drp2/AGENT_SPEC_PHASE.md](../../spec/drp2/AGENT_SPEC_PHASE.md)


## Request And Query Paths

Before changing pick/probe/query execution, GPU request readback, visual-family query policy, or CPU
fallback behavior, read:

1. [../../spec/scene/interaction/GPU_QUERY_SYSTEM.md](../../spec/scene/interaction/GPU_QUERY_SYSTEM.md)
2. [../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md](../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)

Before changing sampled-field format/semantic interpretation, categorical colorizers,
label-volume support, or sampled visual query schemas, read:

1. [../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md](../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md)
