# Reference Grid API Direction

This note records the v0.4 API direction for the scene reference grid.

The v0.4 reference grid remains a scene-owned aid created through
`dvz_reference_grid(panel, &desc)`. It should not revive the v0.3 standalone grid visual API.


## Background

Datoviz v0.3 exposed an infinite-looking horizontal grid as a visual-level API:

```c
DvzVisual* dvz_panel_grid(DvzPanel* panel, int flags);
DvzVisual* dvz_grid(DvzBatch* batch, int flags);

void dvz_grid_color(DvzVisual* grid, vec4 value);
void dvz_grid_linewidth(DvzVisual* grid, float value);
void dvz_grid_scale(DvzVisual* grid, float value);
void dvz_grid_elevation(DvzVisual* grid, float value);
```

The Python wrapper exposed this as `panel.horizontal_grid(elevation=...)`.

The v0.3 implementation drew a very large XZ quad and generated grid lines in the fragment shader
with a repeating procedural pattern. It provided a useful infinite-ground appearance, and later
v0.3 revisions faded the grid by view-space distance. However, the API was horizontal-only,
visual-level, and did not express v0.4 scene concepts such as custom planes, major and minor line
styles, axis styling, retained ownership, and panel-local scene aids.


## Proposed API

Add two orthogonal controls to `DvzReferenceGridDesc`: one describing the generated extent, and one
describing the fade reference.

```c
typedef enum DvzReferenceGridExtent
{
    DVZ_REFERENCE_GRID_EXTENT_FIXED = 0,
    DVZ_REFERENCE_GRID_EXTENT_HORIZON,
    DVZ_REFERENCE_GRID_EXTENT_VIEW_ADAPTIVE,
} DvzReferenceGridExtent;

typedef enum DvzReferenceGridFade
{
    DVZ_REFERENCE_GRID_FADE_NONE = 0,
    DVZ_REFERENCE_GRID_FADE_GRID,
    DVZ_REFERENCE_GRID_FADE_VIEW,
} DvzReferenceGridFade;
```

Add these fields to `DvzReferenceGridDesc`:

```c
DvzReferenceGridExtent extent;
DvzReferenceGridFade fade;

float fade_start;
float fade_end;
float fade_alpha_min;
```


## Semantics

`DVZ_REFERENCE_GRID_EXTENT_FIXED` keeps the current behavior. The grid is exact finite geometry
derived from `size[2]` and `spacing`.

`DVZ_REFERENCE_GRID_EXTENT_HORIZON` creates a large world-snapped grid intended to read as an
infinite ground plane. It still has finite generated geometry.

`DVZ_REFERENCE_GRID_EXTENT_VIEW_ADAPTIVE` keeps the generated grid centered around the camera or
view focus, snapped to `spacing`, so long camera movement does not run off the grid.

`DVZ_REFERENCE_GRID_FADE_NONE` disables distance fade.

`DVZ_REFERENCE_GRID_FADE_GRID` measures fade from the grid origin in the grid plane.

`DVZ_REFERENCE_GRID_FADE_VIEW` measures fade from the camera or view position. This supports a
world-snapped grid whose fade follows the camera, and it also supports view-adaptive grids used by
fly-camera examples.


## Important Combinations

Current finite reference grid:

```c
extent = DVZ_REFERENCE_GRID_EXTENT_FIXED;
fade = DVZ_REFERENCE_GRID_FADE_NONE;
```

Static world-snapped horizon grid:

```c
extent = DVZ_REFERENCE_GRID_EXTENT_HORIZON;
fade = DVZ_REFERENCE_GRID_FADE_GRID;
```

World-snapped grid with camera-relative fade:

```c
extent = DVZ_REFERENCE_GRID_EXTENT_HORIZON;
fade = DVZ_REFERENCE_GRID_FADE_VIEW;
```

Long-distance fly-camera ground grid:

```c
extent = DVZ_REFERENCE_GRID_EXTENT_VIEW_ADAPTIVE;
fade = DVZ_REFERENCE_GRID_FADE_VIEW;
```


## Implementation Staging

Stage 1 adds the public descriptor fields and implements `FIXED`, `HORIZON`, `FADE_NONE`, and
`FADE_GRID` on the retained segment path.

Stage 2 implements `FADE_VIEW` only after the scene aid can access the required camera or view state
without leaking backend details into the public API.

Stage 3 implements `VIEW_ADAPTIVE` only after the scene/controller invalidation path for camera
changes is defined. Generated geometry must be snapped to `spacing` to avoid visual swimming.


## Non-Goals

Do not restore the v0.3 `dvz_panel_grid()` visual API.

Do not describe static finite geometry as truly infinite.

Do not add a parallel presentation, frame-stream, renderer, or Vulkan-wrapper path outside the
v0.4 scene to DRP2 to vklite runtime.
