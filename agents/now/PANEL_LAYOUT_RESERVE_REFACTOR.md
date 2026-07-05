# Panel Layout Reserve Refactor Plan

Status: approved direction, ready for implementation.

Goal: remove the normalized panel layout-reserve compatibility path before v0.4 RC and make plot
space a pixel-based product of panel padding plus attached adornment layout claims.


## Decision

Remove the public normalized reserve API:

1. `DvzPanelLayoutReserve`
2. `dvz_panel_layout_reserve()`
3. `dvz_panel_set_layout_reserve()`
4. `dvz_panel_get_layout_reserve()`

Keep `dvz_panel_set_reserve()` and `dvz_panel_get_reserve()` as the advanced/manual logical-pixel
plot reserve override. Do not replace `DvzPanelLayoutReserve` with another normalized spacing API.

Bare panels should remain edge-to-edge by default. Attached axes, colorbars, and legends should
claim their own pixel reserve automatically. Example polish should use grid margins, grid gutters,
panel padding, adornment styles, or explicit pixel reserve only when the example is intentionally
demonstrating manual layout.


## Current Shape

Relevant files:

1. `include/datoviz/scene.h`
2. `include/datoviz/scene/types.h`
3. `src/scene/core/panel_layout.c`
4. `src/scene/core/panel_geometry.c`
5. `src/scene/core/scene.c`
6. `src/app/app.c`
7. `src/scene/annotation/axis_layout.c`
8. `src/scene/annotation/colorbar.c`
9. `src/scene/annotation/legend.c`
10. `examples/c/example_common.c`
11. `examples/c/example_common.h`
12. `src/scene/tests/axis.c`
13. `src/scene/tests/app.c`
14. `src/scene/tests/dpi.c`
15. `src/scene/tests/scene_interaction_graph.c`

Current panel reserve is already mostly pixel-based:

```text
resolved reserve = base_reserve + axis_reserve + colorbar_reserve + legend_reserve
```

The normalized path is a compatibility bridge: it stores `layout_reserve_enabled`, converts
normalized side values to `base_reserve` at the current panel size, and refreshes that conversion on
resize. This is the part to delete.


## Invariants

Keep these behaviors true after the refactor:

1. A panel with no padding and no attached adornments has a plot rect equal to its panel rect.
2. `dvz_panel_set_padding()` shrinks the panel inner rect before reserves are applied.
3. `dvz_panel_set_reserve()` accepts logical pixels and remains stable across figure/window resize.
4. Attached axes reserve enough room for ticks and labels without user reserve calls.
5. Attached colorbars reserve enough room for ramp, tick labels, title, and gaps.
6. Attached legends reserve enough room for their configured/default width.
7. Invalid aggregate reserves must not collapse the app; preserve the current defensive behavior of
   falling back to a valid base reserve or zero reserve.
8. DPI/user-scale changes still mark screen-space visuals and layout-dependent adornments dirty.
9. Do not create a parallel presentation, layout, renderer, or Vulkan path.


## Implementation Steps

### 1. Remove Public Normalized API

Delete `DvzPanelLayoutReserve` from `include/datoviz/scene/types.h`.

Delete declarations and docs for:

1. `dvz_panel_layout_reserve()`
2. `dvz_panel_set_layout_reserve()`
3. `dvz_panel_get_layout_reserve()`

Keep and tighten docs for:

1. `dvz_panel_set_reserve()`
2. `dvz_panel_get_reserve()`
3. `dvz_panel_set_padding()`
4. `dvz_panel_plot_rect_px()`
5. `dvz_panel_inner_rect_px()`

Make the docs say pixel reserve is an advanced/manual plot-space override, while attached
adornments normally manage their own reserve.


### 2. Delete Compatibility State

Remove panel fields related to normalized layout reserve from internal structs:

1. `layout_reserve_enabled`
2. `layout_reserve`

Search before editing:

```sh
rg -n "layout_reserve|DvzPanelLayoutReserve|dvz_panel_set_layout_reserve|dvz_panel_get_layout_reserve|dvz_panel_layout_reserve" include src examples spec docs
```

Delete these functions from `src/scene/core/panel_layout.c`:

1. `_panel_layout_reserve_valid()`
2. `dvz_panel_layout_reserve()`
3. `dvz_panel_set_layout_reserve()`
4. `_scene_panel_refresh_layout_reserve()`
5. `dvz_panel_get_layout_reserve()`

Delete the refresh call in resize/layout paths, including the current app resize hook that refreshes
normalized reserves. Pixel reserves should not need conversion on resize.


### 3. Preserve Pixel Reserve Semantics

In `dvz_panel_set_reserve()`:

1. stop clearing normalized layout state because it no longer exists;
2. continue validating against the current padded panel size;
3. continue updating `base_reserve`;
4. continue recomputing the resolved reserve and marking layout/view dirty.

In `dvz_panel_get_reserve()`:

1. decide whether the public getter should return the resolved reserve or the manual/base reserve;
2. prefer preserving existing behavior for now: return the current resolved reserve;
3. if changing semantics, update docs and tests explicitly.


### 4. Keep Automatic Adornment Reserve

Do not remove the current axis/colorbar/legend contribution paths. They are the desired model.

Audit these functions after normalized reserve deletion:

1. `_scene_panel_refresh_axis_reserve()`
2. `_scene_panel_refresh_colorbar_reserve()`
3. `_scene_panel_refresh_legend_reserve()`
4. `_scene_panel_set_axis_reserve()`
5. `_scene_panel_set_colorbar_reserve()`
6. `_scene_panel_set_legend_reserve()`

If examples still need broad manual reserves after this cleanup, first check whether the attached
adornment default reserve is too small. Prefer improving the relevant adornment default or style
escape hatch over adding a generic layout reserve backdoor.


### 5. Update Tests

Remove tests whose only purpose is normalized reserve conversion.

Replace them with focused tests for:

1. manual pixel reserve affects plot rect and remains stable after resize;
2. padding plus manual reserve composes correctly;
3. attached X/Y axes produce nonzero bottom/left reserve without manual reserve;
4. attached colorbar produces reserve on its anchor side;
5. attached legend produces reserve on its anchor side;
6. invalid manual aggregate reserve still falls back safely.

Likely test files:

1. `src/scene/tests/axis.c`
2. `src/scene/tests/dpi.c`
3. `src/scene/tests/app.c`
4. `src/scene/tests/scene_interaction_graph.c`

Do not keep compatibility tests for deleted APIs.


### 6. Update Examples

Remove all non-legacy example calls to `dvz_panel_set_layout_reserve()`.

For examples using `example_configure_equal_aspect_panel()`:

1. remove the `DvzPanelLayoutReserve* reserve` parameter;
2. call `example_graphite_cyan_set_panel_background()` and configure `DvzPanelView2DDesc` only;
3. if a specific example needs cosmetic spacing, add a local or shared pixel helper.

Preferred example spacing tools:

1. `dvz_grid_set_margins()`
2. `dvz_grid_set_gutter()`
3. `dvz_panel_set_padding()`
4. adornment style descriptors and explicit adornment `reserve_px`
5. `dvz_panel_set_reserve()` only for intentional manual plot reserve

Legacy examples may either be updated or left out of the public build depending on current repo
policy, but the deleted API must not remain referenced anywhere that builds.


### 7. Update Docs And Generated References

Search `spec/`, `docs/`, and binding metadata for references to the deleted API.

Update only source docs or generated-reference inputs. Do not hand-edit generated public reference
output unless this repo's current documentation flow expects that.

The intended public language:

1. Panels are edge-to-edge by default.
2. Pixel padding reserves room inside a panel before adornments.
3. Attached adornments reserve their own bands.
4. Manual pixel reserve is advanced and rarely needed.


## Validation

Use the narrowest useful loop while iterating, then run the broader checks before committing.

Minimum required before finalizing:

```sh
rg -n "DvzPanelLayoutReserve|dvz_panel_layout_reserve|dvz_panel_set_layout_reserve|dvz_panel_get_layout_reserve" include src examples spec docs
just build
just test axis
just test dpi
git diff --check
```

If `just test axis` or `just test dpi` is not the right filter name, inspect the local test runner
and use the closest focused scene tests.

If build or tests fail for environment reasons, record the exact command and failure.


## Commit Hygiene

Before every commit:

```sh
git diff --check
git status --short
git diff --cached --stat
```

Do not stage or commit:

1. `data` submodule gitlink changes unless the user explicitly approves them in the current turn;
2. generated/runtime binaries such as `libs/vulkan/`, `*.dylib`, `*.so`, `*.dll`, `*.npy`,
   `*.npz`, or `.DS_Store`;
3. unrelated user changes.


## Final Report

Report:

1. deleted public symbols;
2. remaining public layout API;
3. example migration summary;
4. test commands and results;
5. any intentional behavior change in `dvz_panel_get_reserve()`;
6. any remaining manual pixel reserves and why they are still needed.
