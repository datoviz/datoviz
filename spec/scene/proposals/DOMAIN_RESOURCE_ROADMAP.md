# Domain Resource Roadmap

> **Status:** exploratory roadmap for future semantic resources.
> **Scope:** tracks/trajectories, ensembles/uncertainty, and molecular/structural-biology data.


## Summary

Some scientific use cases are not single visual families. They are domain resources with stable
semantic identity, multiple render views, and domain-specific interaction payloads.

This note covers three such directions:

- time-aware trajectories and tracks;
- ensemble and uncertainty data;
- molecular and structural-biology scenes.

The recommended first step is example-driven composition over existing visuals. Promote public C
resources only after repeated examples prove the common model.


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
- tube/ribbon view for selected high-quality tracks.


### First-Class Or Composition?

Start as a composition over `path`, `point`/`marker`, and overlay visuals. Promote to a `track`
visual/resource only if repeated examples need shared time-aware mutation, picking, and fading
behavior.


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


### Render Views

- atoms as `sphere`;
- bonds as `segment`, cylinder-like impostors, or instanced meshes;
- backbone trace as `path`;
- ribbon/cartoon as path/mesh-derived geometry;
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


## Cross-Cutting Requirements

- stable semantic ids;
- link keys between derived visuals from the same domain object;
- render views that can be toggled without destroying source resources;
- per-view style mapping from domain attributes;
- picking results that resolve to domain identities;
- optional time/replay for tracks and molecular dynamics;
- partial updates for changing positions, styles, visibility, and selection.


## Example Plans

Useful future examples:

- generic trajectory layer fixture with time cursor and fading tails;
- ensemble uncertainty viewer with mean/variance/quantile views;
- molecular dynamics trajectory viewer;
- cryo-EM density plus protein fit viewer.


## Open Questions

- Which domain resources deserve public C handles?
- Should molecular structures and ensembles remain Python/GSP-level helpers for longer?
- How should link keys connect atoms, bonds, residues, labels, surfaces, and density regions?
- How should time axes be shared between dashboards, tracks, particles, and molecular dynamics?
- Which uncertainty summaries are common enough to standardize?
