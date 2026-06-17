# Figures, Panels, and Visuals

The figure-panel-visual hierarchy is the user-facing shape of a Datoviz scene.

## Figures

Figures answer "where does this scene render?" They define the render target size and contain one
or more panels. The same figure may be used by an interactive view, an offscreen capture path, or an
embedding provider, depending on which runtime surface is active.

## Panels

Panels answer "which region and coordinate domain does this content use?" A panel has a viewport,
a data domain, optional camera or panzoom state, and attached scene objects. Most layout decisions,
such as single-view figures, grids, linked panels, overlays, and annotation panels, are expressed by
panel placement and controller/domain sharing.

## Visuals

Visuals answer "what homogeneous batch should be drawn?" A point cloud should usually be one point
visual with many point items. A set of trajectories should usually be one path or segment visual
when they share rendering state. Split visuals when family, material, panel attachment, transform,
lifetime, visibility, or update cadence genuinely differs.

## Sampled Resources

Fields and sampled resources answer "what data can be sampled?" Images, volumes, color-mapped
fields, textures, and lookup tables should be represented as retained resources with explicit roles
rather than as hidden backend state.

## Adornments and Overlays

Adornments answer "what semantic context surrounds the data?" Axes, ticks, labels, colorbars,
legends, scale bars, and readouts are not separate renderer paths. They are scene-level objects that
lower to visuals and share the same coordinate, invalidation, and frame-planning rules.

Overlays should be treated as panel or figure content with clear coordinate ownership. Pixel-space
UI-like overlays should stay explicit so they do not get confused with data-coordinate visuals.

See also:

- [Scene model](scene-model.md)
- [Coordinate systems](coordinate-systems.md)
- [Performance model](performance-model.md)
