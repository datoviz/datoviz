# First-Class Guide Snapshot Handoff

Status: landed on `v0.4-dev` after implementation on branch `agent/M184-first-class-guides`.

This handoff covers the M184 Datoviz-side work for GSP S043: first-class guide layout, hit, and
rendered-contribution readback from `DvzPanelFrameSnapshot`.

## Branch

- Base: `f4f8807b3` (`Updating NOTES`)
- Implementation commit on `v0.4-dev`: `7b3bd2f7f`
- Worktree: `/Users/cyrille/GIT/Viz/datoviz-agent-worktrees/M184-first-class-guides`
- Branch: `agent/M184-first-class-guides`

The main Datoviz checkout can now be fast-forwarded to `origin/v0.4-dev` to pick up this handoff
and implementation.

## Implemented

- Added public guide snapshot enums and records in `include/datoviz/scene/types.h`:
  - `DvzGuideKind`
  - `DvzGuideRole`
  - `DvzGuidePart`
  - `DvzRenderedContributionKind`
  - `DvzGuideLayout`
  - `DvzGuideHit`
  - `DvzRenderedContribution`
- Added public frame snapshot API in `include/datoviz/scene.h`:
  - `dvz_panel_frame_guide_count()`
  - `dvz_panel_frame_guide_layout()`
  - `dvz_panel_frame_guide_hit()`
  - `dvz_panel_frame_contribution_count()`
  - `dvz_panel_frame_contribution()`
- Extended `src/scene/core/panel_frame_snapshot.c` so snapshots own immutable guide/contribution
  records.
- Populated records from retained state for:
  - axes, axis grids, tick labels, and axis labels;
  - guide lines and guide spans;
  - colorbar text/ramp/tick generated visuals;
  - legend text/mark generated visuals.
- Regenerated `datoviz/_ctypes.py`.
- Added `scene/scene-graph/panel_frame_snapshot_guide_layouts`.

## Current Semantics

Snapshot geometry is reported in figure logical pixels.

Axis, colorbar, and legend text boxes use retained logical-pixel text layout positions and coarse
text extents. They do not yet use exact renderer glyph metrics. The snapshot diagnostic now records
that limitation as `guide_layout_snapshot_first_slice`.

Guide line/span boxes are derived from the snapshot visible/source data domain and plot rectangle.
Generated visual ids are used as durable guide ids where no separate public retained object id
exists yet.

`dvz_panel_frame_guide_hit()` scans snapshot guide records in reverse insertion order and returns
the topmost matching box in the snapshot, with the same `snapshot_id` as the layout and
contribution records.

## Validation Run

From this worktree:

```sh
DVZ_CMAKE_ARGS='-DDVZ_BUILD_GUI=OFF -DDVZ_WITH_GLFW=OFF -DDVZ_WITH_MSDF_ATLAS=OFF' just configure
git submodule update --init data
git submodule update --init --recursive external/cimgui
cmake --build build --target dvztest_scene
cmake --build build --target datoviz
./build/testing/dvztest_scene scene/scene-graph/panel_frame_snapshot_guide_layouts
./build/testing/dvztest_scene scene/scene-graph/panel_frame_snapshot_core
./build/testing/dvztest_scene scene/scene-graph/panel_view3d_state_readback
./build/testing/dvztest_scene scene/axis/panel_view2d
./build/testing/dvztest_scene scene/fields
./build/testing/dvztest_scene scene/scene-graph
just ctypes
just ctypes-check
git diff --check
```

Results:

- `scene/scene-graph`: 203/203 passed.
- `scene/fields`: 55/55 passed.
- `just ctypes-check`: policy, array facade, and ABI validation passed.
- `git diff --check`: passed.

## Follow-Up for GSP

The next GSP mission should use the new ctypes APIs instead of synthesizing guide boxes:

1. Detect the new functions and records in the Datoviz backend adapter.
2. Map Datoviz frame info plus guide layout/contribution records into the GSP partial snapshot path.
3. Promote guide strictness only for rows with layout, hit/readback, contribution, and matching
   `snapshot_id` evidence.

Do not equate native grid clipping with full guide strictness. Grid clipping is still a separate
capability unless guide identity, boxes, hits, and contributions are all present for the row.

## Stop Signs

- Stop if exact glyph metrics are required; this branch intentionally exposes coarse retained text
  boxes only.
- Stop if a caller needs stable non-visual guide ids for axes before a public retained guide object
  model exists.
- Stop if GSP strict promotion would require inventing guide/contribution records outside the
  Datoviz snapshot.
