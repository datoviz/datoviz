# Scene And DRP2 Rules

These rules cover the active v0.4 scene -> DRP2 -> runtime path.

## Active Vertical Slice

`drp2`, `scene`, and `app` are active v0.4 modules, not future scaffolding.

The current vertical slice is:

```text
scene/frame-plan emission -> DRP2 command stream -> vklite runtime ->
canvas/stream frame execution -> app presentation
```

Scene should emit frame plans and DRP2 streams. The native runtime should execute through `vklite` and borrowed canvas frames without scene owning swapchains, command-buffer lifecycle, or sinks.

## Resource And Render Contracts

Upload nodes create or update resources only. They must not select render behavior.

Render nodes must emit through typed visual metadata, visual descriptors, and draw contracts. Texture-backed visuals such as image, glyph, labels, volume, and textured mesh still use the same visual registry path as buffer-backed visuals. Do not add runtime shortcuts based on resource names, upload order, texture presence, or the fact that an upload created a texture.

Backends consume the same DRP2 command streams. Native/WebGPU gaps should surface as capability diagnostics or unsupported-feature errors, not as scene-lowering forks or parallel texture render paths.

## Scene Contracts

Use the [scene index](../../spec/scene/README.md) to locate the affected contract and the [visual-family index](../../spec/scene/visuals/README.md) for visual coverage and family specifications.

## Visual Family Boundaries

When adding or changing scene visuals, isolate family-specific behavior behind visual descriptors or lowering helpers.

Generic render-emission, visual descriptor, and pipeline plumbing must consume normalized facts instead of adding concrete-family checks.

Do not add branches like these to generic visual, render-emission, or pipeline plumbing files:

```c
if (is_<family>)
if (<family>)
visual_type == DVZ_VISUAL_TYPE_<FAMILY>
```

If a family needs behavior that the generic path cannot express, extend the normalized descriptor/lowering interface first and add focused tests for that reusable concept.

## Shader And ABI Work

Read the contract affected by the change:

| Change | Context |
| --- | --- |
| Shader registry, binding layout, or shader ABI | [Visual/shader architecture](../../spec/scene/implementation/VISUAL_SHADER_REFACTOR.md) |
| Visual-family ownership or generic lowering boundaries | [Boundary contract](../../spec/scene/visuals/BOUNDARY_CONTRACT.md) |
| Family-specific rendering behavior | The affected family specification from the [visual index](../../spec/scene/visuals/README.md) and its section in [implementation decisions](../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md) |

Run `just shader-abi-check` whenever changing:

1. `src/scene/glsl`
2. `src/scene/wgsl`
3. Shader registry entries
4. Visual pipeline bind/layout rules
5. Visual shader ABI documentation

## DRP2 Specs

Use the [DRP2 index](../../spec/drp2/README.md) to find the affected contract. Read [commands](../../spec/drp2/COMMANDS.md) for command semantics, [lifetimes](../../spec/drp2/LIFETIMES.md) for object and encoder/pass state, [errors](../../spec/drp2/ERRORS.md) for validation behavior, or the [fixture runner contract](../../spec/drp2/fixtures/RUNNER.md) for conformance tooling. Read [authority](../../spec/drp2/AUTHORITY.md) when changing contracts or resolving prose/schema/fixture disagreement. The full [reading order](../../spec/drp2/READING_ORDER.md) is for comprehensive protocol reviews.

## Request And Query Paths

Before changing pick/probe/query execution, GPU request readback, visual-family query policy, or CPU fallback behavior, read:

1. [../../spec/scene/interaction/GPU_QUERY_SYSTEM.md](../../spec/scene/interaction/GPU_QUERY_SYSTEM.md)
2. [../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md](../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)

Before changing sampled-field format/semantic interpretation, categorical colorizers, label-volume support, or sampled visual query schemas, read:

1. [../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md](../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md)
