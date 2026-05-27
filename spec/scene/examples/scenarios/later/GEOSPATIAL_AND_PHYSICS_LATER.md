# Later Geospatial And Physics Scenarios

> **Example status:** later pressure tests
> **Target:** future showcases and domain-specific data policies
> **Data:** public prepared caches only when essential
> **Validation:** smoke plus domain-specific visual/interaction checks


## `geo_trajectories_regions_events`

Merged track for animal migration, choropleth globe, earthquake aftershock explorer, and flight
trajectories.

Needed first: geographic transforms, projection/topology helpers, timeline controls, trajectory
identity, region picking, large-data/LOD policies, and cache provenance.


## `tokamak_hep_field_lines`

Merged track for tokamak plasma field lines and high-energy physics event displays.

Needed first: field-line path/tube rendering, vector fields, complex event geometry, labels,
picking/selection, transparency policy, and domain-specific prepared data.


## `many_labels`

Astronomy or dense-map label stress beyond v0.4 basic text. Needs label LOD, culling/collision
policy, placement diagnostics, and performance targets for large label counts.
