# Pinned Readout Overlay Card Implementation

> **Execution Status**
> - **Status:** `DONE`
> - **Updated on:** `2026-05-26`
> - **Purpose:** record the native C implementation path from rendered pinned image readouts to
>   reusable internal cards, public overlay cards, and private rich text blocks.

This record closes the C lane that connected image probe readouts, retained card semantics,
generic screen-space card infrastructure, and private rich text-block rendering. Detailed
semantics remain in `spec/scene/`; this file is the implementation and validation handoff.


## Landed Slices

These commits are the current implemented baseline for this lane:

1. `8cd5af73c docs: add pinned readout overlay card plan`
2. `eddb3830e scene: format pinned readout payloads`
3. `e6ffd7556 scene: render pinned readout cards`
4. `213a78a93 examples: pin image probe readouts`
5. `acdbbca35 scene: factor pinned readouts through internal cards`
6. `c8d85a70d scene: add private rich text block parser`
7. `1bdafc8bb scene: rasterize private text blocks`
8. `343b9bdc7 scene: lower text blocks to image visuals`
9. `acce3878f scene: smoke text block image rendering`
10. `180fbb1ca scene: show selection metadata cards`
11. `26382dc68 scene: add public overlay card API`
12. `12d8c16ea scene: harden overlay card layout and text style`
13. `84f23ff61 scene: harden rich text block raster backend`
14. `0c84332d5 scene: add rich text overlay cards`
15. `ab77a35f6 examples: showcase overlay cards`
16. `4e7600664 examples: add rich text block lowering demo`
17. `5a6640b67 scene: rasterize text blocks with freetype`

Current behavior:

1. `DvzPinnedReadout` formats scalar and vector probe payloads into retained text.
2. Pinned readouts render as retained screen-space cards using a private `DvzSceneCard` shell.
3. `examples/c/techniques/image_probe.c` pins the next resolved image probe on click.
4. A private `DvzTextBlock` parser/measurement prototype exists for `<b>`, `<i>`, `<u>`,
   `<color=#RRGGBB>`, escaped `<`, `>`, and `&`, with source/text style runs and diagnostics.
5. Text blocks can rasterize to owned RGBA8 pixels and lower to image-like scene visuals backed by
   explicit sampled fields, with scale-aware raster output. When a scene/font is provided and
   FreeType is available, rich text blocks use FreeType bitmap rasterization; the deterministic
   pseudo-glyph path remains as a dependency-light fallback.
6. Offscreen app coverage confirms text-block image lowering renders nonblank pixels.
7. Retained selections are the second in-tree `DvzSceneCard` consumer and render selected-item
   metadata cards through the same text/adornment path.
8. The public `DvzOverlay` / `DvzOverlayCard` API exposes panel-local retained cards without
   exposing the private `DvzSceneCard` shell.
9. Public overlay cards now support semantic placement, style updates, GPU text renderer selection,
   and rich text content lowered through the private text-block raster/image backend.
10. `examples/c/techniques/overlay_card.c` demonstrates polished GPU-text overlay cards, and
    `examples/c/techniques/overlay_rich_card.c` demonstrates rich overlay cards.
11. `examples/c/techniques/rich_text_block.c` demonstrates the same private rich text-block
    lowering path without overlay/card APIs.

Remaining follow-up is polish outside this completed lane: DPI cache keys, richer wrapping,
HarfBuzz shaping, richer font-style resolution, and broader annotation rich-text integration.


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
   for the rich text-block backend contract and follow-up polish.
6. [`../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md`](../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md)
   and [`../../spec/scene/slices/TEXT_RENDERING_SLICE.md`](../../spec/scene/slices/TEXT_RENDERING_SLICE.md)
   for the active glyph/text path.


## Scope Decisions

1. The first card consumer stayed private until pinned readouts and selected-item cards proved the
   internal shell.
2. Use existing glyph text for the first rendered pinned readout. Do not block on rich text-block
   rasterization.
3. Treat `DvzPinnedReadout` as a retained semantic readout/card, not as a visual-private helper.
4. Start with one background rectangle, one text body, padding, offset, visibility, and cleanup.
5. Place the first cards in panel-local screen coordinates anchored near the probe position.
6. Format probe payloads plainly: scalar, vector, and RGBA values with optional unit/suffix.
7. Generalize into an internal card shell before adding a second card consumer.
8. Promote public `DvzOverlay`/`DvzOverlayCard` APIs only after at least two consumers prove the
   private shape. This is now satisfied by pinned readouts and selected-item cards.


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

Status: **landed as private `DvzSceneCard` refactor** in `acdbbca35`; second consumer landed in
`180fbb1ca`.

After pinned readouts work, refactor the private card shell toward a small internal overlay/card
layer that can later support hover cards, selected-item cards, telemetry, and readouts without
duplicating visuals.

The second consumer gate is complete: selected-item metadata cards reuse the internal shell and
are covered by `test_scene_selection_card_realizes_pick_metadata`.


### 6. Public Overlay Card API

Status: **landed** in `26382dc68` and hardened in `12d8c16ea`.

The public first slice exposes:

1. opaque `DvzOverlay` and `DvzOverlayCard` handles,
2. `dvz_overlay()` / `dvz_overlay_destroy()`,
3. `dvz_overlay_card_style()` and `DvzOverlayCardDesc`,
4. `dvz_overlay_card()` / `dvz_overlay_card_destroy()`,
5. text, panel-local layout, and visibility setters,
6. focused public API coverage in `test_scene_overlay_card_public_api`,
7. semantic placement modes,
8. GPU text renderer selection for polished card labels,
9. a polished `examples/c/techniques/overlay_card.c` example.


### 7. Rich Text-Block Prototype

Status: **landed as private prototype** in `c8d85a70d`, `1bdafc8bb`, `343b9bdc7`,
`acce3878f`, hardened in `84f23ff61`, and upgraded to FreeType rasterization in `5a6640b67`.
The first slice covers parsing, fixed-advance measurement, deterministic RGBA8 raster output,
sampled-field image lowering, underline/color runs, scale-aware rasterization, and offscreen
nonblank rendering. DPI cache keys, richer wrapping, HarfBuzz shaping, richer font-style
resolution, and broader annotation integration remain follow-up polish.

Prototype a private text-block backend only after the glyph-card path is stable:

1. plain UTF-8 source plus style and max-width constraints,
2. CPU raster output to RGBA8/alpha pixels,
3. image-like quad lowering,
4. DPI-aware cache key and invalidation,
5. focused nonblank offscreen smoke.

Rich markup, rows, wrapping polish, math, and public API promotion are follow-up work unless a
release example depends on them.


### 8. Public Rich Overlay Cards and Examples

Status: **landed** in `0c84332d5` and `ab77a35f6`.

Public overlay cards can now switch from the plain GPU glyph path to rich text content:

1. `DvzOverlayRichTextDesc` describes source markup, fixed layout width, raster scale, and colors;
2. `dvz_overlay_card_set_rich_text()` parses/measures/rasterizes rich content and realizes it as a
   fixed overlay image inside the same card background/layout shell;
3. `dvz_overlay_card_clear_rich_text()` returns the card to the plain GPU-text path;
4. lifecycle, hide/show, and clear behavior are covered by
   `test_scene_overlay_card_rich_text_public_api`;
5. `overlay_card` now showcases polished multi-card GPU text;
6. `overlay_rich_card` showcases rich CPU-raster text inside a retained public overlay card.

Validation recorded for these slices:

```text
git diff --check
just build
just test scene/interaction
./build/examples/c/techniques/overlay_card 1
./build/examples/c/techniques/overlay_rich_card 1
```


### 9. FreeType Rich Text-Block Rasterization

Status: **landed** in `4e7600664` and `5a6640b67`.

The private text-block backend now has a readable font-backed path:

1. `DvzTextBlockRasterDesc` can carry a scene or font pointer plus raster scale and font size;
2. `_scene_text_block_rasterize()` uses FreeType when scene/font context is available;
3. the existing deterministic pseudo-glyph raster path remains the fallback for no-FreeType or
   no-scene tests;
4. rich overlay cards pass their scene into the raster descriptor, so public rich cards use
   FreeType-backed text in normal builds;
5. `rich_text_block` demonstrates non-overlay rich text lowering as a regular image visual;
6. focused tests and offscreen smoke cover parser/layout/raster/lowering behavior.

Validation recorded for this slice:

```text
git diff --check
just build
just test scene/interaction/text_block
just test scene/interaction/overlay_card_rich_text_public_api
just test scene/app-offscreen/text_block_raster_has_nonblank_pixels
./build/examples/c/techniques/rich_text_block 1
./build/examples/c/techniques/overlay_rich_card 1
```


## Subagent Use

Good bounded subagent tasks:

1. inspect existing text/annotation realization hooks and return the safest reuse points;
2. inspect probe-result payload fields and image-probe coordinate semantics;
3. verify tests/examples after a stage lands;
4. prototype rich text-block cache/dirty rules in a separate write scope if requested.

Avoid parallel code edits in the same files, especially `src/scene/interaction.c`,
`src/scene/text_annotation.c`, `src/scene/scene.c`, and shared scene test registration.


## Exit Criteria

This plan moved to `agents/done/` after:

1. pinned image probe readouts render as retained cards through the scene path;
2. the first card shell is reusable internally and documented in code;
3. focused scene tests cover formatting, lifecycle, and emitted visuals;
4. examples demonstrate pinned readouts, public overlay cards, and rich overlay cards;
5. the rich text-block backend is prototyped and validated by offscreen nonblank rendering;
6. public rich overlay cards lower rich text through image-like quads;
7. non-overlay rich text-block lowering is demonstrated by an in-tree example;
8. FreeType-backed rasterization makes rich text blocks readable in normal builds;
9. the public overlay API is gated by two internal consumers.
