# Colorbar Typography And Title Layout

Status: future proposal, unimplemented. This records a narrow follow-up, not a change to the installed API or a release requirement.

## Problem And Existing Contract

Colorbar placement already supports attached panel reserves and detached rectangles. Keep those contracts in [LEGENDS_AND_COLORBARS](../../semantics/LEGENDS_AND_COLORBARS.md) and [the placement decision](../promoted/PANEL_RESERVE_AND_COLORBAR_PLACEMENT.md). The gap is annotation typography: `src/scene/annotation/colorbar.c` fixes tick text at 12 px, title text at 16 px, title spacing at 8 px, and estimates text width from byte count. A large export shown in a smaller website slot needs larger labels without application-side replacement text.

Native fonts, text styling, and scene font defaults already exist. Reuse [semantic text](../../semantics/TEXT.md) and [shaping and atlas behavior](../../implementation/TEXT_SHAPING_ATLAS.md); do not introduce another text renderer.

## Proposed Direction

- Add a colorbar style descriptor and update operation, with separate tick-label and title styles based on `DvzTextStyle`. Names and ABI remain to be decided. Keep existing placement fields and defaults compatible.
- Inherit scene font defaults unless a role explicitly selects a font, size, or color. Specify whether later changes to defaults affect existing inherited styles, and invalidate affected annotations consistently. Preserve explicit overrides.
- Support semantic title locations such as automatic, above, below, and alongside, with an explicit title gap and documented orientation behavior. Tick length and label gap remain independent. Empty titles must consume no title space; a unit alone is valid title content.
- Use resolved font metrics and shaped UTF-8 bounds for layout and rendering, including baseline placement. Replacing a font with a condensed face must change measured extents rather than approximate them with character count. Numeric labels need stable baselines across signs and digit changes.
- Recompute attached reserves from the actual styled content. Detached colorbars stay inside their placement contract without silently resizing the scientific plot. Cover resize, logical-pixel scaling, clipping, font-resource lifetime, and style-update invalidation through existing scene policies.
- Styling and title placement must not modify the scale, domain, tick values, LUT, or ramp sampling. The existing appearance remains the default unless a metric correction fixes an erroneous reserve.

## Acceptance Criteria

Validate horizontal and vertical bars, attached and detached placement, omitted titles, unit-only titles, explicit and inherited fonts, and updates after initial rendering. Include large condensed headings, regular-width tabular numbers, signed values, and units containing µ. Check title and label bounds at multiple DPI scales and after resize, alongside unchanged scalar/ramp values.

Provide small C and Python examples and a portable WebGPU styling proof when implemented. Tests must exercise shared layout/render metrics and reserve invalidation, not just descriptor storage.

## Open Decisions

Choose the smallest compatible descriptor extension or separate setter; define automatic title-side resolution and overflow diagnostics; settle inherited-default update semantics before implementation. Arbitrary per-tick XY offsets and general figure layout are outside this change.
