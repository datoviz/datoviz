# Scene Colorbar Rendering Slice

> **Status:** completed first rendered slice
> **Completed on:** 2026-05-21
> **Commits:** `7ce325d4`, `59820908`, `f9e80cf0`, `24c13eff`, `54ccd3ec`, `9275d97a`

## Summary

The first continuous colorbar rendering slice is implemented on the active scene -> DRP2 path.
Colorbars are retained panel-owned explanatory objects bound to `DvzScale` and `DvzColormap`.

Implemented behavior:

1. panel-attached colorbar creation with orientation, anchor, title, and format setters,
2. deterministic same-panel edge reserve for left/right/top/bottom anchors,
3. continuous ramp realized as an internal primitive visual,
4. tick marks realized as an internal segment visual,
5. tick labels and title realized through the existing text/glyph path,
6. built-in colormap ramp sampling for the active built-in maps,
7. scale-domain, title, orientation, anchor, and colormap update coverage,
8. destroy behavior that hides derived visuals and detaches the colorbar from its panel,
9. DRP2 stream coverage for ramp uploads/draw, tick draw, and glyph-derived text work,
10. `DvzDiagnosticReport` entries for invalid realization such as non-increasing domains.

## Validation

Recorded validation from the landing pass:

1. `just build`
2. `git diff --check`
3. `./build/testing/dvztest_scene colorbar` -> `7/7 tests passed`
4. `./build/testing/dvztest_scene axis` -> `16/16 tests passed`
5. `./build/testing/dvztest_scene text` -> `26/30 tests passed`, `4` Vulkan-dependent tests
   skipped because Vulkan instance creation failed in the local environment.

## Remaining Follow-Ups

Later work should not reopen the first colorbar slice. Treat these as follow-up lanes:

1. app/offscreen visible colorbar smoke fixtures once Vulkan runtime is available,
2. shared or consolidated colorbars in grid-level fixed-size slots,
3. categorical legend support through a dedicated legend slice,
4. richer scale semantics such as log transforms and interactive range editing,
5. visual-family pressure beyond image/volume scalar mapping where examples need it.
