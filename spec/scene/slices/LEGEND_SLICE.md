# Legend Slice

Status: first retained categorical legend slice implemented. Updated: 2026-08-01.

Use [../semantics/LEGENDS_AND_COLORBARS.md](../semantics/LEGENDS_AND_COLORBARS.md) for normative behavior. Continuous colorbars and categorical legends remain separate explanatory-object families.

## Implemented Surface

- `DvzLegend`, `DvzLegendDesc`, `dvz_legend_desc()`, construction, destruction, layout, title, highlight, and multi-highlight APIs are public.
- Legends consume retained categorical scale entries with stable IDs, order, labels, and colors.
- Generated sample marks and text lower through ordinary scene visuals.
- Attached placement participates in panel reserves; detached placement uses explicit placement policy.
- Retained updates, visibility, highlighting, cleanup, tests, examples, bindings, and documentation are present.

## Boundary

Legends explain categorical scales; colorbars explain continuous scales. The first slice does not own interactive filtering, grouped sections, dense collision avoidance, or arbitrary aggregation across unrelated scales. Those require separate design pressure and must preserve stable category identity and normal scene lowering.
