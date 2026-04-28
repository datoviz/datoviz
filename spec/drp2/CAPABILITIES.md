# DRP2 Capabilities

This document defines the minimum capability model needed by DRP2.


## Goals

1. Keep the public contract backend-agnostic.
2. Make optional features explicit and testable.
3. Avoid embedding Vulkan-specific or browser-specific constants in the public DRP2 surface.


## Capability Categories

The runtime should expose a structured capability record covering:

1. protocol version support
2. shader language support
3. texture format support
4. sample count support
5. depth/stencil support
6. limits
7. determinism-related guarantees
8. optional native escape hatches outside the core DRP2 contract


## Minimum Fields

The first capability schema should include at least:

1. `max_bind_groups`
2. `max_color_attachments`
3. `max_buffer_size`
4. `max_uniform_buffer_binding_size`
5. `max_storage_buffer_binding_size`
6. `max_texture_dimension_1d`
7. `max_texture_dimension_2d`
8. `max_texture_dimension_3d`
9. `max_texture_array_layers`
10. `supported_texture_formats`
11. `supported_sample_counts`
12. `supports_timestamp_queries`
13. `supports_fp64`
14. `supported_shader_formats`


## Fixture Capability Shape

When a fixture declares `capabilities`, the first active `2.0` capability object should use this
shape:

1. `max_buffer_size`
2. `max_texture_dimension_1d`
3. `max_texture_dimension_2d`
4. `max_texture_dimension_3d`
5. `supported_texture_formats`
6. `supported_sample_counts`
7. `supported_shader_formats`
8. `supports_fp64`

The omitted fields from the broader future capability model remain planned but are not yet consumed
by the first executable fixture corpus.

The fixture capability object is intentionally smaller than the runtime capability snapshot consumed
by the future scene layer. Fixtures should declare only the fields needed to make the fixture's
validation outcome deterministic.


## Runtime Capability Snapshot

A runtime that is consumed by the scene layer should expose a richer `DvzCapabilitySnapshot`.

This runtime snapshot includes the fixture fields above plus scene-planning limits such as:

1. `max_bind_groups`
2. `max_color_attachments`
3. `max_uniform_buffer_binding_size`
4. `max_storage_buffer_binding_size`
5. `max_texture_array_layers`
6. `supports_readback`
7. `supports_offscreen_targets`
8. `supports_storage_textures`
9. `supports_weighted_blended_oit`
10. `min_uniform_buffer_offset_alignment`
11. `min_storage_buffer_offset_alignment`
12. `min_texture_copy_bytes_per_row_alignment`

The three alignment fields are backend-agnostic numeric limits. They are required for scene/runtime
planning, but individual JSON fixtures do not need to declare them unless the fixture specifically
tests dynamic buffer offsets or texture-copy row-pitch alignment.


## Active `2.0` Command Gates

The first active `2.0` capability validation rules are:

1. `CreateBuffer.size` must not exceed `max_buffer_size`,
2. `CreateTexture.format` must be listed in `supported_texture_formats`,
3. `CreateTexture.sample_count` must be listed in `supported_sample_counts`,
4. `CreateTexture` dimensions must not exceed the corresponding `max_texture_dimension_*` field for
   the chosen texture dimension,
5. `CreateShaderModule.format` must be listed in `supported_shader_formats` when that field is
   present,
6. a shader module declaring `required_features = ["fp64"]` requires `supports_fp64 = true`,
7. compute is mandatory in active DRP2 `2.0` and therefore is not gated by a capability field.

Capability failures should occur during `capability_validation` after schema and semantic validation
have succeeded.


## Feature Gating Rules

1. If a command needs a capability, that dependency must be explicit in the contract.
2. Unsupported features fail during capability validation, before backend submission.
3. Capability reports are declarative; they must not expose backend handle types.
4. WGSL should be the default shader language for the contract.
5. Native-only ingestion paths such as SPIR-V belong behind explicit capability flags.


## Error Mapping

The first capability-validation mapping should be:

1. use `DRP2_ERR_UNSUPPORTED_CAPABILITY` when the runtime explicitly reports lack of support for a
   requested capability,
2. use `DRP2_ERR_FEATURE_REQUIRED` when the command stream requires a feature the runtime reports as
   unavailable and the fixture is explicitly testing that required feature boundary,
3. for the current active fixture corpus, prefer `DRP2_ERR_UNSUPPORTED_CAPABILITY`.


## FP64

FP64 should be treated as optional-but-reportable at the contract level.

Rules:

1. The capability report must state whether FP64 is supported.
2. Pipelines or shader modules that require FP64 must declare that requirement.
3. Producers must be able to select a fallback path when FP64 is unavailable.


## Determinism

The capability model should distinguish:

1. best-effort execution,
2. deterministic rendering and readback where guaranteed,
3. deterministic compute/reduction modes where explicitly supported.

Determinism should be requested by contract-level policy, not inferred from a backend.
