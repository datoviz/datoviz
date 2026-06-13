# Panel View Architecture Plan

Status: active implementation plan.

This plan records the long-term cleanup after promoting the equal-aspect panel view proposal. v0.4
may break v0.3 and interim v0.4-dev APIs when that produces a cleaner scene architecture.


## Target Ownership

Split panel 2D behavior into four separate concepts.

1. **Panel layout** owns panel rect, plot rect, reserve, padding, viewport, and scissor.
2. **Panel data domains** own user semantic bounds per dimension: explicit bounds, fit-to-data, and
   later inverted or log behavior.
3. **Panel view/framing** owns 2D aspect policy, framing padding, resolved base VIEW extent, fitted
   DATA domain, DATA-to-VIEW transform, and visible DATA domain.
4. **Controllers** own navigation state and gesture policy only. They resolve against a
   panel-provided base extent during frame planning and do not store viewport aspect.

The panel view resolver should be the single source of truth:

```text
panel domains + panel plot rect + panel view policy + controller state
    -> base VIEW extent
    -> fitted DATA domains
    -> visible VIEW extent
    -> visible DATA domains
    -> DATA-to-VIEW model
    -> panel apply MVP
```


## API Breaks

Remove compatibility-era names instead of carrying them into the v0.4 release surface:

1. delete `DvzPanelDomainFit`;
2. delete `DVZ_PANEL_DOMAIN_FIT_*`;
3. delete `DVZ_PANEL_DOMAIN_ASPECT_*`;
4. delete `dvz_panel_domain_fit()`;
5. delete `dvz_panel_set_domain_fit()`;
6. delete `dvz_panel_clear_domain_fit()`;
7. delete `DVZ_COORD_VISUAL`.

Require explicit coordinate-space selection through:

1. `DVZ_COORD_VIEW`: metric panel view coordinates, affected by equal-aspect view fit;
2. `DVZ_COORD_DATA`: data/domain coordinates, mapped through panel DATA -> VIEW;
3. `DVZ_COORD_PANEL`: normalized panel coordinates, intentionally viewport-shaped.

Keep the current view-fit API names only if they remain clear after the cleanup:

```c
DvzPanelViewFit dvz_panel_view_fit(void);
int dvz_panel_set_view_fit(DvzPanel* panel, const DvzPanelViewFit* fit);
void dvz_panel_clear_view_fit(DvzPanel* panel);
bool dvz_panel_view_extent(DvzPanel* panel, float out[4]);
```

If the term "fit" remains too narrow, rename the descriptor before freezing the public API. Candidate
names are `DvzPanelView2D` or `DvzPanelFrame2D`.


## Derived State Rule

Do not mutate panel domain storage to apply equal-aspect fit.

Panel domains should retain the user/source domain. The resolver should produce fitted and visible
domains as derived state. Axes, grids, scale bars, queries, DATA-coordinate visual transforms, and
controller links should consume resolver output instead of reading rewritten panel domains.

Replace this pattern:

```text
set view fit -> mutate panel X/Y domains
```

with:

```text
set view policy/source domains -> mark panel view dirty
frame/update/query -> resolve derived fitted domains
```

This keeps resize, panel reserve changes, linked panels, and future domain overrides from depending
on hidden domain rewrites.


## Module Boundaries

Add a core panel view implementation file:

```text
src/scene/core/panel_view.c
```

Move or place these responsibilities there:

1. `dvz_panel_view_fit()` or its renamed successor;
2. `dvz_panel_set_view_fit()`;
3. `dvz_panel_clear_view_fit()`;
4. `dvz_panel_view_extent()`;
5. `_scene_panel_view2d_resolve()`;
6. resolved visible-domain helpers;
7. `_scene_panel_data_model()`;
8. any internal dirty/invalidation hook for panel view policy.

Keep `src/scene/core/panel_geometry.c` focused on geometry:

1. panel pixel rects;
2. plot pixel rects;
3. plot/panel visual rect helpers;
4. layout-space conversion.

Keep `src/scene/annotation/axis.c` focused on axes:

1. axis creation and ownership;
2. axis explicit override state;
3. tick, grid, label, and visual contribution preparation;
4. no panel view-fit ownership.

Keep `src/scene/core/controllers.c` focused on controller binding and link propagation through
resolved panel extents.


## Spec Updates

Before or alongside the API break, update canonical specs so they no longer describe the old model:

1. `spec/scene/pipeline/TRANSFORM_PIPELINE.md`: replace `DVZ_COORD_VISUAL` with `DVZ_COORD_VIEW`
   and `DVZ_COORD_PANEL`; state that panel view/framing owns aspect policy.
2. `spec/scene/core/PANEL_LAYOUT.md`: make plot rect versus panel rect interaction explicit for
   the view resolver.
3. `spec/scene/interaction/CONTROLLERS.md`: clarify that panzoom aspect is gesture policy only;
   controller state does not own viewport aspect.
4. `spec/scene/semantics/AXES.md`: state that axes consume resolved visible DATA domains from the
   panel view resolver.
5. `spec/scene/api/API_SURFACE.md`: remove domain-fit aliases and `DVZ_COORD_VISUAL` from the
   release surface.


## Commit Sequence

Prefer small commits with focused validation.

1. **Spec/API decision:** update specs to make `VIEW`, `DATA`, and `PANEL` the only coordinate
   spaces, panel view the owner of aspect, and domain-fit aliases removed.
2. **Public API break:** delete old domain-fit typedefs, functions, enum values, and
   `DVZ_COORD_VISUAL`; fix compile errors mechanically.
3. **Core ownership:** add `panel_view.c`, move view-fit APIs and resolver there, and remove view
   policy ownership from axis code.
4. **Derived-state resolver:** stop writing fitted domains back into panel domain storage; route
   visible-domain, axis, grid, query, and DATA-to-VIEW paths through resolver output.
5. **Controller/link tightening:** make extent links explicitly use panel-context resolver output.
6. **Validation:** strengthen resize, equal-aspect, linked-panel, axes/grid, and API-absence tests.


## Validation Targets

Use narrow loops while iterating, then broaden before landing the full architecture slice:

```sh
just test scene
just spec-check
git diff --check
```

Focused coverage should include:

1. equal-aspect VIEW-coordinate geometry on wide and tall panels;
2. DATA-coordinate transforms after resize and reserve changes;
3. axes/grid tick alignment against resolved visible DATA domains;
4. `DVZ_COORD_PANEL` stretch behavior;
5. linked `EXTENT_X` and `EXTENT_Y` behavior across different panel aspect ratios;
6. compile or header checks proving removed compatibility symbols are absent from the public API.
