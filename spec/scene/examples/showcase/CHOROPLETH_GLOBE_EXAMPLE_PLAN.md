# Choropleth Globe Example Plan

Date: 2026-05-16

## Goal

Build a Datoviz example that demonstrates filled geographic polygons with anti-aliased contours in a
more visually compelling way than a simple flat 2D map.

The recommended first target is an interactive 3D choropleth globe: region polygons are filled by a
scalar value on the Earth surface, and borders are rendered as crisp anti-aliased contours slightly
above the surface. This shows polygon fills, contour quality, depth, camera interaction, and the
scene -> DRP2 -> vklite path in one focused demo.

## Recommended Demo

### Interactive Choropleth Globe

Render countries, states, or another regional polygon dataset on a sphere.

Core behavior:

- Triangulate each region and project the vertices onto a sphere.
- Fill each region with a colormapped scalar value.
- Draw region borders as anti-aliased paths or line strips with a small radial offset to avoid
  z-fighting.
- Use arcball interaction for inspection.
- Add hover picking later to highlight the active region and expose its value.
- Add an optional time slider once the static path is stable.

This should be the first implementation target because it gives a strong visual payoff without
requiring extrusion, side-wall generation, or complex per-region mesh topology.

## Variants

### Extruded Choropleth Globe

Raise each region by a value-dependent amount and render it as a shallow plate on the globe.

This is visually impressive, but it should come after the basic globe because it adds:

- side-wall generation for polygon boundaries,
- more difficult depth and contour ordering,
- careful handling of adjacent regions with different heights,
- more geometry per region.

### Tilted 3D Map Plane

Render a regional map as a tilted 3D object with perspective, lighting, raised polygons, and
anti-aliased borders.

This is easier than a globe and could work well for a focused dataset such as US states, French
departments, Europe, or a single country. It still demonstrates 3D scene behavior more effectively
than a purely screen-aligned 2D map.

### Choropleth With Continuous Contours

Combine administrative polygon fills with smooth isolines over the same domain.

This could show climate anomaly, population density, elevation, or another continuous field. Regions
would show aggregated values while the contour overlay exposes continuous spatial structure.

### Small Multiples

Render several synchronized choropleths in a multi-panel figure.

This is less visually dramatic than a globe, but it would stress multi-panel layout, shared colormap
logic, and repeated scene resources.

## Data Candidates

Good first datasets:

- Natural Earth countries or admin-1 regions for compact, permissive geographic polygons.
- US states or French departments for a smaller regional example.
- Synthetic per-region values if the first goal is graphics validation rather than data storytelling.

Good value fields:

- population density,
- GDP per capita,
- temperature anomaly,
- election-style categorical or diverging values,
- time-varying synthetic scalar for animation tests.

The checked-in example should not download data at runtime. Prefer a small preprocessed fixture and a
separate conversion script that records source URLs, attribution, simplification tolerance, and
triangulation parameters.

## Rendering Plan

Phase 1: static globe

- Load a compact region fixture with triangulated fills and border paths.
- Convert longitude/latitude vertices to unit-sphere positions.
- Render the Earth base as a simple shaded sphere.
- Render filled region mesh patches with a scalar colormap.
- Render borders as anti-aliased contours with a small radial offset.
- Use arcball camera controls.

Phase 2: interaction and polish

- Add hover picking and active-region highlighting.
- Add a colorbar and simple value legend.
- Add region labels only if text rendering is ready enough; otherwise keep labels out of the first
  slice.
- Add a time slider or animation clock for time-varying values.

Phase 3: advanced geometry

- Add optional extrusion.
- Add side walls and top contours.
- Add per-region height scaling and depth-aware outline rendering.

## Implementation Notes

The fill and contour paths should remain separate:

- Fill geometry should be triangulated mesh data.
- Borders should use the path or line visual path so anti-aliasing can be tuned independently.
- Border vertices should be offset radially by a tiny amount on the globe to avoid z-fighting.
- The example should avoid creating a dedicated map renderer contract; use the existing retained scene
  and DRP2 path.

Keep the first version deterministic and small enough for manual smoke testing. The goal is to
exercise the graphics path, not to become a full GIS pipeline.

## Open Questions

- Which polygon fixture should be bundled first: world countries, US states, or a smaller regional
  dataset?
- Should the first values be real data or synthetic values designed to test colormap ranges?
- Is the current path visual sufficient for globe-border anti-aliasing, or does it need a dedicated
  geographic contour example path?
- Should hover selection target polygon fills first, border paths first, or both?
