# Handoff: Colorbar Explicit Ticks API

Status: completed historical handoff. Updated: 2026-07-05

## Resolution

The original WIP landed in `0be4b0da8`, and the known crash was resolved by
`fb6a94718 Fix explicit colorbar tick handling`. The API is now part of the active v0.4 surface and
has since been migrated through the public API cleanup branch.

Related commits:
  - `20be4d8ea Fix glyph rotation angle convention`
  - `7a864688c Use dark colorbar adornments`
  - `0be4b0da8 WIP add explicit colorbar ticks API`
  - `fb6a94718 Fix explicit colorbar tick handling`

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

## Historical Validation State

Completed:

- `cmake --build build --target dvztest_scene -j4` built successfully.
- `git diff --check` was clean before the WIP commit.
- `python -m py_compile tools/bindings/generate_array_facade.py tools/bindings/array_facade_smoke.py`
  was clean before the WIP commit.

Resolved after this handoff was first written:

- Focused runtime colorbar tests no longer carry the recorded code-139 crash after
  `fb6a94718`.
- Later API cleanup validation converted `dvz_colorbar_set_ticks()` and
  `dvz_colorbar_clear_ticks()` to `DvzResult` and kept generated `ctypes`/C docs in sync.

## Current Pickup

No Datoviz-side pickup remains for the original colorbar explicit-ticks WIP. If colorbar tick
behavior changes again, validate with:

```bash
./build/testing/dvztest_scene --module scene --group fields --case test_scene_colorbar_explicit_ticks_and_labels
```

Also verify whether the test filter syntax was selecting the intended case; the command-line filters
accepted by `dvztest_scene --help` include `--module`, `--group`, and `--case`.

## Do Not Lose

There are unrelated pre-existing Datoviz worktree changes:

- `M NOTES`
- `m data`

They were intentionally not committed in `0be4b0da8`.
