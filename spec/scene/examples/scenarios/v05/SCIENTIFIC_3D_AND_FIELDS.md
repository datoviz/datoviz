# v0.5 Scientific 3D And Field Scenarios

> **Example status:** v0.5 planning bundle
> **Target:** C showcases where they prove renderer/runtime behavior
> **Data:** prepared public caches or deterministic synthetic fallbacks
> **Validation:** smoke, screenshot, interaction, and feature-specific checks


## `mouse_brain_atlas_explorer`

Full brain explorer after the v0.4 narrow brain showcase. Needs region picking, selection-driven
mesh opacity/highlight, linked 2D/3D panels, atlas labels/readouts, and UI tree/filter integration.


## `tracks_tractography_vectors`

Merged track for diffusion tractography and napari-style tracks/vectors/shapes. Needs packed ragged
paths, per-track identity, direction coloring, vector visuals, basic picking/selection, and
high-quality thin-line rendering. Tubes/ribbons and out-of-core million-streamline collections stay
later.


## `galaxy_labels`

Astronomy large-point/label example. Needs point/marker sprite polish, large dataset cache, label
overlay or label-on-demand behavior, rotation/camera animation, and screenshot capture.


## `wind_projections`

Projection-aware version of the v0.4 wind-field showcase. Needs projection transforms, vector
Jacobian semantics, graticule/coastline helpers, optional particle overlay, and hover labels.


## `textured_surface_full_workflow`

Full terrain/planet/Mars workflow after the v0.4 retained textured-mesh proof. Needs asset/cache
policy, camera-path polish, optional overlays, GIS preprocessing, and probe/readout conventions.


## `finite_element_stress`

Applied engineering mesh/field target. Needs mesh scalar fields, colorbar, region/element
selection, optional isolines, and prepared FEA cache or deterministic generated mesh.


## `crystal_phonon`

Materials-science showcase. Needs sphere/mesh rendering, animation of atom displacements or
phonon modes, labels/selection, and stable material controls.
