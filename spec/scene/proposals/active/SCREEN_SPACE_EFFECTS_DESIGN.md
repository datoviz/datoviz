> **Execution Status**
> - **Status:** `MOSTLY PROMOTED`
> - **Updated on:** `2026-05-19`
> - **Purpose:** preserve screen-space effects design rationale and remaining open choices after
>   durable semantics were promoted.

# Screen-Space Effects Design

Durable user-facing semantics now live in
[`../../semantics/EFFECTS.md`](../../semantics/EFFECTS.md). Graph-backed implementation rules live
in [`../../implementation/GRAPH_TECHNIQUES.md`](../../implementation/GRAPH_TECHNIQUES.md). The
execution pickup order remains in
[`../../../../agents/soon/effects/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md`](../../../../agents/soon/effects/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md).

If this note disagrees with the specialized specs, update the specialized specs first and keep only
concise backlog context here.

## Design Rationale

Screen-space effects should support perceptual and interaction-driven enhancement for scientific
visualization without making those enhancements part of base visual semantics.

The three active proposal targets remain:

1. outline rendering;
2. screen-space edge enhancement;
3. glow/bloom.

They should stay aligned with the existing scene technique architecture rather than adding
visual-family-specific render paths. The active stack already has MSAA, EDL, SSAO, G-buffer
resources, and a FramePlan graph, so these effects should be expressed as panel-local techniques
over graph resources.

## Relationship To Existing Techniques

Existing techniques affect priority and scope:

1. MSAA covers geometric raster aliasing, so FXAA is not part of this proposal.
2. SSAO covers local contact and cavity depth perception.
3. EDL provides point-cloud and depth-discontinuity emphasis.
4. G-buffer resources are the preferred basis for normal/depth-driven effects.

Screen-space edge enhancement is therefore not a replacement for SSAO. It is an optional boundary
cue for silhouettes, depth jumps, normal breaks, and object/region boundaries. Outline rendering is
primarily an interaction and selection cue. Bloom is a presentation/intensity cue and should remain
opt-in.

## Public API Sketch

The first public API should be panel-level and typed. Exact names may change during API review.

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

The semantic rule should not change: these are retained panel technique settings, not per-frame
immediate commands.

## Remaining Open Choices

1. Whether outline `target_mask` is a bitset of hover, selection, explicit visual flags, and
   annotations.
2. Whether outlines use object IDs, selection masks, or both in the first implementation.
3. How face-level mesh selection maps to outline masks.
4. How outline precedence composes hover with persistent selection.
5. Whether the first bloom implementation assumes LDR thresholding or introduces an HDR
   intermediate.
6. Whether visuals can declare emissive channels before the broader material API exists.
7. Whether bloom participates in screenshot export by default or requires an export flag.
