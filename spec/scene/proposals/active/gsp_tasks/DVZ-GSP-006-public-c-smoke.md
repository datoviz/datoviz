# DVZ-GSP-006: Public C GSP Smoke

## Goal

Add a narrow public C smoke scenario that exercises the first GSP-compatible Datoviz path:
scene/figure/panel creation, image background, point overlay, offscreen render, PNG capture, point
query, image-only query, and Datoviz id round-trip.

## Files To Inspect/Change

| File | Reason |
|---|---|
| `src/scene/tests/app.c` | offscreen app tests and query execution through the app runtime |
| `src/scene/tests/test_scene.h` | test declaration |
| `include/datoviz/app.h` | public offscreen render and capture APIs |
| `include/datoviz/scene.h` | public constructors, visual uploads, id getters |
| `include/datoviz/scene/interaction.h` | public query request/result path |

## Non-Goals

1. Do not create a GSP adapter inside Datoviz.
2. Do not depend on private scene internals for assertions.
3. Do not add a broad visual-family integration suite.
4. Do not require native GPU availability beyond the existing app-offscreen test fixture policy.

## Implementation Notes

The smoke should:

1. create a 64x64 offscreen view;
2. add a red image background and a yellow point overlay;
3. query the center for `DVZ_SCENE_TARGET_ITEM` and expect the point visual;
4. query an image-only coordinate for `DVZ_SCENE_TARGET_PIXEL` and expect the image visual;
5. assert `scene_id`, `figure_id`, `panel_id`, and `visual_id` round-trip through
   `DvzQueryResult`;
6. call `dvz_view_capture_png()` to cover the adapter-facing raster capture path.

## Tests/Validation

1. `just build`
2. `build/testing/dvztest_scene scene/app-offscreen/gsp_first_slice_smoke`
3. `git diff --check`

## Stop Conditions

1. The smoke requires private scene internals to pass.
2. Image pixel query support is unstable on the active offscreen backend.
3. PNG capture creates a repository-tracked binary artifact.
