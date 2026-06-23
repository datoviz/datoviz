# Handoff: panel 2D axes convenience API

Status: proposed implementation plan.

Goal: add a narrow public convenience API for common 2D panel axes so examples can rely on good
axis defaults and keep local helpers focused on example-specific styling.


## Context

The immediate pressure came from `examples/c/features/user_scale.c`, whose local `_add_axes()`
mostly repeats boilerplate:

1. fetch panel-owned X/Y axes with `dvz_panel_axis()`;
2. apply a reasonable tick policy;
3. enable grids;
4. apply graphite/cyan example style;
5. set labels.

Similar helper duplication exists in other examples, but the first patch should prove the API on
`user_scale.c` only.

Do not add a high-level plotting layer. This is an operation over existing panel-owned axes, not a
new axis ownership model.


## Design Constraints

Keep these current API rules intact:

1. axes are panel-owned semantic objects;
2. `dvz_panel_axis()` returns a borrowed, non-destroyable singleton per dimension;
3. free-standing axis constructors are not supported;
4. existing per-axis setters remain the low-level mutation surface;
5. the new helper is a common atomic setup path, not a replacement for per-axis APIs.

Relevant existing APIs:

```c
DvzAxis* dvz_panel_axis(DvzPanel* panel, DvzDim dim);
DvzAxisTickPolicy dvz_axis_tick_policy(void);
DvzAxisStyle dvz_axis_style(void);
bool dvz_axis_set_visible(DvzAxis* axis, bool visible);
bool dvz_axis_set_grid(DvzAxis* axis, bool visible);
bool dvz_axis_set_label(DvzAxis* axis, const char* label);
bool dvz_axis_set_tick_policy(DvzAxis* axis, const DvzAxisTickPolicy* policy);
bool dvz_axis_set_style(DvzAxis* axis, const DvzAxisStyle* style);
```


## Public API Target

Add a compact descriptor to `include/datoviz/scene/types.h` near the axis types:

```c
typedef struct DvzPanelAxes2DDesc
{
    uint32_t struct_size;
    uint32_t flags;
    const char* x_label;
    const char* y_label;
    DvzAxisTickPolicy tick_policy;
    DvzAxisStyle x_style;
    DvzAxisStyle y_style;
} DvzPanelAxes2DDesc;
```

Add public functions to `include/datoviz/scene.h` in the panel/axis section:

```c
DvzPanelAxes2DDesc dvz_panel_axes_2d_desc(void);
bool dvz_panel_set_axes_2d(DvzPanel* panel, const DvzPanelAxes2DDesc* desc);
```

`dvz_panel_set_axes_2d(panel, NULL)` should use `dvz_panel_axes_2d_desc()`.

Use `set`, not `configure`: the current public API does not use an exported `configure` naming
pattern.


## Defaults

`dvz_panel_axes_2d_desc()` should return good neutral Datoviz defaults:

1. `struct_size = sizeof(DvzPanelAxes2DDesc)`;
2. `flags = 0`;
3. no labels by default;
4. `tick_policy = dvz_axis_tick_policy()`;
5. `x_style = dvz_axis_style()`;
6. `y_style = dvz_axis_style()`;
7. grids enabled through `x_style.show_grid = true` and `y_style.show_grid = true`.

Do not put `x_visible`, `y_visible`, `x_grid`, or `y_grid` in the first descriptor. Existing
per-axis setters already cover asymmetry. The convenience API should express "good default 2D axes",
not every possible axis mutation.


## Implementation Notes

Implement in `src/scene/annotation/axis.c`, or the closest existing axis implementation file.

`dvz_panel_set_axes_2d()` should:

1. validate `panel`;
2. validate descriptor ABI when `desc != NULL`;
3. fetch `DVZ_DIM_X` and `DVZ_DIM_Y` axes with `dvz_panel_axis()`;
4. apply the descriptor tick policy to both axes;
5. apply `x_style` and `y_style`;
6. set labels, where `NULL` clears or leaves labels according to the existing `dvz_axis_set_label()`
   contract;
7. ensure both axes are visible;
8. return `false` on the first validation or setter failure.

Prefer reusing existing validation style from axis tick policy/style validation.

If adding a new public size-versioned struct, add tests that invalid `struct_size` and unknown
flags are rejected.


## Example Helper Target

Add one example-only style helper in `examples/c/example_style.[ch]`:

```c
bool example_graphite_cyan_style_axes_2d(
    DvzPanel* panel, const ExampleAxisStyleOptions* options);
```

This helper should be styling-only:

1. fetch X/Y axes;
2. apply `example_graphite_cyan_apply_axis_style(x_axis, false, options)`;
3. apply `example_graphite_cyan_apply_axis_style(y_axis, true, options)`;
4. do not set tick policy;
5. do not set labels;
6. do not own axis visibility policy beyond what the style already implies.


## First Example Conversion

Update only `examples/c/features/user_scale.c` in the first implementation patch.

Target shape:

```c
static bool _add_axes(DvzPanel* panel)
{
    ANN(panel);

    DvzPanelAxes2DDesc axes = dvz_panel_axes_2d_desc();
    axes.x_label = "x";
    axes.y_label = "amplitude";

    if (!dvz_panel_set_axes_2d(panel, &axes))
        return false;
    return example_graphite_cyan_style_axes_2d(panel, NULL);
}
```

Do not mechanically update every `_add_axes()` clone in the first patch. Once this proves the API,
the remaining examples can be cleaned up separately.


## Tests

Add focused coverage in the scene axis tests:

1. `dvz_panel_axes_2d_desc()` returns current-size initialized records;
2. default descriptor enables X/Y axes and grid;
3. `dvz_panel_set_axes_2d(panel, NULL)` succeeds and applies defaults;
4. labels are applied;
5. custom tick policy is applied to both axes;
6. custom X/Y styles are applied independently;
7. invalid `struct_size` is rejected;
8. unknown flags are rejected.

If there is no public getter for axis internals, follow the existing axis test pattern and inspect
derived state through internal test helpers or frame emission evidence.


## Validation

Run the narrowest relevant checks:

```sh
just test axis
just example-c features/user_scale
git diff --check
```

If touching generated docs or bindings, run the appropriate generator/check from the existing
workflow before finalizing.


## Out Of Scope

1. high-level plotting functions;
2. axis constructors or `dvz_axis_destroy()`;
3. 3D axes;
4. log, categorical, datetime, polar, or geographic axis semantics;
5. full example tree refactor;
6. public style/theme system.
