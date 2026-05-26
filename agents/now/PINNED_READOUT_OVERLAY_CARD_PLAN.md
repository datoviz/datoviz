# Pinned Readout Overlay Card Plan

> **Execution Status**
> - **Status:** `ACTIVE IMPLEMENTATION PLAN`
> - **Updated on:** `2026-05-26`
> - **Purpose:** stage the native C implementation path from rendered pinned image readouts to
>   reusable internal overlay cards and later rich text blocks.

This plan is the active pickup record for the C lane that connects image probe readouts, retained
annotation/card semantics, generic screen-space card infrastructure, and future rich text-block
rendering. Keep detailed semantics in `spec/scene/`; keep this file as the execution route and
handoff record.


## Landed Slices

These commits are the current implemented baseline for this lane:

1. `8cd5af73c docs: add pinned readout overlay card plan`
2. `eddb3830e scene: format pinned readout payloads`
3. `e6ffd7556 scene: render pinned readout cards`
4. `213a78a93 examples: pin image probe readouts`
5. `acdbbca35 scene: factor pinned readouts through internal cards`
6. `c8d85a70d scene: add private rich text block parser`

Current behavior:

1. `DvzPinnedReadout` formats scalar and vector probe payloads into retained text.
2. Pinned readouts render as retained screen-space cards using a private `DvzSceneCard` shell.
3. `examples/c/techniques/image_probe.c` pins the next resolved image probe on click.
4. A private `DvzTextBlock` parser/measurement prototype exists for `<b>`, `<i>`, escaped
   `<`, `>`, and `&`, with source/text style runs and diagnostics.

Remaining follow-up before this plan should move to `agents/done/`:

1. decide whether `DvzSceneCard` should grow a second in-tree consumer before any public overlay API,
2. lower `DvzTextBlock` output to a raster/image-like scene contribution, or explicitly split that
   into a new rich-text backend plan,
3. add an offscreen nonblank smoke once rich text blocks render as image-like quads.


## Owning References

Read these before changing code in this lane:

1. [`../../spec/scene/proposals/active/SCREEN_SPACE_OVERLAY_LAYOUT.md`](../../spec/scene/proposals/active/SCREEN_SPACE_OVERLAY_LAYOUT.md)
   for the generic retained card/overlay model.
2. [`../../spec/scene/proposals/promoted/PROBE_READOUT_DESIGN.md`](../../spec/scene/proposals/promoted/PROBE_READOUT_DESIGN.md)
   for the probe/readout semantic split.
3. [`../../spec/scene/semantics/ANNOTATIONS.md`](../../spec/scene/semantics/ANNOTATIONS.md)
   for annotation classes, attachment, placement, contributions, and invalidation.
4. [`../../spec/scene/examples/api/API_IMAGE_PROBE_PINNED_READOUT.md`](../../spec/scene/examples/api/API_IMAGE_PROBE_PINNED_READOUT.md)
   for the public API pressure test.
5. [`../../spec/scene/implementation/TEXT_BLOCK_BACKENDS.md`](../../spec/scene/implementation/TEXT_BLOCK_BACKENDS.md)
   for the later rich text-block backend.
6. [`../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md`](../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md)
   and [`../../spec/scene/slices/TEXT_RENDERING_SLICE.md`](../../spec/scene/slices/TEXT_RENDERING_SLICE.md)
   for the active glyph/text path.


## Scope Decisions

1. Keep the first card API private under `src/scene`; do not expose `DvzOverlay*` yet.
2. Use existing glyph text for the first rendered pinned readout. Do not block on rich text-block
   rasterization.
3. Treat `DvzPinnedReadout` as a retained semantic readout/card, not as a visual-private helper.
4. Start with one background rectangle, one text body, padding, offset, visibility, and cleanup.
5. Place the first cards in panel-local screen coordinates anchored near the probe position.
6. Format probe payloads plainly: scalar, vector, and RGBA values with optional unit/suffix.
7. Generalize into an internal card shell before adding a second card consumer.
8. Promote public `DvzOverlay`/`DvzOverlayBox` APIs only after at least two consumers prove the
   private shape.


## Commit Stages

### 1. Pinned Readout Formatting

Status: **landed** in `eddb3830e`.

Add focused formatting helpers and tests for `DvzPinnedReadout` content:

1. scalar payloads,
2. vector/RGBA payloads,
3. `DvzFormatDesc` precision/unit/suffix handling,
4. deterministic fixed-size output storage.

Expected validation:

```text
just build
just test scene
git diff --check
```


### 2. Private Internal Card Shell

Status: **landed** in `e6ffd7556` and generalized in `acdbbca35`.

Add a reusable internal scene card shell that can be owned by retained semantic objects:

1. panel pointer and panel-local anchor point,
2. padding and offset,
3. background primitive/quad visual,
4. one glyph/text visual body,
5. visibility and destroy/hide lifecycle,
6. retained update path for text and placement.

Keep names internal and avoid public header churn unless a private header is needed.


### 3. Readout Card Realization

Status: **landed** in `e6ffd7556`.

Wire `DvzPinnedReadout` to the private card shell:

1. `dvz_pinned_readout()` realizes a card for hit probe results,
2. `dvz_pinned_readout_set_format()` updates text and dirty state,
3. `dvz_pinned_readout_destroy()` hides generated visuals and detaches panel bookkeeping,
4. frame-plan emission includes the generated background and glyph work.


### 4. Example Proof

Status: **landed** in `213a78a93`.

Update a narrow C example, preferably `examples/c/techniques/image_probe.c`, so a click/probe can
pin a persistent readout card. Keep the example bounded-frame friendly and avoid adding external
data.


### 5. Internal Overlay Generalization

Status: **landed as private `DvzSceneCard` refactor** in `acdbbca35`; a second consumer is still
the gate before public overlay API promotion.

After pinned readouts work, refactor the private card shell toward a small internal overlay/card
layer that can later support hover cards, selected-item cards, telemetry, and readouts without
duplicating visuals.

Do not expose public APIs in this stage unless a second committed consumer proves the shape.


### 6. Rich Text-Block Prototype

Status: **partially landed** in `c8d85a70d` as private parsing and fixed-advance measurement. CPU
raster output, image-like quad lowering, DPI cache keys, and nonblank rendering smoke remain.

Prototype a private text-block backend only after the glyph-card path is stable:

1. plain UTF-8 source plus style and max-width constraints,
2. CPU raster output to RGBA8/alpha pixels,
3. image-like quad lowering,
4. DPI-aware cache key and invalidation,
5. focused nonblank offscreen smoke.

Rich markup, rows, wrapping polish, math, and public API promotion are follow-up work unless a
release example depends on them.


## Subagent Use

Good bounded subagent tasks:

1. inspect existing text/annotation realization hooks and return the safest reuse points;
2. inspect probe-result payload fields and image-probe coordinate semantics;
3. verify tests/examples after a stage lands;
4. prototype rich text-block cache/dirty rules in a separate write scope if requested.

Avoid parallel code edits in the same files, especially `src/scene/interaction.c`,
`src/scene/text_annotation.c`, `src/scene/scene.c`, and shared scene test registration.


## Exit Criteria

This active plan can move to `agents/done/` when:

1. pinned image probe readouts render as retained cards through the scene path;
2. the first card shell is reusable internally and documented in code;
3. focused scene tests cover formatting, lifecycle, and emitted visuals;
4. one C example demonstrates the flow;
5. the rich text-block backend is either prototyped or explicitly split into a follow-up plan.
