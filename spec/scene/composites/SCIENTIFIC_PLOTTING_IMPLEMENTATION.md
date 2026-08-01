# Scientific Plotting Building Blocks

Status: first v0.4 Datoviz-owned rendering slice implemented. Updated: 2026-08-01.

Use [SCIENTIFIC_PLOTTING_BOUNDARY.md](SCIENTIFIC_PLOTTING_BOUNDARY.md) for the durable ownership split. Datoviz provides retained rendering building blocks; GSP/VisPy2 owns statistical transforms, plotting grammar, data adaptation, and domain recipes.

## Implemented Surface

- `DvzGuideLine` and `DvzGuideSpan` provide horizontal or vertical guide annotations and filled spans.
- `DvzBars` renders explicit intervals or precomputed bar geometry without owning histogram statistics.
- `DvzBand` renders lower/upper envelopes with an optional center path.
- Descriptor initializers, retained mutators, generated visual-role accessors, lifetime handling, tests, examples, documentation, and bindings are present.
- Existing `DvzPath` subpaths remain the preferred first approach for stacked or multi-trace data; no retained trace-collection object is required for v0.4.

## Boundary

These objects accept already-computed positions, intervals, bounds, colors, and styles. They do not bin samples, compute confidence intervals, stack series, infer statistical semantics, or introduce a C plotting grammar. New statistical convenience belongs above Datoviz unless a reusable rendering-native primitive is missing.

## Extension Rule

Before adding another composite, prove that existing visuals and the implemented guide, bars, band, polygon, graph, scale, legend, and annotation objects cannot express the rendering-native requirement. Keep lowering on the normal scene-to-FramePlan-to-DRP2 path and add focused lifetime, update, layout, and render proof.
