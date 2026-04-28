# Example: Path With Live 2D Axes

This example combines grouped path data with axis regeneration under panzoom.


## Owning Specs

This example should be read against:

1. `../VISUAL_FAMILY_RULES.md` for the `path` family contract,
2. `../AXES.md` for axis derivation and regeneration policy,
3. `../pipeline/TRANSFORM_PIPELINE.md` for normalization versus panel-local navigation,
4. `../pipeline/FRAME_PLAN.md` for the scene-level `FramePlan` contribution shape.


## Scene Setup

1. one scene,
2. one 2D panel,
3. one `path` visual with grouped sequences,
4. x and y axes attached to the panel,
5. one panzoom controller.


## Family And Variant

Primary visual family:

1. `path`

Axis-related scene objects:

1. x axis,
2. y axis.

Variant axes:

1. open path mode,
2. standard line styling,
3. live axis tick regeneration.


## Resource Schema Instance

Path resources:

1. source `GroupedItemTable` in `DataSpace`,
2. derived normalized `GroupedItemTable` in `VisualSpace`,
3. `StyleBlock` for linewidth, joins, and caps.

Axis-derived resources:

1. axis line and tick contributions expressed through `segment`-like derived data,
2. label contributions expressed through `glyph`-like derived data.


## Transform Pipeline

For the path visual:

1. grouped source samples live in `DataSpace`,
2. they are normalized into visual-ready grouped path coordinates,
3. panzoom acts afterward.

For axes:

1. tick values are selected in `DataSpace`,
2. tick anchors are mapped into `VisualSpace`,
3. tick and label geometry are rebuilt when needed,
4. panzoom still moves the resulting axis geometry afterward.

The key distinction is:

1. path data usually survives panzoom unchanged,
2. axes may need semantic regeneration under panzoom.


## FramePlan Shape

Frame after only panzoom changes:

1. no upload for path geometry unless data changed,
2. regenerated axis derived resources if visible ticks changed,
3. one `RenderNode` containing path, axis lines, and labels, or multiple render nodes if layering
   policy prefers separation.

Frame after path data changes:

1. `UploadNode` for normalized path data,
2. possible axis regeneration if domain bounds changed,
3. render node(s) for path and axis contributions.


## DRP2 Categories Implied

1. resource writes for path and axis-derived resources when dirty,
2. render-pass lifecycle,
3. draw commands for path, segments, and glyphs,
4. queue submission.


## Pressure On The Spec

This example checks that:

1. grouped resources behave correctly,
2. axes are semantic scene objects rather than a primitive family,
3. data normalization stays separate from panzoom,
4. panzoom can invalidate axis layout without invalidating all visual data,
5. axis-derived uploads remain scoped to plan-visible dirty resources rather than ad hoc execution
   work.
