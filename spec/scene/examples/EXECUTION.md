# Example Overhaul Execution Plan

> **Status:** active execution guide
> **Scope:** final v0.4 C example migration, old-example handling, style consistency, and
> per-example workflow
> **Use with:** [PLANNING.md](PLANNING.md), [ORGANIZATION.md](ORGANIZATION.md),
> [STYLE.md](STYLE.md), and [POLICIES.md](POLICIES.md)


## Decisions

1. Work one example at a time, in the pickup order from [PLANNING.md](PLANNING.md).
2. Do a small structure and metadata pass before broad rewrites, but do not polish many examples in
   parallel.
3. The final public C layout should follow the lane model from [ORGANIZATION.md](ORGANIZATION.md):
   `fundamentals`, `visuals`, `features`, `techniques`, `showcases`, `runtime`, and `advanced`
   or equivalent low-level lanes.
4. Old v0.4-dev examples are source material, not the final catalog. Promote only examples that
   map to a scenario ID, visual-family requirement, feature requirement, runtime proof, technique,
   regression, or stress target.
5. Do not delete temporary examples during the first migration pass unless they are duplicated by a
   validated replacement and no longer serve as regression input. Prefer quarantining or
   de-indexing first.
6. Gallery-critical examples need explicit style and capture metadata before broad documentation
   migration.


## Old Example Handling

Classify each existing file before moving or rewriting it.

| Class | Action |
| --- | --- |
| Promoted release proof | Move or rewrite into the final lane and attach the canonical scenario ID. |
| Visual-family baseline | Keep under `examples/c/visuals/` only if it demonstrates one active visual family with minimal unrelated features. |
| Feature proof | Move from temporary `techniques` or `annotations` folders into `examples/c/features/` when the example is about axes, colorbar, scale bar, annotation, overlay, picking, probing, selection, panels, updates, or controllers. |
| Rendering technique | Keep under `examples/c/techniques/` only when the main subject is a pass-level behavior such as EDL, SSAO, MSAA, WBOIT, depth peeling, depth cueing, or materials. |
| Showcase | Keep under `examples/c/showcases/` only when it is composed, polished, and gallery-facing. Rename the current singular `showcase` lane when doing the structural pass. |
| Runtime or low-level tool | Move to `examples/c/runtime/`, `examples/c/advanced/`, `examples/c/drp2/`, or keep in a clearly non-gallery `tools` lane. |
| Temporary lab or diagnostic | Keep buildable but de-index from public gallery metadata; move to regression/diagnostic/lab only if it remains useful. |
| Superseded duplicate | Remove only after the replacement builds, runs, and has equivalent or better validation. |

The current useful promotion candidates include `protein.c`, `lidar.c`, `brain.c` or `ibl_brain.c`,
`labels.c`, `scatter_axes.c`, `image_probe.c`, `linked_panels.c`, scale-bar examples, visual-family
baselines, and WebGPU fixtures. Treat names as historical clues; the scenario ID and release role
decide the final location.


## Shared Style Baseline

Use [STYLE.md](STYLE.md) as the visual source of truth. The default for final examples is:

1. `graphite_cyan` palette unless the example is explicitly publication-style 2D;
2. no gradient backgrounds for visual-family examples, feature examples, fixtures, or regression
   captures;
3. solid neutral backgrounds, deterministic seeds, deterministic camera poses, and stable data
   ranges;
4. cyan for hover/probe, amber for selection, rose for error/conflict;
5. visual baselines captured at `1280x960`;
6. showcases captured at `1600x1000` or another documented 16:10/16:9 size;
7. real or realistic data for showcases, tiny synthetic data for fixtures and baselines.


## Minimal Metadata

Every migrated public example should have metadata, either in a central manifest or near the
example while the manifest is being built:

```yaml
id: point_2d
title: 2D Points
lane: visuals
status: required
source: examples/c/visuals/point.c
style_preset: visuals
palette: graphite_cyan
capture: 1280x960
seed: 12345
interaction_state: default
validation: smoke+screenshot
```

Add fields only when they are useful for gallery generation, release checks, or documentation
links. Do not let metadata become a second specification for behavior already defined in canonical
scene/API specs.


## Per-Example Loop

Use the same loop for each promoted example.

1. **Choose the scenario or lane target.** Start from [PLANNING.md](PLANNING.md) for required
   scenarios and [EXAMPLE_COVERAGE.md](../../docs/EXAMPLE_COVERAGE.md) for visual/feature
   coverage.
2. **Classify the old example.** Decide whether to promote, rewrite, quarantine, keep as a tool, or
   remove after replacement.
3. **Define the minimal target.** Write down the smallest useful scene, data shape, interaction
   state, and validation result before editing code.
4. **Apply the style preset.** Set background, palette, camera, seed, capture size, colormap, and
   interaction colors consistently.
5. **Implement or migrate the example.** Keep one public idea per file unless it is explicitly a
   showcase. Use shared example helpers only for repeated non-public chores.
6. **Make it smoke-testable.** Prefer bounded frame counts, offscreen/capture paths, deterministic
   data, and explicit optional-backend gates.
7. **Capture or validate.** For gallery examples, produce or verify a nonblank capture. For
   interaction examples, include a bounded manual or automated check.
8. **Update metadata and indexes.** Add scenario ID, lane, status, source path, style preset,
   validation command, and any deferred pieces.
9. **Run the narrowest validation.** At minimum run `git diff --check` and the relevant
   `just example-c <name>` or focused build/test path.
10. **Record follow-ups.** Defer missing polish explicitly instead of hiding it in code comments or
    leaving the example half-promoted.


## Initial Pickup Order

Use this order for the required public proof set:

1. `point_2d`
2. `path_axes_2d`
3. `linked_panels_axes_panzoom`
4. `scale_bar`
5. `image_probe`
6. `linked_panels_probe_colorbar`
7. `marker_picking`
8. `volume`
9. `protein_arcball_viewer`
10. `showcase_wind_field`
11. `textured_terrain_or_planet`
12. `brain_volume_mesh`
13. `dense_point_cloud_edl`
14. `webgpu_browser_subset`


## Stop Conditions

Stop and update the plan before continuing if:

1. a required example exposes a missing public API or runtime feature;
2. the old example cannot be preserved without changing its behavior;
3. the example needs a downloaded or generated asset without a recorded source, license, size
   budget, and cache path;
4. the required validation is graphics-backend-dependent and cannot be reproduced locally;
5. the proposed migration would delete useful diagnostic coverage before a replacement exists.
