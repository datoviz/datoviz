# Handoff: Colorbar Explicit Ticks API

Updated: 2026-06-26

## Current Branch And Commit

- Branch: `v0.4-dev`
- Pushed WIP commit: `0be4b0da8 WIP add explicit colorbar ticks API`
- Prior related commits:
  - `20be4d8ea Fix glyph rotation angle convention`
  - `7a864688c Use dark colorbar adornments`

## What Was Added

The WIP commit adds the public colorbar tick API shape recommended by the GSP ChatGPT Pro
consultation:

- `DvzColorbarTicks`
- `dvz_colorbar_ticks()`
- `dvz_colorbar_set_ticks(DvzColorbar*, const DvzColorbarTicks*)`
- `dvz_colorbar_clear_ticks(DvzColorbar*)`

The retained colorbar now has explicit tick storage:

- `explicit_ticks_enabled`
- `explicit_tick_labels_set`
- `explicit_tick_count`
- `explicit_ticks[]`
- `explicit_tick_labels[][]`

The renderer resolves explicit ticks before automatic ticks and uses explicit labels when provided.
The Python array facade generator has a `dvz_colorbar_set_ticks(colorbar, values, labels=None)`
wrapper mirroring the axis tick wrapper.

## Validation State

Completed:

- `cmake --build build --target dvztest_scene -j4` built successfully.
- `git diff --check` was clean before the WIP commit.
- `python -m py_compile tools/bindings/generate_array_facade.py tools/bindings/array_facade_smoke.py`
  was clean before the WIP commit.

Not complete:

- Focused runtime test `./build/testing/dvztest_scene scene/fields/colorbar` exited with code 139.
- The same crash reproduced for a narrower colorbar filter.
- This was not debugged because the user requested an immediate WIP commit and push.

## Known Issue To Start With

Debug the `dvztest_scene` segfault before considering the API done. Start by running the new case
under `lldb` or with ASAN:

```bash
./build/testing/dvztest_scene --module scene --group fields --case test_scene_colorbar_explicit_ticks_and_labels
```

Also verify whether the test filter syntax was selecting the intended case; the command-line filters
accepted by `dvztest_scene --help` include `--module`, `--group`, and `--case`.

## Next Steps

1. Fix the colorbar test crash.
2. Regenerate ctypes/raw and array facade outputs if required by the binding workflow.
3. Run:
   - `cmake --build build --target dvztest_scene -j4`
   - `./build/testing/dvztest_scene --module scene --group fields --case test_scene_colorbar_explicit_ticks_and_labels`
   - `./build/testing/dvztest_scene --module scene --group fields`
   - Python array facade smoke after regenerated bindings expose `DvzColorbarTicks`.
4. Once Datoviz is green, update GSP to call `dvz_colorbar_set_ticks()` for explicit colorbar
   ticks/labels and regenerate the S029 review pack.

## Do Not Lose

There are unrelated pre-existing Datoviz worktree changes:

- `M NOTES`
- `m data`

They were intentionally not committed in `0be4b0da8`.
