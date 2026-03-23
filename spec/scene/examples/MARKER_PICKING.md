# Example: Marker Scatter With Picking

This example instantiates a richer point-like family with a picking path.


## Scene Setup

1. one scene,
2. one 2D panel,
3. one `marker` visual,
4. picking enabled for the panel,
5. one pointer-driven interaction loop.


## Family And Variant

Family:

1. `marker`

Variant axes:

1. marker-shape mode enabled,
2. edge styling enabled,
3. picking-enabled render path.


## Resource Schema Instance

Scene-facing resources:

1. source `ItemTable` in `DataSpace` with position and marker semantics,
2. derived normalized `ItemTable` in `VisualSpace`,
3. `StyleBlock` for edge width, edge color, and scaling policy,
4. panel-local picking `DerivedField`,
5. picking `ReadbackTarget`.


## Transform Pipeline

1. marker positions originate in `DataSpace`,
2. scene normalization maps them into point-like marker anchors in `VisualSpace`,
3. marker shape and edge semantics remain family-level parameters,
4. panel panzoom applies after normalization,
5. the picking path uses the same panel-local viewing transform.


## FramePlan Shape

Typical frame without hover change:

1. optional `UploadNode` if marker data or style changed,
2. one `RenderNode` for the visible color pass,
3. one `RenderNode` for the picking pass when picking is active,
4. one `ReadbackNode` for the picked pixel when needed.


## DRP2 Categories Implied

1. resource writes for dirty marker data or styles,
2. render-pass lifecycle for color and picking passes,
3. draw commands,
4. copy or readback service path for the picking result,
5. queue submission.


## Pressure On The Spec

This example checks that:

1. `marker` remains distinct from `point`,
2. picking is modeled semantically and not as backend leakage,
3. visual identity and item identity survive the readback path,
4. family-level style semantics do not require a separate family split.
