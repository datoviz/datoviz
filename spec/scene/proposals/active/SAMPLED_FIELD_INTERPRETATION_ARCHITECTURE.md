# Sampled Field Interpretation Architecture

> **Execution Status**
> - **Status:** `ACTIVE PROPOSAL`
> - **Updated on:** `2026-05-27`
> - **Purpose:** define the long-term architecture for interpreting sampled fields across images,
>   labels, volumes, colorizers, and GPU queries.


## Goal

Sampled visuals should not branch directly on raw texture formats throughout image, label, volume,
and query code. A field's storage format, semantic meaning, color mapping, and query payload should
be resolved once into a shared interpretation profile, then consumed by visual-specific techniques.

The public `volume` visual family remains one family. Scalar, RGBA, and label volumes are internal
technique/profile variants selected from `DvzSampledField` format and semantic metadata.


## Core Model

The intended data flow is:

```text
DvzSampledField
    raw storage: dimension, size, format, data, upload state

DvzSampleProfile
    interpreted value kind, sampler kind, filtering rules, query support

DvzColorizer
    transfer function, categorical palette, direct RGBA, or mask policy

DvzVisualTechnique
    image quad, labels quad, volume slice, volume ray march, mesh, point, and so on

DvzQuerySchema
    semantic query fields lowered to concrete backend readback profiles
```

The central rule is:

```text
field format + field semantic + dimension
    -> sample profile
    -> colorizer
    -> visual technique
    -> query payload
```

Visual-family code may still own geometry, traversal, clipping, depth, and compositing policy. It
should not own the global mapping from field formats to sampled-value semantics.


## Locked Decisions

1. Keep one public `volume` visual family.
2. Select scalar, RGBA, unsigned-label, and signed-label volume behavior through internal profiles.
3. Keep `DvzSampleProfile` internal until the design is proven by image, labels, and volume users.
4. Use one shared colorizer abstraction for sampled visuals first, then reuse it for attribute
   visuals later where practical.
5. Lower `DvzScale` to a runtime `DvzColorizer`: continuous transfer, categorical palette, direct
   RGBA passthrough, or mask policy.
6. Support signed and unsigned label data. Initial label-volume support should cover `R8_UINT`,
   `R16_UINT`, `R32_UINT`, `R8_SINT`, `R16_SINT`, and `R32_SINT`.
7. Label fields use nearest sampling only. Linear filtering must be rejected for integer labels.
8. Query results should expose semantic data by default. Label queries return label ids; the CPU may
   map ids to names, colors, and metadata through the categorical scale after a valid GPU result.
9. Do not make `rgba32uint` the core query abstraction. It may be implemented later as one backend
   readback profile if a query schema truly needs four exact 32-bit words.
10. Unsupported format, semantic, render-mode, and query combinations fail through profile
    resolution or explicit unsupported query status. They must not silently fall back to CPU retained
    data sampling.


## Sample Profile Vocabulary

The exact names may change during implementation, but the architecture needs these concepts:

```c
typedef enum DvzSampleValueKind
{
    DVZ_SAMPLE_VALUE_NONE,
    DVZ_SAMPLE_VALUE_SCALAR_F32,
    DVZ_SAMPLE_VALUE_LABEL_U32,
    DVZ_SAMPLE_VALUE_LABEL_S32,
    DVZ_SAMPLE_VALUE_RGBA_F32,
    DVZ_SAMPLE_VALUE_RGBA_U8,
    DVZ_SAMPLE_VALUE_MASK_U8,
} DvzSampleValueKind;

typedef enum DvzSampleSamplerKind
{
    DVZ_SAMPLE_SAMPLER_FLOAT,
    DVZ_SAMPLE_SAMPLER_UINT,
    DVZ_SAMPLE_SAMPLER_SINT,
} DvzSampleSamplerKind;

typedef enum DvzColorizerKind
{
    DVZ_COLORIZER_NONE,
    DVZ_COLORIZER_DIRECT_RGBA,
    DVZ_COLORIZER_COLORMAP_1D,
    DVZ_COLORIZER_TRANSFER_1D,
    DVZ_COLORIZER_CATEGORICAL,
    DVZ_COLORIZER_MASK,
} DvzColorizerKind;
```

A resolved profile should carry at least:

```c
typedef struct DvzSampleProfile
{
    DvzFieldFormat format;
    DvzFieldSemantic semantic;
    uint32_t dimension;

    DvzSampleValueKind value_kind;
    DvzSampleSamplerKind sampler_kind;
    DvzColorizerKind colorizer_kind;

    bool filter_linear_allowed;
    bool query_raw_supported;
    bool query_position_supported;
} DvzSampleProfile;
```

Example mappings:

```text
R32_FLOAT + SCALAR + 2D -> scalar_f32 + float sampler + colormap
R32_FLOAT + SCALAR + 3D -> scalar_f32 + float sampler + transfer function
RGBA8_UNORM + COLOR + 2D -> rgba_u8 + float sampler + direct RGBA
RGBA8_UNORM + COLOR + 3D -> rgba_u8 + float sampler + direct RGBA
R16_UINT + LABEL + 2D -> label_u32 + uint sampler + categorical palette
R16_UINT + LABEL + 3D -> label_u32 + uint sampler + categorical palette
R16_SINT + LABEL + 2D -> label_s32 + sint sampler + categorical palette
R16_SINT + LABEL + 3D -> label_s32 + sint sampler + categorical palette
```


## Colorizer Runtime

`DvzScale` remains the public object for domains, colormaps, transfer state, categorical entries,
legends, and colorbars. Rendering should consume a lowered runtime colorizer.

Categorical colorizers need a GPU-facing lookup resource:

1. dense palette texture indexed by label id, implemented first for simplicity and speed;
2. sparse lookup table or buffer later for sparse atlas ids;
3. fallback color and fallback label text for unknown ids;
4. optional per-category alpha so label volumes can use category opacity in ray marching.

The categorical lookup representation must be hidden behind the colorizer contract so label images,
label volumes, and later label attributes can share it.


## Volume Label Semantics

Label volumes are `volume` visuals whose bound 3D sampled field resolves to a signed or unsigned
label sample profile.

Initial behavior:

1. support slice rendering before full ray marching;
2. sample labels with nearest filtering only;
3. map label ids through the categorical colorizer;
4. treat label `0` as the default background id unless the visual overrides it;
5. return explicit unsupported for scalar-only modes that do not yet have label policy.

Full 3D label rendering should use a named policy, not MIP terminology. The first supported policy
should be first non-background hit along the ray. Later policies may add categorical alpha
compositing, selected-label-only rendering, boundary emphasis, and label-mask rendering.


## Query Semantics

Queries should be requested and decoded as semantic payloads, then lowered to concrete backend
readback formats.

Useful semantic fields include:

```text
visual id
item id
sample value
label id
UVW
voxel coordinate
display RGBA
```

Baseline lowerings:

```text
label id only -> r32uint
signed label id only -> r32uint with explicit signed decode policy
label id + packed UVW -> rg32uint
scalar quantized + packed UVW -> rg32uint
display RGBA8 -> r32uint packed RGBA
four exact uint words -> optional rgba32uint backend profile
```

The displayed RGBA should not be read back by default for label queries. A label id is the stable
semantic value; color, text, ontology, and selection metadata are resolved on the CPU from the
categorical scale after the GPU query result is valid.


## Shader Strategy

Use table-driven shader variant selection before introducing a full shader generator. The C profile
resolver should select a small number of variants by value kind, sampler kind, colorizer kind, and
visual technique.

Shader code should factor these concepts where the current build system allows it:

```glsl
Sample sample_at(vec3 uvw);
vec4 colorize_sample(Sample sample);
vec4 composite_step(vec4 accum, vec4 color);
QueryPayload query_sample(Sample sample, vec3 uvw);
```

Shared volume traversal, bounds, clipping, and ray setup should stay shared. Only decode,
colorization, and policy snippets should vary.


## Implementation Order

1. Add the internal sample-profile resolver and resolver tests.
2. Add the colorizer runtime abstraction and categorical palette lowering.
3. Route existing 2D labels through sample profiles and categorical colorizers without changing
   behavior.
4. Route scalar and RGBA volume setup through sample profiles without changing behavior.
5. Introduce semantic query schemas and lower them to existing integer query profiles.
6. Add unsigned and signed label-volume slice rendering.
7. Add unsigned and signed label-volume slice queries.
8. Add first non-background hit label-volume ray marching.
9. Reuse colorizers for attribute-driven visuals where it reduces duplication.
10. Remove old visual-local format branches after equivalent profile-based paths are covered.


## Validation Requirements

Every implementation slice should include focused tests. Required coverage includes:

1. sample-profile resolver matrix tests;
2. categorical colorizer packing and dirty-update tests;
3. existing 2D labels rendering and query behavior;
4. existing scalar volume rendering and query behavior;
5. existing RGBA volume rendering behavior;
6. unsigned and signed label-volume slice rendering;
7. unsigned and signed label-volume id queries;
8. unsupported profile, filtering, render-mode, and query combinations.

Use these commands as the default validation gate:

```text
just build
just test scene
direnv exec . just test query
direnv exec . just test drp2
just spec-check
git diff --check
```
