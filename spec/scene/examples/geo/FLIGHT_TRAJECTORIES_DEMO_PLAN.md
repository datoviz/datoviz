# Flight Trajectories Geographic Demo Plan

> **Agent Pickup**
> - **Category:** `geo`
> - **Implementation target:** Polished demo concept; implement in stages so the first slice can run with bounded resources.
> - **Data policy:** Public/downloaded assets require cache metadata and an offline fallback or reduced fixture.
> - **Preprocessing:** Usually required; specify source download, conversion, decimation/packing, and generated cache files.
> - **Validation:** Manual visual checklist plus bounded smoke command; add screenshot/readback validation when feasible.


## Summary

Build a deterministic air-traffic trajectory demo with regional map context, path rendering, and
animated aircraft positions. The real-data path should preprocess OpenSky-style ADS-B tracks and
optional OurAirports metadata into a compact cache; the runtime example must not call a live API,
and the first slice can use prepared or synthetic tracks if access is unresolved. Start with a 2D
panzoom regional view showing a few hours of trajectories, moving oriented markers, and color by
altitude, speed, or time. Validate with a smoke run plus manual checks for map alignment, track
splitting, replay continuity, marker orientation, and stable scene updates.


Date: 2026-05-16

## Goal

Build a Datoviz demo around real air traffic trajectories to showcase geographic rendering, animated
paths, image or sprite markers, retained scene updates, and 2D-to-3D visualization workflows.

This should be a practical demo, not a live API client. The checked-in example should load a small,
preprocessed dataset so it is deterministic, fast, and usable in CI/manual smoke testing.

## Demo Shape

Start with a regional 2D map view before attempting a full globe. A regional view is enough to prove
the core value while avoiding early complexity around Earth meshes, map textures, antimeridian
wrapping, and camera framing.

Initial target:

- Render a few hours of real flight tracks over Europe, the US Northeast, or the US West Coast.
- Draw each flight as a path/line strip.
- Animate aircraft positions along the trajectories.
- Use a small aircraft bitmap, sprite, or oriented glyph for moving planes.
- Color trajectories or aircraft by altitude, speed, time, airline, or flight phase.
- Add airport dots and optional labels from a separate airport metadata source.
- Support pan/zoom first; keep arcball globe as a second phase.
- Add hover/picking later for callsign, altitude, speed, and timestamp.

## Dataset Candidates

### OpenSky Network

Best first candidate for real ADS-B trajectory data. OpenSky provides historical state-vector data and
research-oriented datasets that can be converted into compact trajectory samples.

Use it for:

- timestamped latitude/longitude/altitude samples,
- callsign or ICAO24 identifiers when available,
- velocity and heading where available,
- realistic dense trajectory rendering.

Notes:

- Origin/destination metadata may be incomplete or require inference.
- A preprocessing step should downsample, split tracks by aircraft/flight, and write a compact demo
  fixture.

### OurAirports

Good companion dataset for airport metadata. It is useful for airport dots, labels, and contextual
geographic anchors.

Use it for:

- airport coordinates,
- names,
- IATA/ICAO codes,
- country/region filtering.

Notes:

- This is better for static airport context than for trajectory data.

### ADS-B Community History Dumps

Community ADS-B history datasets may be useful if OpenSky access is inconvenient. They need careful
license review before bundling derived data, especially if the source is under ODbL or similar
share-alike terms.

Use it for:

- public daily/hourly samples,
- dense live-like traffic slices,
- demos where attribution/share-alike obligations are acceptable.

### EUROCONTROL Research Data

High-quality European trajectory data, but likely less suitable for a bundled public demo because
access and redistribution terms are more constrained.

Use it for:

- research-only experiments,
- internal comparison,
- validation of European traffic examples if access is available.

## Proposed Files

Potential example:

```text
examples/scene/flight_trajectories.c
```

Potential data fixture:

```text
data/flights/europe_2h.bin
data/flights/europe_2h.json
```

Potential preprocessing script:

```text
tools/make_flight_demo_dataset.py
```

The C example should not download data at runtime. The preprocessing script can document source URLs,
license notes, attribution requirements, downsampling, and conversion to the Datoviz-friendly fixture
format.

## Rendering Plan

Phase 1: regional 2D retained scene

- Load preprocessed flight samples.
- Convert latitude/longitude to a local projected coordinate system.
- Draw airports as points or markers.
- Draw trajectories as retained paths.
- Animate aircraft sprites or oriented glyphs by interpolating along each track.
- Expose a time slider or fixed animation clock.

Phase 2: richer geographic view

- Add altitude-based colormap and colorbar.
- Add hover/pick readouts for active aircraft.
- Add endpoint markers, track fading, or recent trail emphasis.
- Add route filtering by airport, airline, altitude band, or time range.

Phase 3: globe variant

- Reuse the same processed trajectory fixture.
- Render Earth as a sphere with a texture or simple shaded surface.
- Convert latitude/longitude/altitude to 3D positions.
- Draw raised arcs or polyline tracks above the globe.
- Use arcball interaction and depth-tested aircraft glyphs.

## Data Format Sketch

Keep the runtime format simple and C-friendly:

```c
typedef struct FlightTrackHeader
{
    uint32_t track_count;
    uint32_t sample_count;
    double t0;
    double t1;
} FlightTrackHeader;

typedef struct FlightSample
{
    float t;
    float lon;
    float lat;
    float altitude_m;
    float speed_mps;
    float heading_rad;
    uint32_t track_id;
} FlightSample;
```

The JSON sidecar can contain callsigns, airport metadata references, source attribution, projection
bounds, and units. The binary file can remain focused on fast sample upload and animation.

## Why This Is Useful For Datoviz

- Exercises retained path rendering with many line strips.
- Exercises animated partial updates for moving aircraft.
- Exercises image/glyph visual support with rotation and per-instance state.
- Demonstrates geographic coordinates without requiring a dedicated map renderer.
- Provides a natural test bed for picking/probing and hover feedback.
- Can grow from 2D regional maps into 3D globe rendering without changing the core dataset.

## Open Questions

- Which dataset can be redistributed in a small checked-in fixture with acceptable attribution?
- Should the first aircraft marker be a bitmap image, a vector-like glyph, or a tiny mesh?
- Should the first projection be equirectangular, Web Mercator, or a local tangent projection?
- Should this live under `examples/scene/`, `examples/app/`, or a future geographic examples folder?
- How much UI should the first version expose: fixed animation, keyboard controls, or an app-level
  time slider?

## Recommended Next Step

Create a tiny offline fixture from one real dataset source, ideally OpenSky plus OurAirports. Keep it
small enough for the repository, then implement the 2D regional demo first. The globe version should
reuse the same preprocessed trajectory model once the regional path and sprite animation are stable.
