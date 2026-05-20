# Animal Migration Tracks

> **Example status:** informative pressure test
> **Target:** C showcase plus preparation script
> **Data:** public Movebank/prepared cache with synthetic fallback
> **Validation:** smoke, coordinate/replay/selection/cache checklist

See [../SHARED_POLICIES.md](../SHARED_POLICIES.md) for shared worked-example policy.


## Summary

Render movement-ecology trajectory data as individual animal tracks with time replay, current
positions, selection highlighting, movement metrics, and optional environmental context. The first
slice uses a regional map, colored paths per individual, current-position markers, simple playback,
and selected-individual readout.


## User-Visible Result

- A map/projection panel shows many animal trajectories and moving current positions.
- Playback advances a global time cursor; recent trails or full history remain readable.
- Selecting an animal highlights its path, marker, and linked metric series.
- Color modes include identity, speed, altitude/depth, elapsed time, movement state, and selection.
- A later metric panel shows speed, altitude/depth, distance, or another selected metric over time.


## Feature Pressure Points

- Long packed path rendering for many persistent individuals.
- Time-dependent marker/recent-trail updates while historical paths remain static.
- Stable individual identity across path, marker, metric panel, and readout resources.
- Map/projection handling separate from rendering.
- Optional image/environment rasters plus coastline/path overlays.
- Picking nearest trajectory or current-position marker.

The active trajectory visual target is [../../visuals/PATH.md](../../visuals/PATH.md). Future
radius-bearing 3D trajectory tubes should follow [../../visuals/TUBE.md](../../visuals/TUBE.md).


## Required Data And Resources

Preferred data source: a curated public Movebank or Movebank Data Repository dataset selected for
license, size, public availability, citation clarity, species metadata, and visual trajectory
structure. Runtime should not require live Movebank access.

Suggested cache:

```text
~/.cache/datoviz/ecology/animal_migration/
  metadata.json
  track_position_f64.bin
  track_time_i64.bin
  track_id_u32.bin
  track_offsets_u32.bin
  track_attr_f32.bin
  individual_table.json
  environment_rgba8.bin
  environment_meta.json
  coastline_f32.bin
  coastline_offsets_u32.bin
```

Metadata should record source URL, DOI, license, citation, species, individual/fix counts, time
range, coordinate reference, projection, cache version, and preparation script.

If no public cache is available, generate deterministic synthetic tracks with seasonal corridors,
stopover clusters, speed variation, noisy fixes, and optional altitude/depth profiles. The fallback
is for robustness, not the preferred showcase.


## Runtime Data Model

Source arrays:

```text
lon_lat_alt        float64[n_fixes, 3]
timestamp          int64[n_fixes]
individual_id      uint32[n_fixes]
track_offsets      uint32[n_individuals + 1]
speed/heading/distance/movement_state
```

Derived render arrays:

```text
path_position       float32[n_visible_vertices, 3]
path_color          uint8[n_visible_vertices, 4]
current_position    float32[n_individuals, 3]
current_color       uint8[n_individuals, 4]
recent_trail        optional path subset
selected_path       optional highlight geometry
metric_panel        path/marker geometry
```

Keep original lon/lat in double precision where useful; render from local/projected float
coordinates for stability.


## Scene Layout

Minimum viable layout:

```text
map/projection panel: trajectories, current positions, selected individual
```

Preferred layout:

```text
map/projection panel
movement metric panel
individual summary/legend
```

Optional environmental context includes land/sea mask, coastline, topography/bathymetry,
temperature, wind support, NDVI, or sea ice. Tracks must remain useful without it.


## Interactivity And Readout

Required interactions:

- panzoom map navigation,
- play/pause and time slider,
- select individual by click or list,
- show all tracks vs selected track,
- color mode selector,
- selected fix/individual readout.

Preferred additions: hover probe, recent-trail length slider, speed/altitude filters,
movement-state toggles, linked metric-panel selection, reset view, and export clip.

Readout should include individual id, species, timestamp, longitude, latitude, altitude/depth,
speed, heading, cumulative distance, movement state, and nearest stopover/cluster when available.


## Scene And Runtime Behavior

Use the normal scene pipeline; see [../../pipeline/FRAME_PLAN.md](../../pipeline/FRAME_PLAN.md) and
[../../drp2/](../../drp2/).

- Initial setup uploads trajectory paths, current markers, optional coastline/environment resources,
  and metric-panel geometry.
- Replay updates current markers, recent trails, and metric cursor only.
- Selection updates highlight path/marker/metric resources without full trajectory reupload.
- Color-mode changes update trajectory/marker colors and legend/colorbar resources.


## Minimal Implementation Target

1. Load one prepared public tracking cache or deterministic fallback.
2. Project lon/lat into stable local coordinates.
3. Render all trajectories as path visuals.
4. Update current-position markers from the time cursor.
5. Implement individual selection and readout.
6. Add linked movement-metric panel after the map view is stable.


## Validation

- Smoke run loads data, renders nonblank map, advances replay, selects one individual, and tears
  down cleanly.
- Coordinate projection aligns tracks with coastline/environment when present.
- Replay continuity keeps markers on the correct trajectories.
- Selection highlights path, marker, and metric consistently.
- Cache metadata includes source, DOI/citation/license, species, counts, and time range.
