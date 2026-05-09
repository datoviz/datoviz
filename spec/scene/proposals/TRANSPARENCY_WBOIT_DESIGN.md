> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 transparency contract with weighted blended OIT as the
>   first transparent rendering path.

# Transparency and WBOIT Design

This note records the intended transparency contract for the active v0.4 scene stack.


## Objective

Support transparent visuals as a first-class scene feature using weighted blended order-independent
transparency (WBOIT) as the first implementation target.

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

WBOIT should be designed now, even if the first opaque mesh lands before the first transparent
visual.


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
2. transparency path is a visual/render mode,
3. scene/frame-plan decides pass structure from that visual mode.

Reserved visual render modes:

1. `opaque`
2. `transparent_wboit`

Deferred:

1. exact OIT / per-pixel linked list mode
2. implicit fallback to standard sorted alpha blending

Recommendation:

1. do not expose ordinary back-to-front alpha blending as the default transparent path,
2. treat WBOIT as the intended transparent path for v0.4,
3. do not silently downgrade a visual that asks for WBOIT when capability is missing.


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

1. render mode = `transparent_wboit`
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
2. transparent accumulation pass
3. transparent composite/resolve pass

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

WBOIT pressures DRP2 in the following areas:

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

Scene behavior when unavailable:

1. requesting `transparent_wboit` on an unsupported runtime should be a capability failure /
   explicit diagnostic,
2. do not silently downgrade to some weaker blend path unless a future explicit fallback policy is
   designed.


## Mesh Interaction

The active mesh family should be designed so transparent mesh visuals can use the same visual
family with a different render mode.

Recommended behavior:

1. opaque mesh and transparent mesh remain the same family,
2. transparent path is chosen through visual render mode,
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

Do not treat standard sorted alpha blending as the default transparent implementation target.

Reasons:

1. it bakes ordering assumptions into the wrong layer,
2. it is a poor fit for the intended scientific visualization use cases,
3. it would likely create rework once WBOIT lands,
4. the branch already treats WBOIT as the intended v0.4 transparent path.


## Initial Public API Direction

The exact names can evolve, but conceptually the public scene surface should expose:

1. a visual alpha/render mode selector,
2. material opacity,
3. diagnostics/capability feedback when transparent mode cannot be planned.

Suggested conceptual calls:

1. `dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_OPAQUE / DVZ_ALPHA_BLENDED)` or equivalent
2. material opacity/state setters on mesh or future text/image visuals

The implementation should map this to:

1. opaque frame-plan nodes, or
2. opaque + WBOIT accumulation + resolve nodes.


## Phase-1 Implementation Target

The first transparency implementation target should be:

1. mesh visual with `transparent_wboit`,
2. per-panel accumulation targets,
3. explicit transparent accumulation pass,
4. explicit composite pass,
5. capability-gated planning and diagnostics,
6. offscreen and live example coverage.

This is enough to pressure the architecture correctly without trying to solve every transparent
visual family at once.


## Explicit Non-Goals For The First Transparency Slice

1. exact per-pixel linked-list OIT,
2. implicit downgrade to standard alpha blending,
3. every transparent visual family at once,
4. baking transparency behavior into material alpha alone,
5. scene ownership of low-level frame-target internals.
