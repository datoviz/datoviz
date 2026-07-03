# Scientific Plotting Boundary

Status: v0.4 design boundary. Updated: 2026-06-06.

This note defines the split between Datoviz scene/composite support and the future GSP/VisPy2
plotting layer for common scientific plotting features.

First-slice implementation details are tracked in
[`SCIENTIFIC_PLOTTING_IMPLEMENTATION.md`](SCIENTIFIC_PLOTTING_IMPLEMENTATION.md).

Datoviz should provide rendering-native building blocks that are useful from C, raw bindings, GSP,
and VisPy2. GSP/VisPy2 should own the high-level plotting grammar, statistical policy, and Python
data adaptation.


## Boundary Decision

Datoviz owns semantic scene objects when the main problem is retained rendering, panel-domain
attachment, coordinate transforms, picking/query identity, alpha/depth behavior, or lowering to
built-in visuals.

GSP/VisPy2 owns plotting functions when the main problem is statistics, data-frame adaptation,
domain-specific analysis, subplot grammar, user-facing defaults, or Python ergonomics.

This keeps Datoviz useful as a backend without turning the v0.4 C engine into a high-level plotting
library.


## Datoviz-Owned Building Blocks

The following features belong in Datoviz core as scene objects, annotations, composites, or geometry
helpers:

1. guide lines and guide spans, including `hline`, `vline`, `hspan`, and `vspan` convenience
   entry points;
2. bar, rectangle, or interval primitives/composites that can render pre-binned data;
3. band/ribbon composites for uncertainty regions, margins of error, confidence intervals, or
   filled intervals around a path;
4. efficient path and trace collections, including multi-subpath stacked time series;
5. reusable color, alpha, cap, join, axis, label, and panel-domain behavior needed by those objects;
6. C examples that prove the building blocks in realistic scientific scenes.

The intended lowering path is ordinary scene composition:

```text
semantic object -> composite or annotation roles -> built-in visuals -> frame plan -> DRP2
```

Examples:

| Datoviz object | Lowering |
| --- | --- |
| guide line | `segment` role, endpoints derived from visible panel domain |
| guide span | filled mesh/primitive role plus optional outline |
| bars/intervals | generated rect geometry, optional outline path/segment |
| band/ribbon | filled strip mesh/primitive plus optional center `path` |
| stacked traces | one `path` visual with subpaths, or a trace composite that owns such a path |


## GSP/VisPy2-Owned Plotting Layer

The following features should remain outside Datoviz core and be implemented in GSP/VisPy2:

1. high-level functions such as `plot()`, `scatter()`, `hist()`, `fill_between()`, `axhline()`,
   `axvline()`, and subplot-level aliases;
2. histogram bin policy such as `auto`, Freedman-Diaconis, Scott, Sturges, weighted histograms,
   density normalization, cumulative histograms, and stacked/grouped histogram grammar;
3. domain-specific transforms such as spike-train autocorrelogram computation, baseline estimation,
   refractory-period measurement, raster-plus-correlogram layouts, and neuroscience recipes;
4. NumPy, pandas, xarray, CuPy, Dask, or table-schema adaptation;
5. Python-side style presets, semantic plot defaults, user-facing warnings, and backend selection;
6. Matplotlib-oriented vector export decisions from the same GSP scene description.

GSP/VisPy2 may expose familiar names and defaults, but those functions should lower to Datoviz
building blocks when the Datoviz backend is selected.


## Histogram Policy

Datoviz should not make statistical histogram policy its primary API responsibility.

Preferred split:

```text
Datoviz:
  render bars, interval series, and step paths from explicit edges/counts/densities
  optionally provide small deterministic C binning helpers for examples and simple users

GSP/VisPy2:
  implement hist() statistics, bin selection, weights, density/cumulative policy,
  grouped/stacked semantics, and Python data adaptation
```

If Datoviz adds a `DvzHistogram` object, it should primarily be a retained pre-binned series with a
small optional CPU helper. It should not become the canonical owner of every statistical histogram
mode that Python libraries already cover well.


## Guide Policy

Guide lines and spans are Datoviz scene semantics, not Python-only plotting sugar.

They must:

1. attach to a panel or axis domain;
2. recompute geometry when the visible domain changes;
3. share annotation styling, label placement, visibility, and future picking behavior;
4. work from C examples and from future GSP/VisPy2 wrappers;
5. lower through built-in visuals rather than custom backend commands.

Matplotlib-style line and span convenience names stay out of the canonical Datoviz C API. C callers
create guide annotations through explicit descriptors and future plotting wrappers may add shorthand
above Datoviz.


## Band And Error-Margin Policy

Filled uncertainty regions should be modeled as a semantic band/ribbon object, not as a special
mode of `path`.

The center line can be a normal `path`, with screen-space stroke width, cap, join, alpha, and
subpath support. The filled margin should lower to generated fill geometry with explicit alpha and
depth-test defaults.

GSP/VisPy2 should own high-level names such as `fill_between()` and statistical helpers such as
standard error, confidence interval, bootstrap interval, or percentile bands.


## Example Commitments

Datoviz should prove the backend building blocks with C examples that avoid depending on the future
Python plotting layer:

1. spike-train autocorrelogram: precomputed histogram, baseline guide line, zero-lag guide line,
   bilateral refractory span, and label/callout annotations;
2. high-density stacked traces: 32 vertically offset time series, one bottom time axis, compact
   channel labels, and palette-compatible trace colors;
3. thick path with uncertainty band: semi-opaque filled band, thick center path, and axis/grid style
   consistent with the v0.4 example palette.

These examples should demonstrate Datoviz rendering semantics. The same scenes can later become
GSP/VisPy2 examples where the statistical/data-preparation layer is Python-owned.


## API Shape Principles

1. Prefer reusable objects such as guide, span, bars/intervals, band/ribbon, and trace collection
   over plot-type-specific renderers.
2. Keep raw sample statistics optional in Datoviz and explicit in GSP/VisPy2.
3. Expose role visuals only as an advanced escape hatch; normal users should use typed object APIs.
4. Use F64 semantic coordinates at the scene boundary and lower to GPU-facing formats through the
   normal scene resource path.
5. Keep examples deterministic and small enough for release validation, while still looking like
   real scientific workflows.
