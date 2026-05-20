> **Execution Status**
> - **Status:** `MOSTLY PROMOTED`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve WBOIT rationale and follow-up work after promotion into specialized
>   transparency, frame-plan, adaptation, and runtime-boundary specs.

# Transparency and WBOIT Design

This is a promoted proposal record. Active transparency rules live in the canonical specs.


## Decision Addressed

`DVZ_ALPHA_WBOIT` is the active first order-independent transparency path. WBOIT was chosen because
it avoids CPU sorting, is much simpler than exact per-pixel linked lists, and fits scientific scenes
with many translucent fragments.

Transparency remains a visual alpha mode and frame-plan choice, not just “material alpha less than
one.”


## Canonical Specs

Active rules moved to:

1. [`../../semantics/TRANSPARENCY.md`](../../semantics/TRANSPARENCY.md) for alpha modes, WBOIT
   properties, accumulation/resolve targets, selection interaction, and frame-plan structure.
2. [`../../pipeline/FRAME_PLAN.md`](../../pipeline/FRAME_PLAN.md) for pass ordering.
3. [`../../validation/ADAPTATION.md`](../../validation/ADAPTATION.md) for capability-aware
   adaptation and diagnostics.
4. [`../../core/RUNTIME_BOUNDARY.md`](../../core/RUNTIME_BOUNDARY.md) for runtime ownership and
   capability fields.
5. [`../../semantics/LIGHTING.md`](../../semantics/LIGHTING.md) for transparent materials with
   lighting.

Low-level grounding remains in
[`../../../../src/vklite/tests/test_techniques.c`](../../../../src/vklite/tests/test_techniques.c)
and the WBOIT test shaders under `src/vklite/tests/shaders/`.


## Rationale To Preserve

The public model should keep these modes explicit:

1. `DVZ_ALPHA_OPAQUE`
2. `DVZ_ALPHA_BLENDED`
3. `DVZ_ALPHA_WBOIT`
4. `DVZ_ALPHA_DEPTH_PEEL`
5. `DVZ_ALPHA_MASK`

Do not silently downgrade a visual that requests WBOIT. If the required runtime capabilities are
missing, emit explicit adaptation/validation diagnostics unless a future fallback policy is
designed and opted into.


## Proposal-Owned Follow-Up

1. Align DRP2 fixtures around the explicit multi-pass WBOIT accumulation and resolve structure.
2. Keep WBOIT represented through the emitted command stream, not as scene-private runtime magic.
3. Decide whether the public API needs named quality/weight controls beyond `DVZ_ALPHA_WBOIT`.
4. Preserve transparent picking as a separate identity path; do not encode pick ids into WBOIT
   accumulation targets.
5. Verify transparent selection/highlight styling through the same visual family rather than a
   parallel “transparent visual” family.
6. Keep annotation and overlay ordering explicit so screen-space overlays do not accidentally become
   transparent scene geometry.


## Non-Goals Still Valid

1. Exact per-pixel linked-list OIT in the first slice.
2. Implicit fallback to source-over blending.
3. Completing every transparent visual family at once.
4. Scene ownership of low-level frame-target internals.
