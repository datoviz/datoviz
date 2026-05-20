# High-Energy Physics Event Display

> **Example status:** informative pressure test
> **Target:** C showcase plus preparation script
> **Data:** CMS Open Data prepared cache with synthetic fallback
> **Validation:** smoke, visual/picking/provenance/performance checklist

See [../SHARED_POLICIES.md](../SHARED_POLICIES.md) for shared worked-example policy.


## Summary

Render CMS Open Data event-display JSON that has been converted into compact Datoviz-ready arrays.
The runtime should not require ROOT, CMSSW, or arbitrary CERN JSON parsing. The first slice shows
simplified transparent detector geometry, tracks, calorimeter deposits, previous/next event controls,
family visibility toggles, and selected-object readout in one 3D arcball panel.


## User-Visible Result

- A compact CMS-like event display opens with detector context and real event objects.
- Tracks render as 3D paths; calorimeter deposits render as colored/scaled points, boxes, or bars.
- Optional muon segments, jets, and missing transverse energy render when present.
- Users can step events, toggle object families, inspect one selected object, and later see linked
  `r-phi`, `x-y`, `eta-phi`, or summary panels.


## Feature Pressure Points

- Many 3D paths, primitive deposits, transparent detector geometry, and linked projection panels.
- Separate static detector lifecycle from dynamic per-event resources.
- Event stepping with deterministic replacement of tracks, hits, calo deposits, jets, and MET.
- Semantic picking that resolves visual hits to physics object ids.
- Visibility/threshold filters without unnecessary full-scene rebuilds.


## Required Data And Resources

Default Stage 1 source:

```text
CMS SingleElectron event-display file derived from /SingleElectron/Run2012C-22Jan2013-v1/AOD
https://opendata.cern.ch/record/7144
```

Good alternatives: CMS DoubleElectron record 7128, CMS SingleMu record 7145, CMS MuEG record 7139.
Record CERN Open Data citation, source URL, record id, dataset name, experiment, collision energy,
run period, license, event count, object families, cache version, and preparation script.

Suggested cache:

```text
~/.cache/datoviz/hep/cms_singleelectron_2012c/
  metadata.json
  original_event_display.json
  event_offsets_u32.bin
  track_position_f32.bin
  track_offset_u32.bin
  track_attr_f32.bin
  calo_position_f32.bin
  calo_energy_f32.bin
  hit_position_f32.bin
  hit_attr_f32.bin
  jet_attr_f32.bin
  met_f32.bin
  object_index_u32.bin
```

Expected families include tracks, electrons, photons, muons, standalone/global muons, jets,
calorimeter deposits, MET, and hits/segments. Missing families should be omitted gracefully.


## Scene Layout

Minimum viable layout:

```text
3D CMS event view: detector cylinders, tracks, calo deposits, jets, MET
```

Preferred layout:

```text
3D event view
transverse or eta-phi projection
event/object summary
```

Simplified detector geometry should include beamline, tracker cylinders, ECAL/HCAL barrel and
endcap shells, muon-system shells, and optional eta/phi reference rings. Detector context should be
muted and not dominate event objects.


## Visual Encodings

| Object | Encoding |
| --- | --- |
| Tracks | colored 3D paths, optionally by charge or `pt` |
| Muon segments | outer paths or hit points |
| Calo deposits | boxes, bars, or points colored/scaled by energy |
| Jets | cones, rays, or highlighted directions |
| MET | red/magenta transverse-plane arrow |
| Detector | muted translucent cylinders/disks/wireframes |
| Selection | brighter object, outline, or overlay marker |

Use familiar HEP colors where practical: electrons/photons yellow-orange, muons blue/cyan,
jets/calo energy colormap, detector translucent gray.


## Interactivity And Readout

Required interactions:

- arcball/orbit camera,
- previous/next event,
- show/hide detector,
- show/hide object families,
- click or list selection,
- selected-object readout.

Preferred additions: hover picking, linked projection highlight, `pt`/energy thresholds, autoplay,
reset camera, object table selection, and reveal animation.

Readout should report event id, object family, object id, and family-specific fields such as charge,
`pt`, `eta`, `phi`, hit/sample count, cell/tower id, energy, layer, or MET magnitude.


## Scene And Runtime Behavior

Use the normal scene -> FramePlan -> DRP2 path without restating the protocol; see
[../../pipeline/FRAME_PLAN.md](../../pipeline/FRAME_PLAN.md) and [../../drp2/](../../drp2/).

- Initial setup uploads static detector geometry and the selected event resources.
- Event changes update only event-specific tracks, hits, calo, jets, MET, projections, and summary
  data; detector geometry remains static.
- Visibility/threshold changes update style resources or filtered geometry only where necessary.
- Selection changes update highlight resources and linked panels, not the whole event.


## Minimal Implementation Target

1. Prepare one CMS SingleElectron or DoubleElectron JSON sample into the cache schema.
2. Render simplified detector geometry.
3. Render tracks and calorimeter deposits for one selected event.
4. Implement previous/next event stepping.
5. Add object-family toggles.
6. Add picking and linked projections after the 3D view is stable.


## Validation

- Smoke run loads the prepared cache, opens nonblank 3D view, steps at least two events, and tears
  down cleanly.
- Visual check resembles a compact CMS event display with detector context.
- Provenance metadata names CERN source, record id, dataset, license, and event count.
- Object picking returns semantic ids and stable family-specific readout.
- Event stepping is deterministic and does not rebuild detector resources.
