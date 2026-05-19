> **Execution Status**
> - **Status:** `FUTURE ROADMAP`
> - **Updated on:** `2026-05-18`
> - **Purpose:** preserve exploratory direction for future track, ensemble, uncertainty, and
>   molecular or structural-biology semantic resources.
> - **Scope:** tracks/trajectories, ensembles/uncertainty, and molecular/structural-biology data.

# Domain Resource Roadmap


## Summary

Some scientific use cases are not single visual families. They are domain resources with stable
semantic identity, multiple render views, and domain-specific interaction payloads.

This note covers three such directions:

- time-aware trajectories and tracks;
- ensemble and uncertainty data;
- molecular and structural-biology scenes.

The recommended first step is example-driven composition over existing visuals. Promote public C
resources only after repeated examples prove the common model.

Generic resources belong closer to Datoviz core than domain-specific resources. Tracks, timelines,
selection identities, and ensemble summary views can serve many scientific communities. Molecular
structures are useful examples and integration tests, but their parsing, hierarchy conventions, and
domain-specific style presets should remain Python/GSP/example-level unless several independent
Datoviz use cases prove a compact generic core model.


## Maturity Guidance

Recommended maturity order:

| Direction | Core suitability | Initial home |
|---|---|---|
| tracks/trajectories | high | example composition, then possible public `TrackTable` |
| timeline/time cursor | high | align with scene animation clock, then possible coordination helper |
| ensemble summaries | medium | Python/GSP summaries plus generic visuals |
| molecular structures | low/medium | examples and Python-prepared resources first |

The public C API should grow around generic semantics:

- time-indexed samples;
- stable track ids;
- linked semantic selection;
- reusable uncertainty summaries;
- scene-clock coordination.

It should not grow around domain-specific file formats or specialized scientific databases.


## Tracks And Trajectories

### Simple Examples

- animal movement tracks;
- flight paths;
- cell tracking in microscopy;
- particle trajectories;
- storm tracks;
- GPS or vehicle telemetry;
- diffusion tractography;
- HEP particle tracks.


### Data Model

A track table adds time and identity semantics to path-like data.

```text
DvzTrackTable
  sample_count
  track_count
  position[sample_count]       vec2 or vec3
  time[sample_count]           float or int64
  track_id per sample or offsets[track_count + 1]
  sample attributes            speed, category, scalar, event flags
  track attributes             color, group, label, visibility
```

Differences from ordinary `path`:

- time is semantically meaningful;
- gaps and irregular sampling matter;
- current-position markers are common;
- fading tails depend on time;
- picking should resolve track id, sample id, and time;
- linked metrics panels often share the same time cursor.


### Render Views

- full path per track;
- fading tail behind a replay cursor;
- current-position markers;
- event markers along a track;
- selected-track highlight;
- dense trajectory bundle view;
- future [`tube`](../../visuals/TUBE.md) or ribbon view for selected high-quality tracks.


### First-Class Or Composition?

Start as a composition over `path`, `point`/`marker`, and overlay visuals. Promote to a `track`
visual/resource only if repeated examples need shared time-aware mutation, picking, and fading
behavior.

Tracks are the strongest public-C candidate in this document because they are generic across many
domains. A future public object should probably be named `DvzTrackTable`, `DvzTrajectorySet`, or
similar, and should focus on:

- `track_id`;
- sample time or frame index;
- position;
- per-sample attributes;
- per-track attributes;
- gaps and irregular sampling;
- picking payloads that resolve track id, sample id, and time.


## Ensembles And Uncertainty

### Simple Examples

- weather forecast ensemble trajectories;
- confidence intervals around time series;
- probabilistic segmentation;
- simulation ensemble scalar fields;
- uncertainty ellipses around point estimates;
- posterior samples in parameter space;
- error bands and quantile fans.


### Data Model

Ensemble data needs a member axis and summary products.

```text
DvzEnsemble
  member_count
  domain shape
  member ids and weights
  values[member, ...]
  optional precomputed summaries:
    mean
    variance
    quantiles
    min/max envelope
    probability above threshold
```

Uncertainty can appear as:

- scalar intervals;
- vector/tensor covariance;
- probability fields;
- categorical probabilities;
- multiple discrete realizations;
- summary statistics.


### Render Views

- line bands/envelopes;
- ensemble spaghetti lines;
- boxplot/errorbar summaries;
- probability heatmaps;
- variance or entropy overlays;
- covariance ellipses/ellipsoids;
- probabilistic volumes;
- member selection and comparison.

Existing `errorbar`, `boxplot`, and `splat` covariance semantics are local building blocks, not a
complete ensemble model.

Ensembles should stay Python/GSP-level first. The hard work is usually data modeling and statistics:
member axes, reductions, probabilities, quantiles, lazy loading, and domain-specific uncertainty
meaning. Python libraries such as NumPy, xarray, dask, and domain packages are the right place to
compute summaries. Datoviz should initially render prepared summaries.

Recommended display summaries worth standardizing:

| Data kind | Useful summaries |
|---|---|
| scalar/line | min, max, mean, standard deviation, median, lower/upper quantile |
| interval | lower/upper confidence or credible interval |
| categorical | probability, entropy, most likely label, margin between top labels |
| vector/tensor | covariance, covariance ellipse/ellipsoid, magnitude uncertainty |

Avoid standardizing arbitrary statistical models. Standardize display semantics such as central
estimate plus interval, probability field, covariance glyph, and selected ensemble member.


### Compute Opportunities

- mean/variance reductions;
- quantile or percentile approximation;
- min/max envelope generation;
- probability threshold maps;
- member filtering and downsampling.


## Molecular And Structural Biology

### Simple Examples

- protein atoms and bonds;
- ball-and-stick molecule;
- ribbon/cartoon protein;
- molecular dynamics trajectory;
- ligand binding pocket;
- cryo-EM density plus fitted atomic model;
- residue/chain selection and labels.


### Data Model

A molecular structure resource would preserve biological hierarchy and stable ids.

```text
DvzMolecularStructure
  atoms:
    position
    element
    radius
    atom id/name
    residue id
    chain id
  bonds:
    atom a
    atom b
    order/type
  residues:
    id, name, chain, secondary structure
  chains:
    id, name
  frames:
    optional positions per time step for MD
```

Python should own PDB/mmCIF/trajectory parsing first. Datoviz C may eventually own the compact
runtime structure and render views.

This is deliberately lower priority for Datoviz core than tracks or graphs. Molecular structures are
domain-specific: atom naming, residues, chains, alternate locations, biological assemblies, topology
inference, surface generation, and trajectory formats all have specialized conventions. Datoviz
should support molecular examples by rendering prepared arrays through `sphere`, `segment`, `path`,
`mesh`, `volume`, and `glyph`; it should not rush toward a public molecule API.

Promote a C `DvzMolecularStructure` only if repeated examples need shared runtime identity and
selection across atoms, bonds, residues, chains, ribbons, surfaces, labels, and density maps.


### Render Views

- atoms as `sphere`;
- bonds as `segment`, cylinder-like impostors, or instanced meshes;
- backbone trace as [`path`](../../visuals/PATH.md);
- ribbon/cartoon as future [`tube`](../../visuals/TUBE.md) or precomputed `mesh` geometry;
- molecular surface as `mesh`;
- cryo-EM density as `volume`;
- labels as `glyph`;
- selected atom/residue/chain highlight overlays.


### Picking And Selection

Useful semantic payloads:

```text
atom pick:
  atom id, element, residue id, chain id

bond pick:
  bond id, atom ids, residue ids

residue pick:
  residue id, residue name, chain id

surface pick:
  face id plus mapped residue/atom neighborhood when available
```

Selection should operate at atom, residue, chain, ligand, and model levels.

Use named link channels instead of one universal molecule id:

```text
atom channel
bond channel
residue channel
chain channel
model channel
```

Derived visuals can participate in different channels:

```text
atom sphere item      -> atom, residue, chain
bond segment item     -> bond, endpoint atoms, residue(s), chain(s)
ribbon span           -> residue range, chain
surface face          -> residue or atom-neighborhood key when available
label item            -> atom, residue, chain, or model
density region        -> residue/chain/ligand only when a mapping exists
```

The active selection policy can choose the channel appropriate to the interaction.


## Timeline And Animation

The existing scene animation system already owns the scene clock, realtime/offline modes, timer
callbacks, transitions, and camera paths. A future timeline should not replace that system.

Recommended distinction:

| Concept | Role |
|---|---|
| scene clock | authoritative time source for animations and offline export |
| animation | changes scene properties as a function of scene-clock time |
| timeline | coordinates domain time, replay cursors, visible windows, and multiple data views |

A timeline is a coordination object above the scene clock. It can map scene time to domain-specific
time axes:

```text
scene clock t
  -> dashboard sample time
  -> trajectory replay time
  -> particle simulation frame
  -> molecular dynamics frame
  -> ensemble member/time step
```

Useful timeline state:

```text
current_time
duration
playback_rate
paused
looping
visible_window
frame_index or sample_index
time_units
```

Useful consumers:

- DAQ and dashboard traces;
- tracks and fading tails;
- particle history;
- molecular dynamics frames;
- simulation or ensemble time steps;
- linked cursors across panels.

The first implementation should remain example-local and use the existing scene clock plus timer
animations. Promote `DvzTimeline` only after several examples converge on the same coordination
needs. The animation spec currently marks scene-level timeline as deferred; this roadmap describes
what that deferred object could become.


## Plugin And Extension Model

Datoviz is a library, not an application like napari. A napari-style plugin marketplace does not map
directly to the C core because Datoviz does not own a persistent application shell, user environment,
data model registry, or GUI workflow.

Plugin-like extension still has meaning at the edges:

- Python/GSP packages can provide loaders, preprocessors, and domain adapters;
- examples can demonstrate domain integrations without making them core;
- advanced users can register custom shaders, custom visual pipelines, or compute kernels when that
  infrastructure exists;
- app-layer integrations can add optional GUI panels or hosted UI widgets;
- domain packages can emit prepared Datoviz resources rather than extending C directly.

Preferred terminology for Datoviz core is therefore:

```text
extension point
adapter
custom visual
custom shader/compute pipeline
example package
Python/GSP helper
```

Reserve "plugin" for higher-level applications built on Datoviz or GSP, not for the core C library
unless a future hosted application shell actually needs plugin discovery and lifecycle management.


## Cross-Cutting Requirements

- stable semantic ids;
- link keys between derived visuals from the same domain object;
- render views that can be toggled without destroying source resources;
- per-view style mapping from domain attributes;
- picking results that resolve to domain identities;
- optional time/replay for tracks and molecular dynamics;
- optional timeline coordination across several panels/views;
- partial updates for changing positions, styles, visibility, and selection.


## Example Plans

Useful future examples:

- generic trajectory layer fixture with time cursor and fading tails;
- ensemble uncertainty viewer with mean/variance/quantile views;
- molecular dynamics trajectory viewer;
- cryo-EM density plus protein fit viewer.
- timeline coordination example linking tracks, traces, and a replay cursor.


## Open Questions

- What is the smallest public trajectory object that is useful without becoming a plotting library?
- Should a future `DvzTimeline` be a scene object, an app-layer object, or a GSP-level helper?
- Which examples demonstrate enough shared timeline behavior to justify promoting `DvzTimeline`?
- Which ensemble display summaries should become typed visual helpers rather than plain arrays?
- Which molecular features, if any, are generic enough for core Datoviz rather than example/Python
  packages?
