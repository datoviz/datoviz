# Protein Ribbon Mesh Improvement Plan

> **Execution Status**
> - **Status:** `PLANNED`
> - **Updated on:** `2026-05-17`
> - **Purpose:** improve the generated protein cartoon/ribbon mesh used by
>   `examples/c/showcase/protein.c` without creating a new renderer path.

## Context

The protein showcase currently renders ribbon geometry produced by `tools/preprocess_protein.py`
as a regular `dvz_mesh()` visual. The prepared `1UBQ` bundle uses:

1. `ribbon_samples_per_segment = 16`
2. `ribbon_cross_section_count = 24`
3. a generic swept elliptical cross-section for coils, helices, sheets, and turns

The screenshot that motivated this plan shows visible blockiness in the red ribbon body. Raising
tessellation helps, but field practice in molecular viewers is not only "more triangles":
cartoon renderers typically combine secondary-structure-specific shapes, smoothed centerlines,
smoothed frame/orientation fields, beta-strand flattening, and carefully matched normals.

Relevant reference points from common viewers:

1. PyMOL exposes `cartoon_sampling`, `cartoon_refine`, smoothing controls, and per-shape quality
   knobs such as tube/oval/loop quality.
2. ChimeraX exposes ribbon/tube subdivision and cartoon `sides`; examples use higher `sides`
   values for smoother tubes, while centerline/ribbon divisions are also adapted to structure size.
3. VMD NewCartoon exposes representation resolution and spline style; smoother splines may be
   preferable for presentation even when they do not pass exactly through every C-alpha.

## Goals

1. Keep the active scene -> mesh -> DRP2 -> vklite path unchanged.
2. Improve the CPU-side generated ribbon mesh in `tools/preprocess_protein.py`.
3. Preserve fast interactive defaults for large proteins while offering a high-quality preset for
   showcase screenshots and videos.
4. Make generated normals describe the final emitted shape.
5. Add focused metadata and validation so bundle quality choices are explicit and reproducible.

## Non-Goals

1. Do not add a dedicated protein renderer or special scene visual in this slice.
2. Do not activate text/gui/color/wasm scaffolding.
3. Do not require external molecular libraries at runtime.
4. Do not make DSSP mandatory; keep a fallback path when secondary structure is unavailable.

## Proposed Pipeline

Replace the current direct swept-oval path with an explicit preprocessing pipeline:

```text
PDB atoms
 -> residues and optional secondary structure
 -> C-alpha centerline control points per chain
 -> sampled/smoothed centerline
 -> per-sample secondary-structure state
 -> smoothed frame field
 -> secondary-structure-specific cross-section
 -> optional sheet flattening and helix stabilization
 -> positions, normals, colors, indices, metadata
```

## Implementation Plan

### 1. Move Width Scaling Into Preprocessing

Status: highest-priority first fix.

The example currently calls `_protein_bundle_widen_ribbon()` after loading the generated mesh, but
the stored normals still describe the pre-widened oval. Move the width scaling into
`tools/preprocess_protein.py` and remove the C-side widening helper.

Tasks:

1. Add `--ribbon-width-scale`, defaulting to the current visual scale of `1.75`.
2. Apply the scale before writing `ribbon_position.f32` and `ribbon_normal.f32`.
3. Recompute analytic normals from the final cross-section dimensions.
4. Record `ribbon_width_scale` in `metadata.json`.
5. Remove `_protein_bundle_widen_ribbon()` and `PROTEIN_RIBBON_WIDTH_SCALE` from
   `examples/c/showcase/protein.c`.

Expected result: lighting and silhouette describe the same shape.

### 2. Add Quality Presets

Add a simple quality selector to make the intended tessellation clear.

Suggested presets:

| Preset | Samples per residue segment | Cross-section count | Intended use |
| ------ | --------------------------- | ------------------- | ------------ |
| `fast` | 12                          | 16                  | large proteins, quick interaction |
| `default` | 24                       | 24                  | normal showcase use |
| `high` | 32                          | 32                  | captures, close-ups, video |

Tasks:

1. Add `--ribbon-quality {fast,default,high,custom}`.
2. Keep `--ribbon-samples` and `--ribbon-cross-section-count` for custom/manual overrides.
3. Update metadata with the selected quality and resolved numeric values.
4. Regenerate the committed `1UBQ` bundle with the `default` preset unless performance argues for
   `fast`.

### 3. Smooth The Centerline

The current Catmull-Rom path passes through C-alpha points. That is simple and faithful, but close
views can reveal local bends and residue-to-residue wobble.

Tasks:

1. Add an optional smoothing pass over per-chain C-alpha control points before sampling.
2. Keep endpoints stable so chains do not visibly shrink.
3. Use a small default smoothing strength for coils/turns and a stronger but bounded smoothing for
   helices and sheets.
4. Record smoothing settings in metadata.

Implementation notes:

1. Start with a conservative Laplacian-style smoothing pass over C-alpha controls.
2. Keep a `--ribbon-centerline-smoothing` scalar so this remains easy to tune.
3. Consider a B-spline-like path later if the conservative pass is not enough.

### 4. Smooth The Frame Field

Parallel-transported frames avoid wild twisting, but neighboring section frames can still change
abruptly around tight turns. Smooth the orientation field after sampling and before emitting
cross-sections.

Tasks:

1. Store sampled `center`, `tangent`, `side`, and `up` before geometry emission.
2. Smooth `side/up` vectors over a small window, reprojecting onto the tangent plane after every
   pass.
3. Flip signs before averaging when needed so adjacent frames point in the same orientation.
4. Re-orthonormalize `side`, `up`, and `tangent` after smoothing.

Expected result: fewer shape discontinuities around tight turns and smoother specular flow.

### 5. Add Secondary-Structure-Specific Shapes

The current ribbon uses one generic swept oval whose width/thickness changes by secondary
structure. Improve this without adding a new renderer.

Tasks:

1. Keep coil/turn as a round or soft oval tube.
2. Use a wider, flatter oval or rounded slab for sheets.
3. Use a smooth oval/tube for helices, with stabilized orientation along the helix axis when
   consecutive residues form a helix run.
4. Keep shape transitions gradual across a few samples at secondary-structure boundaries.
5. Defer arrowhead geometry for sheets unless the basic slab reads poorly.

Implementation notes:

1. First implement this as per-sample width/thickness/profile parameters.
2. Add a `profile` enum internally only if oval/slab/tube behavior diverges enough to justify it.
3. Preserve the existing `ribbon_color_ss.rgba8` and `residue_secondary_structure.u8` outputs.

### 6. Flatten Beta Strands

Beta sheets should not follow every C-alpha ripple. Add a modest flattening pass for sheet runs.

Tasks:

1. Detect contiguous sheet runs per chain from `residue.ss == SS_SHEET`.
2. Estimate a stable sheet plane/frame per run.
3. Blend sampled sheet positions and section `up/side` frames toward that plane.
4. Keep boundary blending gradual to avoid visible kinks at sheet/coil transitions.

Expected result: sheet ribbons read as cleaner, flatter slabs instead of noisy tubes.

### 7. Add Normal Generation Modes

Analytic oval normals are good for smooth tubes, but mesh-derived normals can be more robust after
slabs, transitions, flattening, or arrowheads.

Tasks:

1. Keep analytic normals as the default for oval/tube profiles.
2. Add an internal mesh-normal computation helper that accumulates area- or angle-weighted triangle
   normals per vertex.
3. Compare analytic and mesh-derived normals for slab/sheet profiles.
4. Add `--ribbon-normal-mode {analytic,mesh}` if both modes remain useful.

## Validation

Focused validation should cover both generated data and runtime rendering:

1. Regenerate `data/examples/proteins/1ubq/prepared`.
2. Check metadata values for quality, width scale, samples, cross-section count, vertex count, and
   index count.
3. Run `just build`.
4. Run the protein smoke example with a bounded frame count:
   `./build/examples/c/showcase/protein data/examples/proteins/1ubq/prepared 60`.
5. Run at least one focused scene/app test if the example code changes; use the narrowest relevant
   target before broad validation.
6. Run `git diff --check` before finalizing.

Visual acceptance criteria:

1. The close-up red ribbon body has no obvious longitudinal stepping at the default quality preset.
2. Specular highlights flow smoothly across turns instead of revealing frame discontinuities.
3. Sheet/helix/coil shapes remain readable at normal camera distance.
4. Large proteins remain usable with the `fast` preset.

## Suggested First Patch

Start with a deliberately narrow change:

1. Add preprocessing-side `--ribbon-width-scale`.
2. Change default samples from `16` to `24`.
3. Leave cross-section count at `24`.
4. Remove C-side widening.
5. Regenerate only `1UBQ`.

This should address the most likely current artifact while keeping risk low. Broader
centerline/frame/secondary-structure changes can follow once the shape/normal mismatch is gone.
