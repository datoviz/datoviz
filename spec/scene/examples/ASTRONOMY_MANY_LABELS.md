# ASTRONOMY MANY LABELS

## Purpose

This example pressure-tests the scene text stack with a realistic astronomical finder-chart
workflow: a dense Gaia star field rendered as points, with thousands of small source labels anchored
to visible stars.

The goal is not to make a pretty planetarium view. The goal is to expose the cost of rendering,
updating, culling, and interacting with many short-but-real scientific labels in a workload where
label density, overlap, pan/zoom, and stable screen-space placement all matter.

This should become one of the main acceptance examples for a future `TextVisual` or label visual
because the text load is large enough to make naive CPU layout, per-label draw calls, and repeated
glyph uploads fail quickly.


## Owning specs and contracts

This worked example exercises:

1. `LATEX_MICROTEX_TEXT_VISUAL.md` for glyph atlas, text placement, glyph instances, and mixed
   world-anchor/screen-size text.
2. `POINT_2D.md` for the base star scatter layer.
3. `LINKED_PANELS_AXES_PANZOOM.md` for 2D navigation semantics.
4. `MARKER_PICKING.md` for source hover/selection behavior.
5. The scene -> `FramePlan` -> DRP2 -> runtime path for retained visual updates.

It is an informative pressure test, not a separate normative API source.


## Scientific use case

Astronomers routinely inspect sky cutouts with catalog overlays:

- source identifiers next to detected stars;
- magnitude labels for selected brightness ranges;
- color or class labels for special objects;
- proper-motion arrows or highlighted selected sources;
- hover readouts that report catalog fields.

The concrete scenario here is a Gaia DR3 field viewer. The user pans and zooms through a dense
sky region, toggles label modes, and inspects individual sources by Gaia `source_id`.

The important property is that the labels are not decorative. They are part of the scientific
inspection workflow: labels connect plotted points to catalog rows.


## Dataset

Primary dataset: **Gaia DR3 `gaiadr3.gaia_source`** queried through the ESA Gaia Archive TAP
service.

Reference links:

- Gaia DR3 dataset DOI page: `https://esdcdoi.esac.esa.int/doi/html/data/astronomy/gaia/DR3.html`
- Gaia DR3 documentation: `https://gea.esac.esa.int/archive/documentation/GDR3/`
- Gaia Archive: `https://gea.esac.esa.int/archive/`
- ESA Gaia programmatic TAP example:
  `https://www.cosmos.esa.int/web/gaia-users/archive/programmatic-access`

Use a bounded cone or rectangular region rather than an all-sky export. Good initial fields:

- **Pleiades**: visually recognizable, moderate source density, good for presentation.
- **Galactic plane patch**: very dense and better for stress testing.
- **Large Magellanic Cloud patch**: dense field with a strong scientific visual identity.

The preparation step should query only the columns needed by the C demo:

```sql
SELECT TOP 250000
    source_id,
    ra,
    dec,
    phot_g_mean_mag,
    bp_rp,
    parallax,
    pmra,
    pmdec
FROM gaiadr3.gaia_source
WHERE 1 = CONTAINS(
    POINT('ICRS', ra, dec),
    CIRCLE('ICRS', 56.75, 24.12, 2.0)
)
AND phot_g_mean_mag IS NOT NULL
ORDER BY phot_g_mean_mag ASC
```

The coordinates above are an illustrative Pleiades-centered field. The preparation script may offer
named fields and generate different cache directories for each field.


## Datoviz-ready cache

The interactive C example must not depend on network access, Python astronomy packages, FITS/VOTable
parsing, or a TAP client. Those belong in a preparation step.

Store the prepared binary cache under:

```text
.cache/datoviz-astronomy/gaia_dr3/<field_name>/
  metadata.json
  positions_f32.bin        # N x 3, normalized panel/world coordinates
  colors_rgba8.bin         # N x 4, brightness or color-index mapped
  sizes_f32.bin            # N, point size derived from G magnitude
  source_id_u64.bin        # N, Gaia source_id
  g_mag_f32.bin            # N, phot_g_mean_mag
  bp_rp_f32.bin            # N, Gaia BP-RP color, NaN when missing
  parallax_f32.bin         # N, mas, NaN when missing
  pm_f32.bin               # N x 2, pmra/pmdec, NaN when missing
  label_offset_u32.bin     # N, byte offset into label_text_utf8.bin
  label_length_u16.bin     # N, byte length
  label_text_utf8.bin      # packed UTF-8 labels
  lod_1k_u32.bin           # index list for label and point presets
  lod_5k_u32.bin
  lod_10k_u32.bin
  lod_50k_u32.bin
  lod_all_u32.bin
```

`metadata.json` should include at least:

```json
{
  "dataset": "gaia_dr3",
  "field": "pleiades_2deg",
  "source_count": 250000,
  "ra_center_deg": 56.75,
  "dec_center_deg": 24.12,
  "radius_deg": 2.0,
  "x_min": -0.92,
  "x_max": 0.92,
  "y_min": -0.92,
  "y_max": 0.92,
  "label_modes": ["source_id", "g_mag", "short"],
  "license_note": "Gaia data from ESA/Gaia/DPAC"
}
```


## Scene setup

The first C example should be named:

```text
examples/astronomy/gaia_many_labels_glfw.c
```

The scene contains one figure with one panzoom panel:

1. a black or very dark background clear color;
2. one `dvz_point()` visual for all visible sources;
3. one text/label visual for source labels;
4. optional crosshair or hover marker for the selected source;
5. a small GUI panel controlling label mode, label budget, and culling.

Initial controls:

- field selector: `pleiades`, `galactic_plane`, `lmc`;
- point LOD: `1k`, `10k`, `50k`, `all`;
- label budget: `0`, `1k`, `5k`, `10k`, `50k`;
- label text: `source_id`, `G magnitude`, `short`;
- label color: fixed white, by magnitude, by BP-RP color;
- overlap policy: off, grid-cell first label, brightest per cell;
- hover readout toggle;
- freeze layout toggle for debugging pan/zoom cost.


## Label modes

The cache should contain enough raw fields to generate these labels:

```text
source_id:  "5853498713190525696"
g_mag:      "G=17.2"
short:      "17.2 / 1.4"
motion:     "pm=+12.4,-5.1"
parallax:   "plx=7.31"
```

The first implementation can prepack only `source_id` labels and generate shorter modes at runtime
from numeric buffers. That keeps the worst-case long-label path available while allowing smaller
labels for readable screenshots.


## Text placement semantics

Each label uses:

- a world/data anchor at the star position;
- a pixel-space glyph size, independent of zoom;
- a small pixel offset from the point, for example `(4, -4)`;
- optional leader/callout only for hovered or selected sources;
- no depth test in the 2D panel case.

This mixed placement mode is the critical text feature:

```text
anchor = world_to_screen(source_position)
glyph_quad = anchor + pixel_offset + shaped_glyph_offsets
```

Labels should remain readable while panning and zooming, without changing their glyph size merely
because the sky field is zoomed.


## Culling and overlap policy

This example should make culling behavior explicit because many-label rendering is never just a draw
throughput problem.

Required policies:

1. **Frustum culling**: skip labels whose anchor is outside the panel viewport.
2. **Magnitude priority**: when reducing labels, prefer brighter stars.
3. **Screen-grid thinning**: keep at most one label per configurable screen tile, such as 24x12 px.
4. **Stable ordering**: avoid labels flickering between frames when pan/zoom changes slightly.
5. **Hover override**: always show the hovered and selected source labels.

The first milestone may perform this on the CPU every frame. The final performance target should
keep the expensive parts incremental or GPU-friendly.


## FramePlan and DRP2 shape

The intended steady-state frame should not recreate glyph atlas textures, point buffers, or label
strings every frame.

Initial frame:

1. upload point position/color/size buffers;
2. upload or initialize glyph atlas texture;
3. upload label string/glyph instance buffers for the first visible label set;
4. create point and text pipelines;
5. draw points;
6. draw glyph instances.

Pan/zoom-only frame:

1. update panel transform uniforms;
2. update visible label instance buffer only if the culling result changes;
3. draw points;
4. draw glyph instances.

Label-mode change:

1. rebuild affected label strings or select a different packed-label range;
2. shape any new glyph runs;
3. append missing glyphs to the atlas;
4. upload changed glyph instances;
5. draw points and labels.

This example should fail review if the frame plan shows per-label draw calls, per-frame full atlas
rebuilds, or full string reshaping on ordinary pan/zoom when labels are unchanged.


## Performance targets

Use the same executable to benchmark label budgets:

```text
./build/examples/astronomy/gaia_many_labels_glfw --field pleiades --labels 1000 --frames 300
./build/examples/astronomy/gaia_many_labels_glfw --field pleiades --labels 5000 --frames 300
./build/examples/astronomy/gaia_many_labels_glfw --field pleiades --labels 10000 --frames 300
./build/examples/astronomy/gaia_many_labels_glfw --field plane --labels 50000 --frames 300
```

Metrics to print:

- frame time and FPS;
- visible point count;
- visible label count;
- shaped glyph count;
- glyph atlas occupancy;
- glyph atlas upload bytes this frame;
- label instance upload bytes this frame;
- culling/layout CPU time;
- draw count and DRP2 command count.

The key stress cases are:

1. static view with many labels;
2. continuous pan with stable culling;
3. wheel zoom through several label-density regimes;
4. switching from short labels to long `source_id` labels;
5. hover/selection updates without rebuilding the whole label set.


## Current implementation gap

As of the current v0.4 scene slice, this example depends on text rendering work that is not yet
complete.

Already useful:

- retained point rendering through the scene -> DRP2 -> app path;
- panzoom panel interaction;
- C examples with binary-cache loading patterns;
- pick/probe request plumbing for point-like workflows;
- existing font assets under `data/fonts/`.

Missing or incomplete:

- a production text/glyph visual backed by a glyph atlas;
- mixed world-anchor/screen-size placement in the renderer;
- bulk glyph-instance upload and update paths;
- label culling/overlap policy hooks;
- glyph atlas diagnostics and reuse tests;
- text-specific FramePlan and DRP2 fixtures;
- hover/selection integration that can force specific labels visible.


## Acceptance criteria

The example is successful when:

1. a real Gaia DR3 field can be prepared into the cache format above;
2. the C demo renders at least `100k` stars as points from the cache;
3. the same demo can render `1k`, `5k`, and `10k` visible labels interactively;
4. a stress preset can attempt `50k` labels and print meaningful diagnostics even if the FPS drops;
5. pan/zoom updates do not rebuild the glyph atlas or reshape unchanged strings;
6. label overlap policy can reduce visible labels deterministically;
7. hover and selected source labels remain visible regardless of thinning;
8. the frame trace shows batched text rendering rather than per-label draw calls.


## Why this pressures the architecture

This example isolates a common scientific visualization workload that is easy to underestimate:
many small, anchored labels over many points.

It pressures:

- glyph atlas lifetime and growth;
- UTF-8 string storage and shaped-run caching;
- mixed coordinate transforms;
- dynamic instance-buffer updates;
- culling and screen-space overlap policy;
- stable interaction under pan/zoom;
- DRP2 command volume and retained-resource reuse;
- diagnostics for distinguishing layout cost, upload cost, and draw cost.

The same infrastructure is needed later for axes, tick labels, colorbars, annotations, molecule
residue labels, segmentation-cell labels, and microscopy hover readouts.
