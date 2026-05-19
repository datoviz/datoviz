# Screen-Space Effects

This document defines the user-visible semantics for optional scene screen-space effects:

1. outline rendering;
2. screen-space edge enhancement;
3. bloom.

These effects provide perceptual and interaction-driven enhancement. They are not part of the base
visual semantics and should remain opt-in, panel-local by default, and reproducible across
interactive rendering and export.

Implementation-facing graph details live in
[`../implementation/GRAPH_TECHNIQUES.md`](../implementation/GRAPH_TECHNIQUES.md).

## Common Rules

Screen-space effects are retained panel technique settings, not per-frame immediate commands.

Common rules:

1. effects are off by default;
2. effects are represented in retained scene state;
3. users should not create graph passes, intermediate textures, or fullscreen pipelines for the
   common path;
4. exact image inspection, probe, and quantitative readback workflows must be able to disable or
   bypass presentation effects;
5. export behavior must be explicit and reproducible;
6. effects must respect panel viewport and scissor boundaries;
7. effects must not affect picking, probing, colorbar mapping, or readback identity unless a future
   effect explicitly defines such behavior.

The first public API should be panel-level and typed. Exact names may change during API review, but
the semantic shape should remain descriptor-based panel technique state.

## Outline Rendering

Outline rendering provides explicit boundary emphasis for hovered, selected, or otherwise marked
scene identities.

Recommended first scope:

1. selected object outline;
2. hovered object outline;
3. selected item outline when the visual can produce stable item IDs;
4. linked-selection outline through scene-owned selection state.

Rules:

1. outline state is derived from scene identity, selection, hover, or explicit highlight state;
2. outline rendering must not change authoritative visual color, geometry, or picking results;
3. outlines must respect panel viewport and scissor boundaries;
4. outlines must not bleed across panels;
5. hidden or clipped fragments should not outline unless the effect explicitly requests
   through-wall behavior;
6. transparent visual outlines need an explicit policy before they are enabled.

The preferred implementation basis is mask or ID-buffer rendering followed by screen-space dilation
or edge detection and composite. Family-specific analytic outlines may still be useful for point,
sphere impostor, and text visuals later, but they should not be the only general outline mechanism.

Open semantic choices:

1. whether target selection is a bitset of hover, selection, explicit visual flags, and annotations;
2. whether outlines use object IDs, selection masks, or both in the first implementation;
3. how face-level mesh selection maps to outline masks;
4. how outline precedence composes hover with persistent selection.

## Screen-Space Edge Enhancement

Screen-space edge enhancement marks depth and/or normal discontinuities in the rendered panel.
Unlike outline rendering, it is not tied to interaction state.

Useful cases:

1. low-contrast 3D meshes and surface shells;
2. dense overlapping geometry;
3. silhouettes that remain ambiguous with SSAO alone;
4. region boundaries in segmented surfaces;
5. downscaled screenshots or publication figures.

Rules:

1. edge enhancement is off by default;
2. the effect is panel-local and applied after base opaque/depth-producing passes;
3. normal edges should only be used when the normal source is known and stable;
4. depth thresholds should be expressed in a space that behaves predictably under camera changes;
5. the effect must respect panel viewport and scissor boundaries;
6. the effect should be disabled or excluded for exact image inspection unless requested.

Relationship to SSAO:

1. SSAO remains the local occlusion/cavity cue;
2. edge enhancement remains the discontinuity/boundary cue;
3. both may be enabled together, but edge enhancement should normally run after SSAO composite.

Recommended inputs are resolved panel color, linear or reconstructable depth, a normal buffer when
available, and optionally object IDs later for semantic region boundaries.

## Bloom

Bloom adds a blurred contribution from bright or emissive pixels. It is useful for astronomy,
fluorescence microscopy, particle events, highlighted traces, and presentation-oriented scenes, but
it can distort quantitative color interpretation.

Rules:

1. bloom is off by default;
2. bloom must be documented as a presentation effect unless the data mapping declares an
   emissive/intensity channel;
3. bloom should not affect picking, probing, colorbar mapping, or readback identity;
4. bloom export behavior must be controlled by the same panel technique state as interactive
   rendering;
5. exact image and scalar-field inspection workflows should be able to disable bloom.

Recommended first scope:

1. panel-level opt-in bloom over resolved scene color;
2. thresholded bright-pass extraction;
3. separable blur or mip-chain blur;
4. additive or energy-limited composite back into panel color.

Open choices:

1. whether the first implementation assumes LDR thresholding or introduces an HDR intermediate;
2. whether visuals can declare emissive channels before the broader material API exists;
3. whether bloom participates in screenshot export by default or requires an export flag.

## Ordering

Preferred default composition order for a panel with all relevant effects enabled:

```text
base opaque / transparent / volume composition
SSAO or EDL composite, when enabled
edge enhancement composite
bloom bright-pass and blur
outline mask generation
outline composite
external UI overlay slot
presentation or export
```

Rationale:

1. edge enhancement should see the shaded scene after ambient occlusion;
2. bloom should not blur selection outlines by default;
3. outlines should remain crisp and visible above bloom;
4. external UI remains outside the scene FramePlan and should not be affected by scene effects.

This ordering may be adjusted for specific export modes, but deviations must be explicit.

## Validation Expectations

Each implemented effect should have:

1. default-off FramePlan graph tests;
2. opt-in graph resource and pass creation tests;
3. DRP2 stream or runtime smoke coverage for graph lowering;
4. offscreen image-difference coverage on a deterministic scene;
5. panel-boundary tests for multi-panel figures;
6. export coverage when image export includes panel effects;
7. interaction coverage for hover and selection outlines.
