# Soon Queue

This directory holds imminent follow-up notes that should stay visible, but should not make
`agents/now/` noisy.

Use this directory for work expected soon, including WebGPU/WASM, depth peeling, screen-space
volume occlusion, graph-backed screen-space effects, scene techniques, visual-family expansion,
text/layout implementation slices, and controller work.
Stable scene semantics and example analysis belong under `spec/scene/`; keep only executable
pickup order, validation notes, and unresolved implementation choices here.

Text shaping, layout, atlas, cache, and DRP2 emission contracts belong in
[../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md](../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md).

## Directory Buckets

1. [runtime](runtime/): app resource injection, DRP2 diagnostics, WebGPU, and WASM runtime lanes.
2. [tooling](tooling/): test-runner scheduling and automation follow-ups.
3. [text-layout](text-layout/): text, glyph, atlas/cache, and shaping follow-up notes.
4. [scene](scene/): scene visual and layout follow-up plans.
5. [interaction](interaction/): controller and camera interaction follow-up plans.
6. [effects](effects/): render techniques, transparency, SSAO, MSAA, and screen-space effects.

When a lane becomes the active branch focus, link it from `agents/now/NEXT_STEPS.md`. When it
lands, remove the plan from this queue, move or rewrite the final implementation record under
`agents/done/`, and delete stale links from the relevant `soon/` README files.
