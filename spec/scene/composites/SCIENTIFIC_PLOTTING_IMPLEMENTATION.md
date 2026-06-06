# Scientific Plotting Implementation Plan

Status: v0.4 first-slice implementation plan. Updated: 2026-06-06.

This file turns the boundary in
[`SCIENTIFIC_PLOTTING_BOUNDARY.md`](SCIENTIFIC_PLOTTING_BOUNDARY.md) into an implementation order
for Datoviz-owned rendering building blocks.

The goal is not to create a C plotting grammar. The goal is to provide stable retained scene
objects that future GSP/VisPy2 plotting functions can target.


## First-Slice Scope

Implement these in order:

1. guide annotations: horizontal/vertical guide lines and spans;
2. bars/intervals: pre-binned or explicit interval rendering;
3. bands/ribbons: semi-opaque filled regions around a path or between two curves;
4. scientific examples that prove the building blocks together.

Defer a retained trace-collection object until examples prove that existing `path` subpaths are
insufficient. The first 32-channel stacked trace example should use `dvz_path()` with explicit
subpaths.


## Naming Direction

Preferred public object names:

| Concept | Preferred Datoviz API | Notes |
| --- | --- | --- |
| guide line | `DvzGuideLine`, `dvz_guide_line()` | generic retained object |
| horizontal guide line | `dvz_hline()` | convenience wrapper over guide line |
| vertical guide line | `dvz_vline()` | convenience wrapper over guide line |
| guide span | `DvzGuideSpan`, `dvz_guide_span()` | filled interval in one dimension |
| horizontal span | `dvz_hspan()` | convenience wrapper over guide span |
| vertical span | `dvz_vspan()` | convenience wrapper over guide span |
| bars/intervals | `DvzBars`, `dvz_bars()` | rendering object, not statistical `hist()` |
| band/ribbon | `DvzBand`, `dvz_band()` | uncertainty region plus optional center path |

`DvzHistogram` should not be the first public object. If added later, it should be a small wrapper
or helper around `DvzBars` for pre-binned data, not the canonical owner of all histogram statistics.


## Header Placement

Use focused headers once implementation begins:

```text
include/datoviz/scene/plot.h
src/scene/composites/guide.c
src/scene/composites/bars.c
src/scene/composites/band.c
```

`include/datoviz/scene.h` should include `scene/plot.h` once the API is active.

If guide lines are implemented inside the annotation subsystem first, keep the public declarations
in `scene/plot.h` or a future `scene/guide.h`; do not hide plotting-oriented guide names in a
large generic annotation header.


## Common API Rules

All descriptors must follow existing scene conventions:

1. first fields are `uint32_t struct_size` and `uint32_t flags`;
2. returned objects are opaque scene-owned handles unless an explicit destroy API exists;
3. setters copy inbound arrays or strings unless documented otherwise;
4. coordinates are semantic F64 data coordinates at the object boundary;
5. generated visuals attach with `coord_space = DATA` unless the object explicitly uses panel or
   viewport space;
6. generated visuals use ordinary role names for advanced inspection.

Advanced role access should eventually use the shared composite role accessor. Normal examples
should not require role-visual mutation.


## Guide Lines And Spans

Guides are panel-attached annotation-like objects. They track one panel's visible data domain and
emit generated visuals with endpoints or fill geometry derived from that domain.

Recommended descriptors:

```c
typedef enum
{
    DVZ_GUIDE_ORIENTATION_HORIZONTAL = 0,
    DVZ_GUIDE_ORIENTATION_VERTICAL   = 1,
} DvzGuideOrientation;

typedef struct DvzGuideLineDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzGuideOrientation orientation;
    double value;
    float stroke_width_px;
    DvzSegmentCap cap_start;
    DvzSegmentCap cap_end;
    uint8_t color[4];
    int32_t z_layer;
    const char* label;
} DvzGuideLineDesc;

typedef struct DvzGuideSpanDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzGuideOrientation orientation;
    double min_value;
    double max_value;
    uint8_t fill_color[4];
    uint8_t outline_color[4];
    float outline_width_px;
    int32_t z_layer;
    const char* label;
} DvzGuideSpanDesc;
```

Recommended constructors:

```c
DvzGuideLineDesc dvz_guide_line_desc(void);
DvzGuideSpanDesc dvz_guide_span_desc(void);

DvzGuideLine* dvz_guide_line(DvzPanel* panel, const DvzGuideLineDesc* desc);
DvzGuideLine* dvz_hline(DvzPanel* panel, double y, const DvzGuideLineDesc* desc);
DvzGuideLine* dvz_vline(DvzPanel* panel, double x, const DvzGuideLineDesc* desc);

DvzGuideSpan* dvz_guide_span(DvzPanel* panel, const DvzGuideSpanDesc* desc);
DvzGuideSpan* dvz_hspan(DvzPanel* panel, double y0, double y1, const DvzGuideSpanDesc* desc);
DvzGuideSpan* dvz_vspan(DvzPanel* panel, double x0, double x1, const DvzGuideSpanDesc* desc);
```

First-slice lowering:

| Object | Role | Visual |
| --- | --- | --- |
| `DvzGuideLine` | `"line"` | `segment` |
| `DvzGuideSpan` | `"fill"` | `primitive` or `mesh` generated quad |
| `DvzGuideSpan` | `"outline"` | optional `segment`/`path` |
| either | `"label"` | optional label annotation or text visual |

First-slice behavior:

1. `hline`: endpoint x coordinates come from the visible panel x domain; y is fixed.
2. `vline`: endpoint y coordinates come from the visible panel y domain; x is fixed.
3. `hspan`: x range comes from visible panel x domain; y range is fixed.
4. `vspan`: y range comes from visible panel y domain; x range is fixed.
5. If visible domain is not yet resolved, use the panel's explicit domain or object bounds on first
   frame; update after domain resolution.
6. Disable depth test by default; alpha mode is blended when color alpha is not opaque.

Tests:

1. descriptor defaults and validation;
2. endpoint/fill geometry for known panel domains;
3. domain-change invalidation;
4. alpha/depth defaults;
5. invalid min/max, NaN, and NULL descriptor handling.


## Bars And Intervals

`DvzBars` is a retained rendering object for explicit intervals. It is the Datoviz target for
pre-binned histograms and generic interval charts.

Recommended descriptor:

```c
typedef enum
{
    DVZ_BARS_ORIENTATION_VERTICAL   = 0,
    DVZ_BARS_ORIENTATION_HORIZONTAL = 1,
} DvzBarsOrientation;

typedef struct DvzBarsDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzBarsOrientation orientation;
    double baseline;
    float gap_fraction;
    uint8_t fill_color[4];
    uint8_t outline_color[4];
    float outline_width_px;
    int32_t z_layer;
} DvzBarsDesc;
```

Recommended data setter:

```c
int dvz_bars_set_intervals(
    DvzBars* bars, uint32_t count, const double* starts, const double* ends,
    const double* values);
```

For vertical bars, `starts`/`ends` are x interval edges and `values` are y values from `baseline`.
For horizontal bars, `starts`/`ends` are y interval edges and `values` are x values from `baseline`.

First-slice lowering:

| Role | Visual |
| --- | --- |
| `"fill"` | generated quad mesh/primitive triangles |
| `"outline"` | optional path or segment strokes |

Histogram helpers may be added later as explicit utilities:

```c
int dvz_histogram_bins_uniform(
    uint32_t sample_count, const double* samples, uint32_t bin_count,
    double min_value, double max_value, double* out_edges, double* out_counts);
```

This helper is optional and should not block `DvzBars`.

Tests:

1. vertical and horizontal interval geometry;
2. nonzero baseline;
3. alpha/depth defaults;
4. bounds calculation;
5. zero-width, negative-width, NaN, and empty input validation.


## Bands And Ribbons

`DvzBand` is a retained composite for filled uncertainty regions and optional center paths.

Recommended descriptor:

```c
typedef struct DvzBandDesc
{
    uint32_t struct_size;
    uint32_t flags;
    uint8_t fill_color[4];
    uint8_t line_color[4];
    float line_width_px;
    bool show_line;
    bool show_bounds;
    uint8_t bound_color[4];
    float bound_width_px;
    int32_t z_layer;
} DvzBandDesc;
```

Recommended setters:

```c
int dvz_band_set_bounds(
    DvzBand* band, uint32_t count, const double* x, const double* lower,
    const double* upper);

int dvz_band_set_center(
    DvzBand* band, uint32_t count, const double* x, const double* y);
```

`dvz_band_set_center()` is optional. If absent and `show_line` is true, the center may be derived as
`0.5 * (lower + upper)` in the first slice.

First-slice lowering:

| Role | Visual |
| --- | --- |
| `"fill"` | generated triangle strip or mesh |
| `"line"` | optional `path` |
| `"bounds"` | optional `path` with two subpaths |

NaN handling should split the band into multiple valid spans rather than producing triangles across
missing data. This can share path subpath logic.

Tests:

1. asymmetric lower/upper bounds;
2. derived center line;
3. explicit center line;
4. NaN gap splitting;
5. alpha/depth defaults and bounds calculation.


## Stacked Trace Example

Do not add a retained trace collection in the first slice. Use:

1. one `dvz_path()` visual;
2. `dvz_path_set_subpaths()` with 32 subpaths;
3. data coordinates for time and vertically offset channel values;
4. one bottom x axis and either hidden y ticks or sparse channel labels.

If several examples or GSP lowering produce duplicated trace packing code, promote a small
`DvzTraceCollection` object later.


## Scientific Examples

Add examples after the corresponding primitives exist:

| Example | Depends on | Purpose |
| --- | --- | --- |
| `examples/c/scientific/spike_autocorrelogram.c` | guides, spans, bars | histogram plus baseline/refractory annotations |
| `examples/c/scientific/stacked_traces.c` | path subpaths | panel-6 style high-density time-series proof |
| `examples/c/scientific/error_band_path.c` | band, path | thick path plus semi-opaque margin of error |

The spike autocorrelogram example should use deterministic synthetic data or prepared arrays. The
example may compute autocorrelogram bins locally for C proof, but that computation is not a core
Datoviz plotting API.


## Implementation Order

1. Add `scene/plot.h` descriptor sketches and opaque handle declarations behind implementation
   files.
2. Implement `DvzGuideLine` and `DvzGuideSpan` as panel-attached retained objects.
3. Add guide geometry tests and one small guide feature example.
4. Implement `DvzBars` from explicit intervals.
5. Add the spike autocorrelogram example using `DvzBars`, `dvz_hline()`, `dvz_vline()`, and
   `dvz_vspan()`.
6. Implement `DvzBand`.
7. Add the error-band path example.
8. Add the stacked-trace example using existing path subpaths.
9. Reassess whether trace collection, histogram wrapper, or rectangle visual is justified.


## Non-Goals For The First Slice

1. full `hist()` statistical policy;
2. pandas/xarray/NumPy adaptation;
3. stacked/grouped histogram grammar;
4. automatic annotation collision solving;
5. Matplotlib-compatible Python naming beyond Datoviz convenience wrappers;
6. a new renderer, presentation path, or backend-specific shortcut.

