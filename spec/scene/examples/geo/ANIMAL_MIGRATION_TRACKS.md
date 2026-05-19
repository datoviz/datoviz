# Animal Migration Tracks

> **Agent Pickup**
> - **Category:** `geo`
> - **Implementation target:** Geographic or globe/terrain example with a minimal deterministic mode and optional real assets.
> - **Data policy:** Prefer public datasets with cache metadata; include a synthetic fallback for offline development.
> - **Preprocessing:** Required for real datasets; specify download, projection, tiling, simplification, and cache outputs.
> - **Validation:** Smoke command, camera/interaction checklist, and visual checks for projection or coordinate correctness.


## Summary

Build a movement-ecology trajectory demo with individual animal tracks, time replay, selected
animal highlighting, and optional environmental context. The preferred data is a curated public
Movebank or Movebank Data Repository dataset converted offline into a compact cache with license,
citation, species, and source metadata; use deterministic synthetic or prepared tracks for the
first slice if the dataset is not finalized. Start with a regional map, colored paths per
individual, current-position markers, and simple playback before linked metric panels. Validate
with a smoke command plus manual checks for coordinate alignment, replay continuity, selection
highlighting, trajectory readability, and cache metadata completeness.


## Example Name

`ANIMAL_MIGRATION_TRACKS`


## Purpose

Specify a Datoviz v0.4 showcase example for movement ecology visualization. The example renders
public animal tracking data as interactive individual trajectories with time replay, moving
positions, selected-animal highlighting, movement metrics, and optional environmental context.

This should not be a generic map demo. The scientific identity is:

```text
animal tracking records -> individual trajectories -> time replay -> movement metrics -> environmental context
```


## Why This Example Exists

This example fills a gap in the showcase set: ecology, animal movement, and individual-based
trajectory data.

It is distinct from `GLOBAL_WIND_PROJECTIONS` because it shows Lagrangian animal tracks rather than
gridded atmospheric fields. It is distinct from `EARTHQUAKE_AFTERSHOCK_EXPLORER` because the
objects are persistent individuals observed repeatedly over time, not independent event points.

It should pressure:

1. long path rendering for many individuals,
2. time-dependent filtering and moving current-position markers,
3. per-individual selection and highlighting,
4. linked time-series panels,
5. color modes by identity, speed, altitude/depth, elapsed time, or movement state,
6. map/projection transforms and optional environmental rasters,
7. prepared real-data caches from public tracking repositories.


## Recommended Data Source

Use a curated public Movebank or Movebank Data Repository dataset as the preferred Stage 1 data
source.

Movebank is appropriate because it is a standard platform for animal tracking data and provides
public datasets, repository records, and API/data-access workflows. Runtime should not require live
Movebank access. A preparation script should download or import a selected public dataset and export
a compact Datoviz-ready cache.

Candidate dataset families:

- bird migration tracks,
- stork, eagle, hawk, or seabird GPS trajectories,
- marine turtle or seal tracks,
- ungulate seasonal migration,
- foraging-trip datasets with repeated outbound/return paths.

The final Stage 1 dataset should be chosen by license, size, public availability, and visual
clarity. The spec should record the dataset DOI, license, source URL, species, and citation
metadata in the prepared cache.


## User-Facing Scenario

The default scene should show:

- a regional map or simple coastline/land background,
- many individual animal trajectories as colored paths,
- current positions moving during replay,
- the selected individual highlighted,
- recent trail segments brighter than older history,
- a linked speed/altitude/distance panel,
- a time slider and play/pause controls.

The strongest screenshot should show recognizable migration corridors or foraging trips, not random
lines over a map.


## Scene Layout

Recommended layout:

```text
+------------------------------------------------------------------+
| map / regional projection                                         |
| trajectories, current positions, selected individual, environment |
+------------------------------------------------------------------+
| movement metric panel                                             |
| speed / altitude / distance / selected individual over time        |
+------------------------------------------------------------------+
| individual summary / legend                                       |
+------------------------------------------------------------------+
```

Minimum viable version:

1. one 2D map panel with panzoom,
2. long trajectories as paths,
3. current animal positions as points at the replay time,
4. time slider or playback,
5. selected individual highlight and readout.

Preferred fuller version:

1. linked movement-metric panel,
2. optional environmental raster background,
3. recent-trail fading,
4. stopover or foraging-cluster markers,
5. color modes by speed, altitude/depth, elapsed time, or movement state,
6. brush/select a time interval.


## Data Strategy

### Stage 1: Prepared Public Tracking Cache

The first implementation should use a small prepared cache generated from a public tracking dataset.

Suggested cache layout:

```text
~/.cache/datoviz/ecology/animal_migration/
  metadata.json
  track_position_f64.bin       # lon, lat, optional altitude/depth
  track_time_i64.bin           # timestamp per fix
  track_id_u32.bin             # individual id per fix, if not offset-only
  track_offsets_u32.bin        # packed trajectory offsets per individual
  track_attr_f32.bin           # speed, altitude/depth, distance, heading, state
  individual_table.json        # species, individual id, sex/age/tag metadata if available
  environment_rgba8.bin        # optional map/environment raster
  environment_meta.json        # optional georeference and scalar metadata
  coastline_f32.bin            # optional packed lon/lat coastline paths
  coastline_offsets_u32.bin
```

Recommended metadata:

```text
source
source_url
doi
license
citation
species
n_individuals
n_fixes
time_start
time_end
coordinate_reference
projection
cache_layout_version
preparation_script
```


### Stage 2: Environmental Context

A later preparation step may add environmental context aligned with the tracking domain:

- land/sea mask,
- coastline paths,
- topography or bathymetry,
- temperature,
- wind support,
- NDVI or vegetation index,
- sea-ice extent for polar tracks.

The environmental layer should be optional. The animal tracks must remain visible and useful without
it.


### Synthetic Fallback

If no public cache is available, generate deterministic synthetic migration tracks:

- several individuals,
- seasonal migration corridors,
- stopover clusters,
- speed variation,
- noisy GPS fixes,
- optional altitude/depth profiles.

The fallback is only for robustness. The preferred showcase should use a real public dataset.


## Runtime Data Model

Logical source data:

```text
lon_lat_alt        float64[n_fixes, 3]
timestamp          int64[n_fixes]
individual_id      uint32[n_fixes]
track_offsets      uint32[n_individuals + 1]
speed              float32[n_fixes]
heading            float32[n_fixes]
distance           float32[n_fixes]
movement_state     uint8[n_fixes]
```

Derived render data:

```text
path_position      float32[n_visible_vertices, 3]
path_color         uint8[n_visible_vertices, 4]
current_position   float32[n_individuals, 3]
current_color      uint8[n_individuals, 4]
recent_trail       optional path subset around replay time
selected_path      optional highlighted path geometry
metric_panel       path or marker geometry for linked time series
```

The cache may store original longitude/latitude in double precision. Rendering should use a local
or projected float coordinate system for stability.


## Visual Encodings

Map panel:

```text
trajectory path     one path per individual or packed grouped paths
current position    point marker at current replay time
recent trail        brighter or thicker path near current time
selected animal     highlighted path and marker
environment         optional image or coastline overlay
```

The active trajectory target is [`path`](../../visuals/PATH.md). Future radius-bearing 3D
trajectory tubes should follow [`TUBE.md`](../../visuals/TUBE.md) rather than defining a
movement-ecology-specific renderer.

Color modes:

- individual identity,
- speed,
- altitude or depth,
- elapsed time,
- movement state,
- selected vs unselected.

Movement metric panel:

```text
x       time
y       speed, altitude/depth, cumulative distance, or selected metric
cursor  current replay time
marker  selected fix or event
```


## Time Replay

The time cursor should drive the whole scene:

```text
global time -> current position per individual -> recent trail segment -> metric cursor
```

Replay modes:

1. **Full History**
   - show all trajectories faintly,
   - current positions move along them,
   - selected individual is highlighted.

2. **Recent Trail**
   - show only a trailing time window brightly,
   - old tracks fade or remain as context.

3. **Interval Brush**
   - show only fixes inside a selected time range.

Interpolation between fixes is optional for MVP. A later version may interpolate current positions
linearly between timestamps.


## Interactivity

Required MVP interactions:

1. panzoom map navigation,
2. play/pause replay,
3. time slider,
4. select individual by click or list,
5. show all tracks vs selected track,
6. color mode selector,
7. selected fix or individual readout.

Preferred interactions:

1. hover probe for nearest track fix,
2. recent-trail length slider,
3. speed/altitude filtering,
4. movement-state toggles,
5. linked metric-panel selection,
6. reset map view,
7. export clip.


## Picking And Readout

Picking should identify the nearest individual trajectory or current-position marker.

Readout should include:

```text
individual id
species
timestamp
longitude
latitude
altitude or depth, if available
speed
heading
cumulative distance
movement state
nearest stopover or cluster, if available
```

Selection should highlight:

- the full selected trajectory,
- the current position marker,
- the corresponding metric time series,
- the selected fix in the metric panel when applicable.


## Movement Metrics

The preparation script or runtime may compute:

```text
speed
heading
step length
turning angle
cumulative distance
stopover/cluster id
movement state
```

These metrics should be derived data, not replacements for the source trajectory records.


## FramePlan Shape

### Static Setup

Initial frame:

```text
UploadNode  -> trajectory path positions and colors
UploadNode  -> current position markers
UploadNode  -> coastline/environment resources, if available
UploadNode  -> metric panel geometry
RenderNode  -> map panel
RenderNode  -> metric panel
RenderNode  -> legend/summary panel
```


### Replay Frame

When the time cursor advances:

```text
UploadNode  -> current animal positions
UploadNode  -> recent-trail geometry, if enabled
UploadNode  -> metric-panel time cursor
RenderNode  -> affected panels
```

Full historical trajectories, coastline paths, and environmental rasters should remain static.


### Selection Change

When an individual or fix is selected:

```text
UploadNode  -> selected trajectory highlight
UploadNode  -> selected current marker
UploadNode  -> linked metric-panel highlight
RenderNode  -> affected panels
```

No full trajectory reupload should be required for selection-only changes.


### Color Mode Change

When color mode changes:

```text
UploadNode  -> trajectory colors
UploadNode  -> marker colors
UploadNode  -> legend/colorbar resources
RenderNode  -> affected panels
```


## DRP2 Command Categories

The example is expected to require:

- path buffers for long trajectories and recent trails,
- point buffers for current positions and selected fixes,
- optional image resources for environmental rasters,
- optional path resources for coastlines or boundaries,
- repeated uploads for replay markers and recent trails,
- panel transform updates for panzoom,
- readback/pick requests for trajectory or marker selection,
- optional capture/video commands through the app/canvas layer.


## Implementation Notes

The first C implementation can stay focused:

1. load one prepared public tracking cache,
2. project lon/lat to stable local coordinates,
3. render all trajectories as [`path`](../../visuals/PATH.md) visuals,
4. update current-position markers from the time cursor,
5. implement individual selection,
6. add the linked movement-metric panel after the map view is stable.

The preparation script should handle dataset-specific field names and normalize them into the stable
cache schema described here.


## Key Pressure On The Scene Spec

This example checks that Datoviz v0.4 can express a movement-ecology workflow where:

- long trajectories and moving current positions share one time cursor,
- individual identity remains stable across path, marker, and metric-panel resources,
- replay updates small dynamic resources while historical tracks remain static,
- map/projection handling stays separate from rendering,
- real public tracking data can be prepared into compact scene resources.
