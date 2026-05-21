# Scene 2D Axes Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining 2D axes work after the first panel-owned domain/tick/geometry
>   slice landed.


## Current State

Durable axes semantics live in
[`../../../spec/scene/semantics/AXES.md`](../../../spec/scene/semantics/AXES.md). The active
domain/ownership recommendation lives in
[`../../../spec/scene/proposals/active/AXES_DOMAIN_DESIGN.md`](../../../spec/scene/proposals/active/AXES_DOMAIN_DESIGN.md),
with controller binding behavior in
[`../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md`](../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md).

The active v0.4 code already has:

1. finite linear X/Y panel domains through `dvz_panel_set_domain()`;
2. visible-domain queries through `dvz_panel_visible_domain()`;
3. data-to-visual position mapping through `dvz_panel_data_to_visual_positions()`;
4. panel-owned `DvzAxis` handles through `dvz_panel_axis()`;
5. retained axis visibility, grid, label, tick policy, style, and plot-margin setters;
6. panzoom-aware linear tick regeneration with cached covered-domain reuse;
7. primitive-backed spine, major/minor tick, and optional grid geometry;
8. rendered tick labels and axis labels through the current `dvz_text()` visual path;
9. focused scene tests and the `examples/c/techniques/scatter_axes.c` smoke example.

Use this file only for remaining execution work. Do not duplicate stable axis/domain semantics here.


## Remaining Axes Work

Recommended follow-up commits:

1. Harden tick and axis label layout: collision behavior, edge clipping, formatter policy, and
   richer text style controls are still separate from the first rendered-label slice.
2. Harden plot-area reserve behavior so axes, labels, colorbars, legends, and annotations can share
   panel-adjacent space without one-off margins.
3. Add image/screenshot smoke coverage for `scatter_axes`.
4. Decide whether inverted numeric domains should become valid in the first v0.4 API. If yes, update
   validation, tick ordering, tests, and the semantics together.
5. Add log-domain and nonlinear-domain support only after linear labels and visible geometry are
   stable.
6. Keep linked panels domain-driven: shared navigation can imply shared visible domains, but each
   panel should own its own axis layout cache and derived resources.
7. Add axis-level picking only after overlay picking has a shared payload contract for semantic
   owner id, axis dimension, tick index, and data value.


## v0.3 Reference

Use the v0.3 axis stack as behavior reference, not as architecture:

1. `v0.3/src/scene/ticks.c`
2. `v0.3/include/datoviz/scene/ticks.h`
3. `v0.3/src/scene/box.c`
4. `v0.3/src/scene/ref.c`
5. `v0.3/src/scene/panzoom.c`
6. `v0.3/src/scene/axis.c`
7. `v0.3/tests/scene/test_ticks.c`
8. `v0.3/tests/scene/test_panzoom.c`
9. `v0.3/tests/scene/test_ref.c`

Useful ideas that remain relevant:

1. 1/2/5 nice-step tick ladder;
2. decimal precision and factored offset/exponent metadata;
3. data-domain to visual-space normalization;
4. panzoom extent conversion;
5. axis update flow from visible domain to ticks to derived geometry.

Avoid reviving v0.3 `DvzRef`, old batch/glyph/factor coupling, fixed-axis shader flags, or the
declared-but-not-implemented log-tick flag.


## Validation

For remaining axes work:

```text
just build
just test test_axis
just test text
just test scene
git diff --check
```

For rendered geometry or layout changes, also run the focused example or trace smoke:

```text
./build/examples/c/techniques/scatter_axes 60
DVZ_DRP2_TRACE=full DVZ_DRP2_TRACE_COLOR=0 ./build/examples/c/techniques/scatter_axes 2
```
