# Soon Queue

This directory holds imminent implementation plans that should stay visible, but should not make
`agents/now/` noisy.

Use this directory for work expected soon, including WebGPU/WASM, depth peeling, screen-space
volume occlusion, graph-backed screen-space effects, scene techniques, visual-family expansion,
text/layout implementation slices, and controller work.
Stable scene semantics and example analysis belong under `spec/scene/`; keep only the executable
implementation plan here.

Text shaping, layout, atlas, cache, and DRP2 emission contracts belong in
[../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md](../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md).

When a lane becomes the active branch focus, link it from `agents/now/NEXT_STEPS.md`. When it
lands, move the implementation record to `agents/done/`.
