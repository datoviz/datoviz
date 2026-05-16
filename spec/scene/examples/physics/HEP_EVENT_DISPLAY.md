# High-Energy Physics Event Display

> **Agent Pickup**
> - **Category:** `physics`
> - **Implementation target:** Scientific domain example with staged implementation, starting from deterministic or prepared data.
> - **Data policy:** Synthetic or prepared-cache first slice; real public data may be a second stage with license notes.
> - **Preprocessing:** Document any data conversion script, output schema, coordinate normalization, and cache validation.
> - **Validation:** Smoke run plus domain-specific visual, picking/probe, and performance acceptance criteria.

## Summary

Build a high-energy physics event-display scene example using CMS Open Data event-display JSON that
has been converted into compact Datoviz-ready arrays. The prepared cache should preserve provenance
and contain flattened event offsets, track paths, calorimeter deposits, hits or segments, jets,
missing transverse energy, and optional semantic object indices; runtime should not require ROOT,
CMSSW, or arbitrary CERN JSON parsing. The first practical slice should load the CMS SingleElectron
sample cache, show simplified transparent detector geometry in one 3D arcball panel, render tracks
and calorimeter objects, provide previous/next event controls, and expose selected-object readout.
Validate with smoke execution, CMS event-display visual checks, picking/probe checks, provenance
checks, and staged performance criteria.


## Example Name

`HEP_EVENT_DISPLAY`


## Purpose

Specify a Datoviz v0.4 showcase example for high-energy physics event visualization. The example
uses real CMS event-display data from the CERN Open Data Portal, converts it into compact
Datoviz-ready arrays, and renders collision events as interactive detector geometry, reconstructed
tracks, calorimeter deposits, jets, missing transverse energy, and linked projection panels.

This should not be a generic particle simulation. The scientific identity is:

```text
CMS Open Data event-display JSON -> reconstructed objects -> detector context -> linked event display
```


## Why This Example Exists

This example fills a gap in the showcase set: high-energy physics detector-event visualization with
real open data.

It should pressure:

1. many 3D paths for reconstructed tracks and muon segments,
2. transparent detector geometry,
3. primitive or mesh rendering for calorimeter energy deposits,
4. linked 2D and 3D event projections,
5. event stepping with per-event resource replacement,
6. semantic object picking,
7. visibility filters by physics-object family,
8. compact caches derived from real event-display JSON.


## Stage 1 Data Source

Stage 1 should use CMS event-display JSON files from the CERN Open Data Portal, not synthetic data.

These files are small derived datasets intended for the browser-based CMS 3D event display. They are
much lighter than full ROOT/AOD workflows and avoid requiring CMSSW or ROOT at Datoviz runtime.

Recommended default record:

```text
CMS SingleElectron event-display file derived from /SingleElectron/Run2012C-22Jan2013-v1/AOD
https://opendata.cern.ch/record/7144
```

Good alternatives:

```text
CMS DoubleElectron event-display file derived from /DoubleElectron/Run2012C-22Jan2013-v1/AOD
https://opendata.cern.ch/record/7128

CMS SingleMu event-display file derived from /SingleMu/Run2012C-22Jan2013-v1/AOD
https://opendata.cern.ch/record/7145

CMS MuEG event-display file derived from /MuEG/Run2012C-22Jan2013-v1/AOD
https://opendata.cern.ch/record/7139
```

Reference documentation:

```text
CERN Open Data Portal overview
https://opendata.web.cern.ch/docs/about

CMS event display entry point
https://opendata.cern.ch/visualise/events/CMS

CMS Open Data information
https://opendata.cern.ch/research/CMS
```

The spec should record source URLs and citation metadata in the prepared cache. The first Datoviz
runtime example should read the prepared cache, not parse arbitrary CERN JSON directly unless a
small local parser is already available and robust.


## Recommended Default Scenario

Use the CMS SingleElectron Run2012C event-display sample as the default.

Rationale:

- the dataset is real CMS 8 TeV proton-proton collision data,
- the files are small enough for an example cache,
- electron/photon-like objects are visually understandable,
- tracks and calorimeter deposits give a strong event-display screenshot,
- the source is official CERN Open Data rather than a synthetic toy.

Synthetic events should exist only as a fallback when no cache is available.


## User-Facing Scenario

The default scene should show:

- simplified transparent CMS detector geometry,
- reconstructed tracks as curved or segmented 3D paths,
- calorimeter energy deposits as colored towers or blocks,
- muon segments and hits when present,
- jets as direction cones or vectors,
- missing transverse energy as an arrow in the transverse plane,
- an event selector for previous/next event,
- object-family visibility toggles,
- object picking and linked readout.

The strongest screenshot should look like a compact CMS event display, with real event objects
inside detector context.


## Scene Layout

Recommended layout:

```text
+------------------------------------------------------------------+
| 3D CMS event view                                                 |
| detector cylinders, tracks, calorimeter deposits, jets, MET        |
+------------------------------------------------------------------+
| transverse or eta-phi projection                                  |
+------------------------------------------------------------------+
| event/object summary                                               |
+------------------------------------------------------------------+
```

Minimum viable version:

1. one 3D panel with arcball interaction,
2. simplified detector barrel/endcap geometry,
3. reconstructed tracks as paths,
4. calorimeter deposits as points or primitive boxes,
5. previous/next event controls,
6. object pick or selected-object readout.

Preferred fuller version:

1. linked transverse `r-phi` or `x-y` panel,
2. linked `eta-phi` calorimeter energy panel,
3. object table or histogram panel,
4. family filters for tracks, muons, jets, calo, MET, detector,
5. selected object highlight across panels,
6. animated reveal from interaction point outward.


## Data Preparation Strategy

### Stage 1: CERN JSON To Datoviz Cache

The Stage 1 workflow should be:

1. download one official CMS event-display JSON file from CERN Open Data,
2. cache the original file locally,
3. parse it with a preparation script,
4. flatten events into compact binary arrays,
5. record provenance and source URLs in `metadata.json`,
6. run the C example against the compact Datoviz cache.

Suggested cache layout:

```text
~/.cache/datoviz/hep/cms_singleelectron_2012c/
  metadata.json
  original_event_display.json
  event_offsets_u32.bin
  track_position_f32.bin       # packed 3D path samples
  track_offset_u32.bin         # one offset per track path
  track_attr_f32.bin           # charge, pt, eta, phi, type id, event id
  calo_position_f32.bin        # cell/tower centers or box transforms
  calo_energy_f32.bin          # energy and layer/type ids
  hit_position_f32.bin         # detector hits or segments, if available
  hit_attr_f32.bin
  jet_attr_f32.bin             # pt, eta, phi, energy, event id
  met_f32.bin                  # magnitude, phi, event id
  object_index_u32.bin         # optional semantic object lookup
```

Recommended metadata:

```text
source
source_url
record_id
dataset_name
experiment
collision_energy
run_period
license
n_events
object_families
cache_layout_version
preparation_script
```


### Runtime Policy

The runtime example should:

1. look for the prepared cache,
2. optionally trigger or instruct the preparation step if it is missing,
3. load compact arrays directly,
4. never require ROOT or CMSSW,
5. fall back to a small synthetic event only if the real cache cannot be prepared.

The cache should be small enough for normal example usage.


## Object Families

The exact object names depend on the CMS event-display JSON schema. The preparation script should
map available objects into a stable Datoviz event-display schema.

Expected or useful families:

```text
tracks
electrons
photons
muons
standalone_muons
global_muons
jets
calorimeter_deposits
missing_transverse_energy
hits_or_segments
```

If some families are absent in a selected file, the runtime should omit them gracefully.


## Detector Geometry

Stage 1 should use simplified generated detector geometry rather than a full CMS geometry model.

Suggested geometry:

- beamline as a thin path,
- tracker as translucent nested cylinders,
- electromagnetic calorimeter barrel and endcaps as translucent cylinders/disks,
- hadronic calorimeter as a larger translucent shell,
- muon system as outer cylinders/disks,
- optional eta/phi reference rings.

The detector geometry provides context only. It should not dominate event objects.


## Visual Encodings

3D event panel:

```text
tracks          colored 3D paths, optionally by charge or pt
muon segments   longer outer paths or hit points
calo deposits   boxes, bars, or points colored and scaled by energy
jets            cones, rays, or highlighted directions
MET             arrow in transverse plane
detector        transparent cylinders/disks/wireframes
selection       brighter object, outline, or overlay marker
```

Projection panels:

```text
r-phi / x-y     transverse projection of tracks and MET
eta-phi         calorimeter energy or object positions
histogram       object pt or energy distribution
```

Default colors should follow familiar HEP conventions where practical:

- charged tracks: multi-color or charge-colored,
- electrons/photons: yellow/orange,
- muons: blue/cyan,
- jets/calo: energy colormap,
- MET: red or magenta arrow,
- detector: muted translucent gray.


## Interactivity

Required MVP interactions:

1. arcball/orbit camera,
2. previous/next event,
3. show/hide detector geometry,
4. show/hide object families,
5. select object by click or UI list,
6. selected object readout.

Preferred interactions:

1. hover picking for tracks, calo deposits, jets, and MET,
2. linked highlight across 3D and projection panels,
3. pt or energy threshold sliders,
4. event autoplay,
5. reset camera,
6. object table selection.


## Picking And Readout

Picking should resolve the visual hit to a semantic event object.

Track readout:

```text
event id
object family
track id
charge
pt
eta
phi
number of samples or hits
```

Calorimeter readout:

```text
event id
object family
cell/tower id
energy
eta
phi
layer
```

Jet or MET readout:

```text
event id
object family
pt or missing transverse energy
eta
phi
energy, when available
```

Selected objects should be highlighted in all linked panels where they appear.


## Event Stepping

The event selector should replace visible event resources without rebuilding static detector
geometry.

When changing events:

- event-specific tracks, hits, calo deposits, jets, and MET update,
- detector geometry remains unchanged,
- projection panels update,
- selected object state resets unless the selection can be mapped to the new event.

Event stepping should be deterministic and fast enough for quick browsing.


## Animation Modes

Animation is optional but useful.

Event autoplay:

```text
display event N for a fixed duration, then advance to N + 1
```

Object reveal:

```text
tracks grow outward from the interaction point
calorimeter deposits appear after tracks
muon segments appear last
```

Object reveal is a display animation only. It should not alter the event data model.


## FramePlan Shape

### Static Setup

Initial setup:

```text
UploadNode  -> detector geometry vertices, indices, colors
UploadNode  -> event object arrays for selected event
UploadNode  -> projection panel geometry
UploadNode  -> object summary panel data
RenderNode  -> 3D event panel
RenderNode  -> projection panels
RenderNode  -> summary panel
```


### Event Change

When the selected event changes:

```text
UploadNode  -> track paths for new event
UploadNode  -> calo deposit primitives for new event
UploadNode  -> hit, jet, and MET geometry for new event
UploadNode  -> projection and summary panel data
RenderNode  -> affected panels
```

Detector geometry should remain static.


### Visibility Or Threshold Change

When an object-family toggle or threshold changes:

```text
UploadNode  -> visibility/style resources, if available
UploadNode  -> filtered object geometry only if subset hiding is unavailable
RenderNode  -> affected panels
```


### Selection Change

When an object is selected:

```text
UploadNode  -> highlight geometry or selection style buffer
UploadNode  -> linked projection/object-table highlight
RenderNode  -> affected panels
```

No full event reupload should be required for selection-only changes.


## DRP2 Command Categories

The example is expected to require:

- buffers for detector meshes, track paths, calo primitives, hit points, jets, MET, and projections,
- draw commands for path, point, primitive, mesh, and annotation-style visuals,
- dynamic uploads for event changes, filters, and selection highlights,
- panel transform updates from arcball and panzoom controllers,
- optional readback/pick requests for semantic object selection,
- optional capture/video commands through the app/canvas layer.


## Implementation Notes

The first C implementation can stay focused:

1. prepare one CMS SingleElectron or DoubleElectron JSON sample into a Datoviz cache,
2. render simplified detector geometry,
3. render tracks and calorimeter deposits for one selected event,
4. implement previous/next event stepping,
5. add object-family toggles,
6. add picking and linked projection panels after the 3D event view is stable.

The preparation script should be tolerant of schema differences between CMS event-display files and
should preserve unknown fields in metadata when practical.


## Key Pressure On The Scene Spec

This example checks that Datoviz v0.4 can express a real-event display workflow where:

- real CERN Open Data can be flattened into compact scene resources,
- static detector context and dynamic event objects have separate lifecycles,
- many 3D paths and primitive deposits can be replaced per event,
- semantic picking resolves visual geometry back to physics objects,
- linked projections and summary panels share event and selection state.
