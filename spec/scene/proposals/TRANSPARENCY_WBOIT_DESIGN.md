> **Execution Status**
> - **Status:** `MOSTLY PROMOTED`
> - **Updated on:** `2026-05-16`
> - **Purpose:** preserve WBOIT design rationale and remaining follow-up work after promotion into
>   specialized transparency specs.

# Transparency and WBOIT Design

This note records the intended transparency contract for the active v0.4 scene stack.


## Authority Note

Active transparency rules now live primarily in
[`../semantics/TRANSPARENCY.md`](../semantics/TRANSPARENCY.md). Frame-plan ordering belongs in
[`../pipeline/FRAME_PLAN.md`](../pipeline/FRAME_PLAN.md), capability adaptation belongs in
[`../validation/ADAPTATION.md`](../validation/ADAPTATION.md), and runtime-boundary ownership belongs
in [`../core/RUNTIME_BOUNDARY.md`](../core/RUNTIME_BOUNDARY.md).

This proposal remains a rationale and follow-up note for WBOIT attachment/resolve design. If it
disagrees with the specialized specs, update the specialized specs first and keep only a concise
backlog note here.


## Objective

Support transparent visuals as a first-class scene feature. Ordinary source-over blending,
weighted blended order-independent transparency (WBOIT), and depth peeling are distinct alpha modes.
WBOIT is the active first OIT slice.

This note is about:

1. public scene-facing transparency modes,
2. scene/frame-plan representation,
3. DRP2/runtime implications,
4. attachment and resolve structure,
5. interaction with picking and capability checks.


## Why This Is Active Now

Transparency is a hard architectural pressure point for the active branch.

It matters immediately for:

1. mesh transparency,
2. the intended brain-shell example,
3. multi-pass frame-plan design,
4. runtime attachment ownership,
5. capability-aware adaptation.

The first WBOIT path is now active enough to validate alpha-mode planning, DRP2 emission, and
runtime execution. Remaining work should broaden coverage without changing the public alpha-mode
meaning.


## Existing Material In The Repo

There is already useful grounding material in the tree:

1. scene-level transparency spec in
   [spec/scene/semantics/TRANSPARENCY.md](../semantics/TRANSPARENCY.md)
2. DRP2 capability notes in
   [spec/drp2/CAPABILITIES.md](../../drp2/CAPABILITIES.md)
3. low-level WBOIT technique coverage in
   [src/vklite/tests/test_techniques.c](../../../src/vklite/tests/test_techniques.c)
4. existing test shaders:
   [wboit_accum.frag](../../../src/vklite/tests/shaders/wboit_accum.frag),
   [wboit_comp.frag](../../../src/vklite/tests/shaders/wboit_comp.frag)

This note narrows those ideas into the active implementation direction.


## Public Transparency Model

Transparency should not be represented only as “alpha less than one”.

Recommended split:

1. material opacity remains material data,
2. transparency path is a visual alpha mode,
3. scene/frame-plan decides pass structure from that visual mode.

Installed visual alpha modes:

1. `DVZ_ALPHA_OPAQUE`
2. `DVZ_ALPHA_BLENDED` — ordinary source-over alpha blending
3. `DVZ_ALPHA_WBOIT` — weighted blended OIT
4. `DVZ_ALPHA_DEPTH_PEEL` — depth peeling
5. `DVZ_ALPHA_MASK`

Deferred:

1. exact OIT / per-pixel linked list mode
2. implicit fallback between OIT modes

Recommendation:

1. keep ordinary source-over alpha blending explicit as `DVZ_ALPHA_BLENDED`,
2. use `DVZ_ALPHA_WBOIT` for the active weighted-OIT path,
3. use `DVZ_ALPHA_DEPTH_PEEL` for the current explicit higher-quality OIT path,
4. do not silently downgrade a visual that asks for WBOIT when capability is missing.


## Why WBOIT First

Weighted blended OIT is the right first transparency path because:

1. it avoids CPU-side per-frame sorting,
2. it is far simpler than exact per-pixel linked lists,
3. it is already consistent with the intended scientific “many translucent surfaces/regions”
   workflows,
4. the repo already has low-level coverage for the underlying technique.

This is the best practical middle ground between quality, complexity, and implementation speed.


## Visual-Level Contract

A transparent visual should carry:

1. alpha mode = `DVZ_ALPHA_BLENDED`, `DVZ_ALPHA_WBOIT`, or `DVZ_ALPHA_DEPTH_PEEL`
2. opacity/material alpha state
3. any future transparency-specific material controls

The visual should not need to know:

1. exact attachment allocation,
2. exact resolve/composite pass wiring,
3. backend-specific blend setup.

Those belong below the visual API.


## Frame-Plan Contract

The frame plan should make transparency explicit as multiple passes, not one opaque pass with
hidden blending.

Recommended panel-level pass ordering:

1. opaque pass
2. source-over transparent pass when `DVZ_ALPHA_BLENDED` visuals are present
3. WBOIT accumulation pass when `DVZ_ALPHA_WBOIT` visuals are present
4. WBOIT composite/resolve pass
5. depth-peeling passes when `DVZ_ALPHA_DEPTH_PEEL` visuals are present

Rules:

1. opaque visuals render before transparent visuals,
2. no opaque visual renders after the transparent accumulation stage in the same panel,
3. panels without transparent visuals should not incur transparent accumulation/resolve passes.

This keeps transparency visible in planning and testable at the scene/DRP2 boundary.


## Attachment Model

The WBOIT path should reserve explicit accumulation targets.

Recommended first-pass targets:

1. `accum_color`
   - format class: floating-point color target, likely RGBA16F
2. `accum_weight` or reveal/transmittance target
   - format class: single-channel floating-point target, likely R16F
3. shared depth target with opaque pass as appropriate

The exact formula and naming can still move, but the public scene/runtime architecture should
assume:

1. extra transparent accumulation targets exist,
2. they are not the same as the final panel color target,
3. a composite pass resolves them to the visible output.


## Resolve / Composite Pass

The composite pass should be explicit in the frame plan and DRP2 emission.

Responsibilities:

1. sample accumulation targets,
2. combine them with the opaque background result,
3. write the final resolved color to the panel output target.

This is important because:

1. it makes attachment ownership explicit,
2. it gives a stable insertion point for later transparency-related overlays if needed,
3. it avoids hiding significant multi-pass behavior in the transparent material path.


## DRP2 Requirements

The active DRP2/runtime lane should support WBOIT through the existing explicit command surface,
not through scene-private shortcuts.

WBOIT is represented explicitly in emitted command streams. It pressures DRP2 in the following
areas:

1. multiple color attachments,
2. per-attachment blend state,
3. depth attachment coordination,
4. pass sequencing,
5. texture sampling of intermediate attachments in the composite pass.

Recommendation:

1. keep WBOIT representation explicit in emitted command streams,
2. add or align fixtures around multi-pass transparent accumulation and resolve structure,
3. do not hide WBOIT as “special runtime knowledge” outside the contract.


## Runtime Ownership Boundaries

Transparency should respect the existing scene/runtime boundaries.

Rules:

1. scene selects the transparent mode and emits the required plan,
2. runtime owns the concrete attachment allocation and execution,
3. canvas/runtime boundaries remain the owner of frame targets and command-buffer lifecycle,
4. scene does not begin owning swapchain/render-target internals just because transparency is
   multi-pass.

This matches the active v0.4 design discipline.


## Capability Model

There should not be a single public “WBOIT supported” boolean at the scene boundary.

Recommendation:

1. derive WBOIT availability from lower-level capability facts,
2. expose scene-facing diagnostics when a requested transparent mode cannot be realized.

Relevant lower-level capability pressures include:

1. enough color attachments,
2. floating-point color attachment support for accumulation targets,
3. color blending support,
4. ability to execute the needed accumulation and resolve passes.

Scene behavior when WBOIT is unavailable:

1. requesting `DVZ_ALPHA_WBOIT` on an unsupported runtime should be a capability failure /
   explicit diagnostic,
2. do not silently downgrade to some weaker blend path unless a future explicit fallback policy is
   designed.


## Mesh Interaction

The active mesh family uses the same visual family for opaque and transparent mesh visuals.

Recommended behavior:

1. opaque mesh and transparent mesh remain the same family,
2. transparent path is chosen through visual alpha mode,
3. material opacity participates in the transparent accumulation path,
4. no separate “transparent mesh” public visual family is needed.


## Picking Interaction

Picking and transparency need to be considered together now.

Recommendation:

1. transparent visuals should still be pickable,
2. picking should not rely on the transparency composite pass as the source of identity,
3. pick passes/buffers should remain logically separate from color-composite transparency passes.

For the first design stage:

1. do not encode pick ids into WBOIT accumulation targets,
2. keep pick rendering as a separate concern with shared scene identity tables.

This avoids coupling transparency math with identity resolution.


## Highlighting and Selection

Future highlighting/selection should be compatible with transparency.

Recommended direction:

1. selection/highlight state modifies the transparent visual’s appearance before transparent
   accumulation,
2. it should not require a second unrelated transparency architecture.

Exact highlight rules can be specified later, but the WBOIT path should assume highlighted
transparent visuals are normal and supported.


## Relationship To Text and Annotation

Transparent scene geometry and overlaid annotations will coexist.

Recommended ordering intent:

1. WBOIT handles transparent scene geometry,
2. annotation overlays and screen-space helpers should remain explicitly placed in the frame plan,
3. they should not accidentally become transparent scene geometry unless intentionally authored that
   way.

This matters for scale bars, 3D labels, and bounding-box annotations.


## Why Not Standard Alpha Blending First

Do not treat standard sorted alpha blending as the default OIT implementation target.

Reasons:

1. it bakes ordering assumptions into the wrong layer,
2. it is a poor fit for the intended scientific visualization use cases that need OIT,
3. the branch already has WBOIT as the explicit first OIT path.


## Initial Public API Direction

The exact names can evolve, but conceptually the public scene surface should expose:

1. a visual alpha-mode selector,
2. material opacity,
3. diagnostics/capability feedback when transparent mode cannot be planned.

Suggested conceptual calls:

1. `dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_OPAQUE / DVZ_ALPHA_BLENDED /
   DVZ_ALPHA_WBOIT / DVZ_ALPHA_DEPTH_PEEL)` or equivalent
2. material opacity/state setters on mesh or future text/image visuals

The implementation should map this to:

1. opaque frame-plan nodes, or
2. opaque + WBOIT accumulation + resolve nodes.


## Phase-1 Implementation Status

The active first transparency slice includes:

1. retained visuals with `DVZ_ALPHA_WBOIT`,
2. per-panel accumulation targets,
3. explicit transparent accumulation pass,
4. explicit composite pass,
5. capability-gated planning and diagnostics.

This is enough to pressure the architecture without treating every transparent visual family as
complete.


## Explicit Non-Goals For The First Transparency Slice

1. exact per-pixel linked-list OIT,
2. implicit downgrade to standard alpha blending,
3. every transparent visual family at once,
4. baking transparency behavior into material alpha alone,
5. scene ownership of low-level frame-target internals.
