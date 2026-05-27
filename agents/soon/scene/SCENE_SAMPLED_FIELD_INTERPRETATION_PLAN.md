# Scene Sampled Field Interpretation Plan

> **Execution Status**
> - **Status:** `ACTIVE / ARCHITECTURE REFACTOR PLAN`
> - **Updated on:** `2026-05-27`
> - **Purpose:** give future agents an implementation sequence for the sampled-field interpretation,
>   colorizer, label-volume, and query-schema refactor.

Start here when asked to refactor sampled image, labels, volume, colorizer, or sampled-field query
behavior.

Durable design intent lives in
[`../../../spec/scene/proposals/active/SAMPLED_FIELD_INTERPRETATION_ARCHITECTURE.md`](../../../spec/scene/proposals/active/SAMPLED_FIELD_INTERPRETATION_ARCHITECTURE.md).
Read it before changing public field semantics, volume label behavior, colorizer resource lowering,
or query payload design.


## One-Sentence Goal

Replace scattered visual-local format branching with one internal sampled-field interpretation
layer that resolves storage format, semantic meaning, colorizer, visual technique, and GPU query
payload consistently across images, labels, volumes, and later attribute-driven visuals.


## Read First

1. [`../../../spec/scene/proposals/active/SAMPLED_FIELD_INTERPRETATION_ARCHITECTURE.md`](../../../spec/scene/proposals/active/SAMPLED_FIELD_INTERPRETATION_ARCHITECTURE.md)
2. [`../../../spec/scene/proposals/promoted/SAMPLED_FIELD_API_DESIGN.md`](../../../spec/scene/proposals/promoted/SAMPLED_FIELD_API_DESIGN.md)
3. [`../../../spec/scene/visuals/IMAGE.md`](../../../spec/scene/visuals/IMAGE.md)
4. [`../../../spec/scene/visuals/LABELS.md`](../../../spec/scene/visuals/LABELS.md)
5. [`../../../spec/scene/visuals/VOLUME.md`](../../../spec/scene/visuals/VOLUME.md)
6. [`SCENE_GPU_QUERY_OVERHAUL.md`](SCENE_GPU_QUERY_OVERHAUL.md)
7. [`SCENE_VOLUME_RENDERING_FOLLOWUP.md`](SCENE_VOLUME_RENDERING_FOLLOWUP.md)


## Locked Decisions

1. Keep one public `volume` visual family. Do not add a public `label_volume` family for the first
   implementation.
2. Select scalar, RGBA, unsigned-label, and signed-label volume behavior through internal sample
   profiles.
3. Support signed labels as first-class label data, alongside unsigned labels.
4. Keep `DvzSampleProfile` internal until image, labels, volume, and query users have validated the
   design.
5. Use a shared colorizer runtime lowered from `DvzScale`.
6. Implement dense categorical GPU palettes first, with an internal abstraction that can later use
   sparse atlas lookup.
7. Return semantic query values by default. Label queries return label ids; CPU-side scale metadata
   resolves names, colors, and domain-specific labels after a valid GPU result.
8. Do not center the architecture on `rgba32uint`. It can be added later as one backend lowering for
   wide exact query payloads.
9. Reject unsupported combinations explicitly through profile resolution or query unsupported status.
10. Never revive CPU retained-data sampling as a fallback for rendered visual queries.


## Current Code Hotspots

1. `include/datoviz/scene/field.h`: field formats, field semantics, and sampled-field descriptors.
2. `include/datoviz/scene/scale.h`: scale, colormap, categorical entries, legends, and colorbars.
3. `src/scene/scale.c`: current scale state and categorical metadata.
4. `src/scene/visual_shader_desc.c`: visual shader key selection for image, labels, and volume.
5. `src/scene/glsl/labels_uint.frag` and `src/scene/glsl/labels_sint.frag`: existing 2D label
   categorical shader paths.
6. `src/scene/glsl/volume_slice.frag`, `src/scene/glsl/volume_mip.frag`, and
   `src/scene/glsl/volume_composite.frag`: current scalar/RGBA volume paths.
7. `src/scene/visuals/volume/query.c`: current scalar slice volume query policy and integer-format
   rejection.
8. `src/scene/query/execute.c`: query profile selection and generic execution orchestration.
9. `include/datoviz/scene/types.h`: query result, query profile, status, and value-kind surface.


## Phase 1 - Sample Profile Resolver

Add an internal resolver, for example under:

```text
src/scene/sample_profile.c
src/scene/sample_profile.h
```

Keep public API churn minimal at first. The resolver should accept field format, field semantic, and
dimension, then return a profile containing value kind, sampler kind, colorizer kind, filtering
constraints, and query support flags.

Required mappings:

1. scalar 2D image fields;
2. scalar 3D volume fields;
3. direct RGBA 2D and 3D fields;
4. unsigned integer 2D labels;
5. signed integer 2D labels;
6. unsigned integer 3D label volumes;
7. signed integer 3D label volumes.

Commit target:

```text
scene: add sampled field profile resolver
```

Validation:

```text
just build
just test test_scene_sample_profile
git diff --check
```


## Phase 2 - Colorizer Runtime

Introduce an internal colorizer runtime lowered from `DvzScale`. It should initially support:

1. direct RGBA passthrough;
2. scalar colormap or transfer texture;
3. categorical label palette;
4. fallback color for unknown labels;
5. dirty invalidation when scale categories or transfer data change.

Use a dense categorical GPU palette first, but hide that representation behind the colorizer so a
sparse table can be added later without changing visual-family code.

Commit target:

```text
scene: add sampled visual colorizers
```

Validation:

```text
just build
just test scene
git diff --check
```


## Phase 3 - Port Existing 2D Labels

Route existing label visuals through the sample-profile resolver and categorical colorizer without
changing user-visible behavior.

Keep the public `label` visual family. The point of this phase is to prove the shared profile and
colorizer model against already working signed and unsigned 2D label paths.

Commit target:

```text
scene: route labels through sample profiles
```

Validation:

```text
just build
direnv exec . just test query
just test test_scene_labels_visual_binds_categorical_scale
git diff --check
```


## Phase 4 - Port Existing Scalar And RGBA Volumes

Route existing scalar and RGBA volume setup through sample profiles. Do not add label-volume
rendering yet.

The goal is to remove duplicated volume-local format logic before adding new formats. Existing slice,
MIP, composite, transfer, opacity, and query behavior should remain unchanged.

Commit target:

```text
scene: route volumes through sample profiles
```

Validation:

```text
just build
just test test_scene_volume
direnv exec . just test query
git diff --check
```


## Phase 5 - Semantic Query Schemas

Introduce an internal query schema/lowering step. Query callers and visual-family code should speak
in semantic fields such as label id, scalar value, UVW, voxel coordinate, visual id, and displayed
RGBA. Backend execution can then lower the schema to `r32uint`, `rg32uint`, two `r32uint`
attachments, or a future `rgba32uint` payload.

Do not make `rgba32uint` mandatory. It is only a backend profile for future exact four-word
payloads.

Commit target:

```text
scene: add semantic query schemas
```

Validation:

```text
just build
direnv exec . just test query
direnv exec . just test drp2
git diff --check
```


## Phase 6 - Label Volume Slice Rendering

Add label-volume slice rendering for both unsigned and signed integer 3D fields:

1. `R8_UINT`, `R16_UINT`, `R32_UINT`;
2. `R8_SINT`, `R16_SINT`, `R32_SINT`;
3. nearest sampling only;
4. categorical colorizer lookup;
5. background id defaults to `0`;
6. explicit unsupported status for render modes without label policy.

Keep the public family as `volume`.

Commit target:

```text
scene: render signed and unsigned label volume slices
```

Validation:

```text
just build
just test test_scene_volume_label
direnv exec . just test scene
git diff --check
```


## Phase 7 - Label Volume Queries

Add GPU-backed label-volume slice queries for both signed and unsigned labels.

Baseline payloads:

1. label id only -> `r32uint`;
2. label id plus packed UVW -> `rg32uint`;
3. signed label ids decode through an explicit signed policy from the `r32uint` word.

After readback, CPU-side scale metadata may resolve the label id to display name, color, opacity,
and application metadata. Do not sample retained CPU field data as a fallback.

Commit target:

```text
scene: query signed and unsigned label volumes
```

Validation:

```text
just build
direnv exec . just test query
just test test_scene_volume_label_query
git diff --check
```


## Phase 8 - Label Volume Ray Marching

Add full 3D label-volume rendering after slice rendering and slice queries are stable.

First policy:

```text
first non-background hit
```

Parameters:

1. background id;
2. step count;
3. global opacity scale;
4. per-category alpha from the categorical colorizer;
5. early termination threshold.

Do not describe this as MIP. Label MIP is not a default semantic policy.

Commit target:

```text
scene: raymarch categorical label volumes
```

Validation:

```text
just build
direnv exec . just test scene
git diff --check
```


## Phase 9 - Attribute Colorizer Reuse

Only after sampled visuals are stable, consider using the same colorizer runtime for point, mesh,
path, and other attribute-driven visuals:

1. scalar attribute -> continuous colormap;
2. label attribute -> categorical palette;
3. direct RGBA attribute -> passthrough.

This phase should be separate from label-volume work and should only proceed where it removes real
duplication.


## Phase 10 - Delete Legacy Branches

After equivalent profile-driven paths are tested, remove visual-local format branches and duplicated
palette/transfer handling.

Explicit cleanup targets:

1. image and labels format dispatch;
2. volume scalar/RGBA branches that duplicate profile data;
3. query profile decisions based on visual-family ad hoc format checks;
4. stale docs that describe categorical label volumes as future-only.


## Broad Validation Gate

Before closing the architecture slice, run:

```text
just build
just spec-check
direnv exec . just test query
direnv exec . just test drp2
direnv exec . just test scene
git diff --check
```

Before committing any implementation slice, verify `git status --short` and do not stage `data`,
`libs/vulkan/`, generated runtime libraries, or binary payloads without explicit user approval.
