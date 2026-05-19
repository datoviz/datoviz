> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-17`
> - **Purpose:** define the intended v0.4 scene-level model for outline rendering, screen-space
>   edge enhancement, and bloom as graph-backed panel effects.

# Screen-Space Effects Design

This note records the proposed scene contract for three optional screen-space effects:

1. outline rendering,
2. screen-space edge enhancement,
3. glow/bloom.

The goal is to keep these effects aligned with the existing scene technique architecture rather
than adding visual-family-specific render paths. The active stack already has MSAA, EDL, SSAO,
G-buffer resources, and a FramePlan graph, so these effects should be expressed as panel-local
techniques over graph resources.


## Objective

Support perceptual and interaction-driven enhancement for scientific visualization without making
those enhancements part of the base visual semantics.

Required properties:

1. effects are opt-in and panel-local by default,
2. effects are represented in retained scene state, not as backend commands,
3. FramePlan graph resources and passes are the only execution contract,
4. DRP2 and vklite remain generic lowering/runtime layers,
5. quantitative image and probe workflows can disable or bypass these effects,
6. export behavior is explicit and reproducible.


## Relationship To Existing Techniques

These effects should reuse the existing technique-planning lane described in
`docs/architecture/scene_techniques_materials.md` and
`agents/soon/effects/SCENE_TECHNIQUES_MATERIALS_PLAN.md`.

Existing techniques affect priority and scope:

1. MSAA already covers geometric raster aliasing, so FXAA is not part of this proposal.
2. SSAO already covers local contact and cavity depth perception.
3. EDL already provides point-cloud and depth-discontinuity emphasis.
4. G-buffer resources already provide the preferred basis for normal/depth-driven effects.

Screen-space edge enhancement is therefore not a replacement for SSAO. It is an optional boundary
cue for silhouettes, depth jumps, normal breaks, and object/region boundaries. Outline rendering is
primarily an interaction and selection cue. Bloom is a presentation/intensity cue and should remain
opt-in.


## Public Model

The first public API should be panel-level and typed. Users should not create arbitrary graph
passes, intermediate textures, or fullscreen pipelines for the common path.

Candidate public descriptors:

```c
typedef struct DvzOutlineDesc
{
    bool enabled;
    float width_px;
    DvzColor color;
    float alpha;
    uint32_t target_mask;
} DvzOutlineDesc;

typedef struct DvzEdgeEnhancementDesc
{
    bool enabled;
    float depth_threshold;
    float normal_threshold;
    float strength;
    DvzColor color;
} DvzEdgeEnhancementDesc;

typedef struct DvzBloomDesc
{
    bool enabled;
    float threshold;
    float intensity;
    float radius_px;
    uint32_t iterations;
} DvzBloomDesc;
```

Candidate entry points:

```c
DvzOutlineDesc dvz_outline_desc(void);
int dvz_panel_set_outline(DvzPanel* panel, const DvzOutlineDesc* desc);

DvzEdgeEnhancementDesc dvz_edge_enhancement_desc(void);
int dvz_panel_set_edge_enhancement(DvzPanel* panel, const DvzEdgeEnhancementDesc* desc);

DvzBloomDesc dvz_bloom_desc(void);
int dvz_panel_set_bloom(DvzPanel* panel, const DvzBloomDesc* desc);
```

The exact public names may change during API review. The semantic rule should not: these are
retained panel technique settings, not per-frame immediate commands.


## Outline Rendering

Outline rendering provides explicit boundary emphasis for hovered, selected, or otherwise marked
scene identities.

Recommended first scope:

1. selected object outline,
2. hovered object outline,
3. selected item outline when the visual can produce stable item IDs,
4. linked-selection outline through scene-owned selection state.

The preferred implementation is mask or ID-buffer based:

1. render selected/hovered targets into a panel-local mask or object-id texture,
2. run a screen-space dilation or edge-detect pass,
3. composite the outline over the panel color target.

This should be preferred over geometry inflation as the general path because it works across visual
families and stays compatible with item-level selection. Family-specific analytic outlines may still
be useful for point, sphere impostor, and text later, but they should not be the only mechanism.

Rules:

1. outline state is derived from scene identity, selection, hover, or explicit highlight state,
2. outline rendering must not change the authoritative visual color, geometry, or picking result,
3. outlines should respect panel viewport and scissor boundaries,
4. outlines must not bleed across panels,
5. hidden or clipped fragments should not outline unless the effect explicitly requests through-wall
   behavior,
6. transparent visuals need an explicit policy before they can produce outlines.

Open choices:

1. whether `target_mask` is a bitset of hover, selection, explicit visual flags, and annotations;
2. whether outlines use object IDs, selection masks, or both in the first implementation;
3. how face-level mesh selection maps to outline masks;
4. how outline precedence composes with hover and persistent selection.


## Screen-Space Edge Enhancement

Screen-space edge enhancement marks depth and/or normal discontinuities in the rendered panel.
Unlike outline rendering, it is not tied to interaction state.

Useful cases:

1. low-contrast 3D meshes and surface shells,
2. dense overlapping geometry,
3. silhouettes that remain ambiguous with SSAO alone,
4. region boundaries in segmented surfaces,
5. downscaled screenshots or publication figures.

Recommended inputs:

1. resolved panel color,
2. linear or reconstructable depth,
3. normal buffer when available,
4. optional object-id buffer later for semantic region boundaries.

Rules:

1. edge enhancement is off by default,
2. the effect is panel-local and applied after base opaque/depth-producing passes,
3. normal edges should only be used when the normal source is known and stable,
4. depth thresholds should be expressed in a space that behaves predictably under camera changes,
5. the effect must respect panel viewport/scissor boundaries,
6. the effect should be disabled or excluded for exact image inspection unless requested.

Relationship to SSAO:

1. SSAO remains the local occlusion/cavity cue,
2. edge enhancement remains the discontinuity/boundary cue,
3. both may be enabled together, but edge enhancement should normally run after SSAO composite.


## Bloom

Bloom adds a blurred contribution from bright or emissive pixels. It is useful for astronomy,
fluorescence microscopy, particle events, highlighted traces, and presentation-oriented scenes, but
it can distort quantitative color interpretation.

Recommended first scope:

1. panel-level opt-in bloom over the resolved scene color,
2. thresholded bright-pass extraction,
3. separable blur or mip-chain blur,
4. additive or energy-limited composite back into the panel color target.

Rules:

1. bloom is off by default,
2. bloom must be explicitly documented as a presentation effect unless the data mapping declares an
   emissive/intensity channel,
3. bloom should not affect picking, probing, colorbar mapping, or readback identity,
4. bloom export behavior must be controlled by the same panel technique state as interactive
   rendering,
5. exact image and scalar-field inspection workflows should be able to disable bloom.

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

1. edge enhancement should see the shaded scene after ambient occlusion,
2. bloom should not blur selection outlines by default,
3. outlines should remain crisp and visible above bloom,
4. external UI remains outside the scene FramePlan and should not be affected by scene effects.

This ordering may be adjusted for specific export modes, but deviations should be explicit.


## FramePlan And Capability Requirements

The proposal depends on these scene planning capabilities:

1. graph resources for color, depth, normal, mask, object-id, bright-pass, and blur intermediates;
2. graph passes that read sampled textures and write panel-local color attachments;
3. deterministic panel viewport/scissor metadata on all fullscreen passes;
4. visual pass capability flags for normal/depth and object-id participation;
5. stable interaction/selection identity mapping for outline targets;
6. export paths that can include or exclude panel effects reproducibly.

If current DRP2 commands cannot express a needed graph pass, the missing capability should be added
to the DRP2 spec rather than bypassing the scene -> FramePlan -> DRP2 route.


## Validation Expectations

Each implemented effect should have:

1. FramePlan graph tests proving default-off behavior and opt-in pass/resource creation,
2. DRP2 stream or runtime smoke coverage for graph lowering,
3. offscreen image-difference coverage on a deterministic scene,
4. panel-boundary tests for multi-panel figures,
5. export coverage when image export includes panel effects,
6. interaction coverage for hover/selection outlines.


## Promotion Target

Once the API and ordering stabilize, promote durable rules into a specialized scene semantics file,
for example `spec/scene/semantics/EFFECTS.md`. Keep implementation sequencing in
`agents/soon/effects/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md`.
