> **Execution Status**
> - **Status:** `FUTURE SCENE PRESENTATION PROPOSAL`
> - **Updated on:** `2026-08-03`
> - **Purpose:** define visual-wide and item-state modifier pressure without duplicating application-state, material, selection, or GUI ownership.
> - **Baseline reviewed:** `73dc1ae0128b9897806fad470bd05c1143d7a316`

# Visual Modifiers

## Decision Addressed

Interactive applications need cheap, nondestructive presentation changes such as opacity, size scale, width scale, and tint without rewriting dense base attributes. Datoviz already has selected, unselected, and hovered item-state styles; this proposal asks whether the same semantics should extend to a visual-wide modifier and become stable targets for future reactive bindings.

This is exploratory v0.5+ work. It does not change the implemented v0.4 selection, hover, material, shader ABI, or visual-family contracts.

## Existing Authority

Current selection and hover presentation is owned by `DvzItemStateVisualStyle`, `DvzSelectionVisualStyle`, `dvz_selection_set_visual_style()`, and `dvz_hover_set_visual_style()`. The implementation lowers supported item-state styles through the existing item-state resource and shader path.

The canonical current behavior remains in [`../../interaction/SELECTION_VISUAL_STYLE.md`](../../interaction/SELECTION_VISUAL_STYLE.md), installed headers, shader ABI checks, and visual-family implementations. This proposal must not silently replace those objects, change default rendering, or reinterpret their flags.

Material and lighting state remains semantically distinct. A modifier changes presentation of authored attributes; it is not a material definition, transfer function, dense selection membership resource, or general style stack.

## Proposed Semantic Model

Evaluate one compact modifier vocabulary that may be applied at different scopes:

| Scope | Meaning |
| --- | --- |
| visual | apply to every supported item after interpreting base attributes. |
| selected | apply to selected items while selection presentation is active. |
| unselected | apply to non-selected items while selection presentation is active. |
| hovered | apply to the current hovered item after selection presentation. |

The scope is part of the target, not a reason to create several unrelated modifier record types. Existing item-state style structs may remain the public representation if extending them produces a cleaner API than introducing `DvzModifier`.

The first vocabulary should be limited to operations with clear semantics and existing implementation pressure:

1. opacity multiplier;
2. point-like size multiplier;
3. line-like width multiplier;
4. tint color and mix amount.

Brightness, contrast, gamma, saturation, exposure, outlines, halos, pulses, depth bias, and arbitrary named style stacks are deferred until their color-space, composition, capability, and rendering semantics are specified.

## Composition

The proposed conceptual order is:

```text
base item attributes
    -> visual-wide modifier
    -> selected or unselected modifier
    -> hovered modifier
    -> material and lighting
    -> depth cue and scene effects
    -> output encoding
```

Geometry modifiers such as size or width may execute in a vertex or geometry path. Color modifiers must state whether tinting occurs in authored, semantic, or linear render space and must remain compatible with the normative color-management contract.

Composition must be deterministic and tested when several scopes are active. A neutral modifier must preserve current output exactly.

## Capability And Failure Rules

Each visual family must report or validate which modifier fields it supports. Unsupported combinations such as width scaling on a volume or point-size scaling on an image must return an explicit error or diagnostic rather than being ignored.

Coverage advances one visual-family group at a time. Point-like families are the natural first pressure test because their selection and hover shaders already apply alpha, tint, and scale. Line-like width scaling is a separate milestone. Image, volume, mesh, text, and material-sensitive families require their own semantic review.

A visual-wide modifier that changes effective geometry must participate in CPU bounds, picking, culling, and export behavior where those systems depend on the modified extent. It cannot be treated as a shader-only decoration when that would make scene behavior inconsistent.

## Resource And Shader Boundary

The implementation should reuse or deliberately evolve the common item-state style path rather than create a parallel descriptor architecture. Any shader ABI change must preserve explicit set ownership, Vulkan and WebGPU parity policy, neutral resources, and existing ABI validation.

A shared immutable neutral resource with copy-on-write materialization is a possible optimization, not a semantic requirement. Allocation and descriptor strategy should follow measurement after the public and shader representations are settled.

Modifier updates should invalidate only visual properties and the smallest parameter upload needed for the next frame. They must not dirty or reupload dense position, color, size, width, membership, or topology resources unless the modifier changes a CPU-derived contract that requires recomputation.

## Reactive Integration

The cross-cutting reactive proposal is [`../../../architecture/REACTIVE_APPLICATION_STATE.md`](../../../architecture/REACTIVE_APPLICATION_STATE.md). A supported modifier field may later expose a typed scene endpoint, but the modifier remains scene-owned presentation state.

An application-owned property may drive a modifier through a one-way binding. A GUI inspector may edit a scene-owned modifier endpoint directly. Creating both a property and a modifier value as peer authorities is prohibited by the single-authority rule.

String paths such as `modifier.opacity` or `selected.size_scale` are useful examples, not approved public schema. Endpoint naming and versioning must be decided with the reactive endpoint representation.

## Prototype Sequence

1. Audit current selection and hover public structs, visual-family support, shader ABI, bounds, picking, and invalidation behavior.
2. Decide whether visual-wide presentation extends `DvzItemStateVisualStyle`, introduces a renamed common style, or requires a distinct record with explicit conversions.
3. Implement one private point-like visual-wide opacity or size-scale prototype with neutral-output parity and no dense upload.
4. Prove deterministic visual, selection, and hover composition in CPU payload and rendering tests.
5. Prove capability errors, target destruction, bounds/picking consistency, Vulkan shader ABI, and applicable WebGPU behavior.
6. Only then expose one modifier endpoint to the reactive prototype and evaluate public naming.

## Acceptance Criteria

A point visual with millions of retained base positions, colors, and sizes can change a supported visual-wide or selected size factor continuously without uploading the dense size attribute. Selection and hover composition remains deterministic, neutral settings preserve current rendering, CPU-visible bounds remain correct, unsupported visual families fail explicitly, and a future reactive binding can target the field without bypassing scene ownership or invalidation.

## Non-Goals

This proposal does not define arbitrary subset styles, theme inheritance, material parameters, transfer functions, dense selection membership, expression-driven shader values, general shader reflection, or a backend-specific parameter escape hatch.

It does not make reactive properties authoritative for scene-owned selection, hover, visual, or material state.

## Open Decisions

1. Extend the existing item-state style representation or introduce a common modifier representation.
2. Public typed setters versus a convenience aggregate style setter.
3. Exact color space and alpha semantics for tint composition.
4. CPU bounds and picking policy for every geometry-affecting modifier.
5. Initial visual-family coverage and capability-query shape.
6. Shared neutral resource versus per-visual allocation after measurement.
7. Stable endpoint identifiers and schema versioning if reactive integration is promoted.
