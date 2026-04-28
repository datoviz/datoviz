# Scene/DRP2 Spec Cleanup Decision Log

> **Status:** `AWAITING OWNER DECISIONS`
> **Created on:** `2026-04-28`
> **Purpose:** collect the owner decisions needed before a full consistency pass over
> `spec/drp2/`, `spec/scene/`, related examples, fixtures, and draft headers.

This document is intentionally not normative spec text yet.

Use it as an inline decision log:

1. read each decision,
2. either accept the recommendation or write an alternative under `Owner decision`,
3. leave notes or follow-up questions directly under the relevant item.

Unless explicitly overridden, the recommendation under each item is the default I would implement in
the cleanup pass.


## Cleanup Goal

The cleanup pass should leave the project with:

1. one coherent DRP2 `2.0` active surface,
2. one coherent scene taxonomy and runtime boundary,
3. no stale examples contradicting the normative documents,
4. enough detail to start the scene-to-DRP2 converter,
5. enough detail to start a DRP2 Vulkan runtime on top of vklite.


## Proposed Pass Order

Recommendation:

1. resolve decisions in this file,
2. update DRP2 status/prose/schema/fixtures for active surface consistency,
3. update scene taxonomy and runtime-boundary documents,
4. update visual-family specs, examples, and draft headers,
5. add or update lightweight spec-check fixtures/tests where they catch the resolved behavior.

Owner decision:

TODO(user): ok


## Decision D001: DRP2 Active Surface

Question:

Should shader modules, samplers, and texture views be active in DRP2 `2.0`, or remain deferred?

Recommendation:

Make them active in DRP2 `2.0`.

Reasoning:

1. current `COMMANDS.md`, schemas, and fixtures already use shader modules,
2. real scene visuals need samplers and texture views for image, mesh texture, sphere texture,
   glyph atlas, and volume paths,
3. deferring them would force hidden runtime-side objects that the scene cannot validate cleanly,
4. the runtime can still synthesize Vulkan implementation details underneath these logical objects.

Spec edits if accepted:

1. update `agents/now/DRP2_SPEC.md` to remove stale deferred entries,
2. update `spec/drp2/LAYER1.md` active object-kind list,
3. update `spec/drp2/README.md`, `LIFETIMES.md`, and `schema/DEFERRED.md` if any stale wording
   remains.

Owner decision:

TODO(user): agree


## Decision D002: DRP2 Shader Language Requirement

Question:

Should WGSL remain mandatory for DRP2 `2.0` execution conformance even though the first native
Vulkan runtime will execute SPIR-V through vklite?

Recommendation:

Keep WGSL mandatory at the DRP2 contract level, and make the native runtime responsible for a
WGSL-to-SPIR-V compilation path.

Reasoning:

1. WGSL keeps the protocol aligned with browser/WebGPU parity,
2. it avoids making SPIR-V the portable authoring format,
3. the converter and fixture corpus can stay backend-agnostic,
4. vklite can remain SPIR-V-facing internally while the DRP2 runtime owns shader ingestion.

Implementation implication:

The runtime needs an explicit shader-compiler service boundary. It may use shaderc/tint/naga or a
repo-approved dependency, but this should not leak into scene semantics.

Owner decision:

TODO(user): agree


## Decision D003: Shader Entry Points

Question:

Should DRP2 support arbitrary shader entry-point names, or require `main`?

Recommendation:

Support arbitrary `entry_point` names as the spec already says, but allow the first built-in scene
shader library to use `main` consistently.

Reasoning:

1. `CreateShaderModule.entry_point` is already in the active command contract,
2. arbitrary entry points are common in WGSL and useful for future custom visuals,
3. vklite currently hardcodes `"main"` and should be extended at the DRP2 runtime layer rather
   than narrowing the protocol.

Owner decision:

TODO(user): agree


## Decision D004: Bind-Group Layout Sampler Entries

Question:

Should `CreateBindGroupLayout.entries[].binding_type` include `sampler`?

Recommendation:

Yes. Add `sampler` to bind-group layout entries and align prose, schema, and fixture validation.

Reasoning:

1. `CreateBindGroup` already allows `binding_type = sampler`,
2. texture sampling requires a sampler binding in Vulkan/WebGPU-style layouts,
3. omitting it makes sampler support internally inconsistent.

Owner decision:

TODO(user): agree


## Decision D005: Bind-Group Shader Stage Visibility

Question:

Should DRP2 bind-group layout entries include explicit shader-stage visibility?

Recommendation:

Yes, add a minimal `visibility` field with shader-stage flags, defaulting to all stages only if
omitted for backward fixture compatibility.

Reasoning:

1. Vulkan descriptor set layouts require stage flags,
2. WebGPU bind-group layouts require visibility,
3. explicit visibility improves validation without exposing backend-specific handles,
4. omitting it forces runtime reflection or overly broad visibility for every binding.

Owner decision:

TODO(user): agree


## Decision D006: Dynamic Offset Alignment

Question:

Should DRP2 `2.0` expose dynamic uniform/storage buffer offset alignment limits?

Recommendation:

Do not expose them in the core fixture corpus yet, but let the runtime capability snapshot include
optional alignment fields before scene implementation starts.

Reasoning:

1. current DRP2 intentionally avoids backend-shaped alignment fields,
2. real Vulkan execution cannot ignore alignment for dynamic offsets,
3. scene planning needs this information if it packs many parameter blocks into shared buffers,
4. making the fields optional lets existing fixtures remain narrow.

Suggested fields:

1. `min_uniform_buffer_offset_alignment`,
2. `min_storage_buffer_offset_alignment`,
3. `min_texture_copy_bytes_per_row_alignment`.

Owner decision:

TODO(user): ok but i don't understand what "Do not expose them in the core fixture corpus yet" means


## Decision D007: Render-Pass Attachments And Texture Views

Question:

Should render-pass attachments keep referencing textures directly, or switch to texture views?

Recommendation:

Keep DRP2 `2.0` render-pass attachments referencing textures directly, but document the exact
implicit full-subresource view rule and restrictions.

Reasoning:

1. this keeps the first render-pass path simple,
2. texture views are still needed for bind-group sampling,
3. attachment views can be promoted later if mip/layer attachment use becomes necessary,
4. the runtime can synthesize Vulkan image views internally for full-subresource attachments.

Required clarification:

For active `2.0`, render attachments use mip `0`, layer `0`, full extent, and the texture's declared
format unless a later command extends the attachment descriptor.

Owner decision:

TODO(user): agree


## Decision D008: QueueSubmit Readback Model

Question:

Is `QueueSubmit.readbacks` plus `QueueSubmitReply` the official DRP2 `2.0` readback primitive?

Recommendation:

Yes. Treat it as official and remove stale notes saying DRP2 has no readback primitive.

Reasoning:

1. current schemas and fixtures already encode this model,
2. it is sufficient for picking and simple offscreen readback,
3. it avoids adding a separate `ReadBuffer` command in `2.0`,
4. it maps cleanly to a synchronous scene helper for click/query picking.

Owner decision:

TODO(user): agree


## Decision D009: Submitted-Work Lifetime

Question:

Should active DRP2 `2.0` keep the conservative rule that resources referenced by submitted work
cannot be destroyed for the rest of the stream?

Recommendation:

Yes for `2.0`. Defer fences/completion-based destruction to `2.1`.

Reasoning:

1. the first runtime can be correct without protocol-visible fences,
2. scene/runtime cache eviction can happen at stream teardown or internally after GPU completion
   without changing protocol-visible state,
3. adding fences now would broaden the spec and fixture corpus significantly.

Owner decision:

TODO(user): agree


## Decision D010: DRP2 Capabilities Needed Before Runtime

Question:

Should the DRP2 capability shape be expanded before the Vulkan runtime starts?

Recommendation:

Add a runtime-facing capability record now, but keep the current fixture capability shape minimal
unless a fixture actually exercises a field.

Reasoning:

The scene needs more than the fixture runner currently consumes: readback support, offscreen target
support, storage texture support, max bind groups, max color attachments, binding-size limits,
alignment limits, and shader format support. Those can exist in runtime capability docs without
forcing every fixture to declare them.

Owner decision:

TODO(user): agree


## Decision D011: Scene `Figure`, `Canvas`, And `RenderTarget`

Question:

What should be the scene-facing top-level output object: `DvzFigure`, `DvzRenderTarget`, or both?

Recommendation:

Use both, with strict meanings:

1. `DvzFigure` is a pure scene/layout object containing panels, margins, axes, and export layout;
2. `DvzRenderTarget` is the runtime-resolved output destination;
3. `DvzCanvas` remains app/runtime-owned and is never held by the scene;
4. a figure references one or more logical render targets, not a canvas.

Reasoning:

1. scientific users need a figure-level layout concept,
2. the scene should still avoid canvas/window/swapchain ownership,
3. this reconciles the useful draft header shape with `RUNTIME_BOUNDARY.md`.

Spec edits if accepted:

1. rename draft comments that say `DvzFigure` is "canvas / window",
2. add `DvzRenderTarget` to the draft header,
3. clarify `dvz_figure(...)` construction and render-target binding,
4. update examples that imply the scene owns a canvas.

Owner decision:

TODO(user): agree


## Decision D012: Runtime Submission Input

Question:

Should `dvz_runtime_submit()` consume a `FramePlan` or a DRP2 command stream?

Recommendation:

Make the architectural boundary explicit as two layers:

1. scene-to-DRP2 converter consumes `FramePlan` and emits a DRP2 command stream,
2. DRP2 runtime consumes the command stream,
3. a convenience `dvz_runtime_submit_frame_plan()` may combine both for applications.

Reasoning:

1. the user specifically wants both scene-to-DRP2 converter and DRP2 runtime,
2. conflating them makes testing harder,
3. DRP2 replay and fixture execution should not require a scene object,
4. the scene can still have a simple app-facing submission helper.

Owner decision:

TODO(user): agree


## Decision D013: `slice` Taxonomy

Question:

Should volume slicing be an `image` mode or a `volume` render mode?

Recommendation:

Make `slice` a `volume` render mode, not an `image` mode.

Reasoning:

1. slicing depends on 3D sampled-field semantics, crop/bounds, axis order, value range, and probe
   behavior,
2. `image` should remain flat 2D raster/atlas/heatmap semantics,
3. `volume` already has `render_mode = slice`,
4. this avoids confusing sampled 2D images with views into 3D fields.

Spec edits if accepted:

1. update `VISUAL_CONTRACT.md`,
2. replace or rename `examples/IMAGE_SLICE.md`,
3. update `PICKING.md`, `USE_CASES.md`, `RESOURCE_MODEL.md`, and transform notes that mention
   image-family slice-like modes.

Owner decision:

TODO(user): agree


## Decision D014: First Implemented Visual Slice

Question:

Which visual families should be targeted for the first real scene-to-DRP2 converter slice?

Recommendation:

Start with:

1. `primitive`,
2. `pixel`,
3. `point`,
4. `image` with `rgba`, `scalar`, and `none`,
5. `mesh` basic triangle list without advanced isolines,
6. picking for `point` or `pixel`.

Defer first implementation of:

1. `glyph`,
2. `volume`,
3. `sphere`,
4. `boxplot`,
5. `errorbar`,
6. custom visuals,
7. exact transparency paths beyond opaque/simple blended.

Reasoning:

This covers static geometry, dynamic buffers, texture sampling, offscreen/readback, and picking
without starting with font atlases, ray marching, impostors, or statistical composite geometry.

Owner decision:

TODO(user): agree


## Decision D015: Group/Span Public API

Question:

How should users declare grouped visual structure and `PER_GROUP` attributes?

Recommendation:

Use explicit named data attributes, not special enum constants:

1. `span_sizes` declares structural spans for grouped visuals such as `path` and `glyph`,
2. `group_id` declares per-item group identity for flat `ItemTable` visuals,
3. `PER_GROUP` data is accepted when `n == group_count`,
4. for grouped visuals, default group identity is span identity unless an explicit future spec says
   otherwise.

Reasoning:

1. this matches the string-based `dvz_visual_set_data()` model,
2. it avoids introducing `DVZ_ATTR_*` after the API direction rejected attr enums,
3. it handles both contiguous groups and arbitrary group ids.

Spec edits if accepted:

1. remove `dvz_visual_spans()` references unless you want that function,
2. replace `DVZ_ATTR_GROUP_ID` wording with `"group_id"`,
3. fix `PIXEL.md` to not imply all flat `PER_GROUP` cases are contiguous unless `span_sizes` is
   explicitly provided.

Owner decision:

TODO(user): hold on this requires more discussion, there is a distinction between groups and spans right?


## Decision D016: `PER_CHAR` Source For Glyph

Question:

Should glyph-specific `PER_CHAR` remain a formal attribute source?

Recommendation:

Keep it, but define it as a glyph-only extended source in `ATTRIBUTE_SOURCES.md`.

Reasoning:

1. per-character color is useful for text-heavy scientific labels and sequence logos,
2. forcing it into `PER_ITEM` would make string-level and character-level item identity ambiguous,
3. localizing it as a glyph extension keeps the core source model simple.

Owner decision:

TODO(user): i don't understand this, explain clearly


## Decision D017: Missing-Data Semantics

Question:

Should NaN/Inf/missing values be specified before implementation?

Recommendation:

Yes. Add a shared scene rule and allow family overrides.

Recommended defaults:

1. NaN positions: item is skipped and not pickable,
2. NaN scalar color/size: use scale missing-value color or fallback parameter,
3. Inf coordinates: validation warning or skip, depending on strictness mode,
4. texture NaNs: colormap missing color for scalar textures when supported.

Reasoning:

Scientific data routinely contains missing values. Leaving this undefined will produce inconsistent
visuals and hard-to-debug backend behavior.

Owner decision:

TODO(user): agree, the user should be able to specify the default value for NaN/inf for the various parameters. More generally there is the question of the default values for the parameters of each visual, like default color, size etc if none is provided. Is it tackled somewhere?


## Decision D018: Units And Quantity Metadata

Question:

Should axes, scales, probes, and readbacks carry optional unit metadata in the first scene spec?

Recommendation:

Add optional unit/label metadata to domains, scales, and probe/readback payloads, but do not enforce
unit algebra in v0.4.

Reasoning:

1. scientific plots need readable units,
2. axes and colorbars can display them,
3. full dimensional analysis would be too much for the first implementation.

Owner decision:

TODO(user): agree


## Decision D019: Categorical Scales

Question:

Should categorical color/size/opacity mappings be first-class alongside continuous scales?

Recommendation:

Yes for color scales in v0.4; size/opacity categorical scales may use the same scale object if the
implementation cost is small.

Reasoning:

1. group-colored scientific data is common,
2. legends need stable category ordering and labels,
3. encoding categories as arbitrary scalar values loses semantic identity.

Owner decision:

TODO(user): agree


## Decision D020: Additional Sci-Viz Families

Question:

Should bar, histogram, area, violin, vector-field, and streamline visuals be added now?

Recommendation:

Do not add them to the first implementation target, but record them explicitly as deferred visual
families or derived composite helpers.

Suggested classification:

1. `bar`: likely first-class or high-level composite over `boxplot`/`primitive`,
2. `histogram`: high-level data transform plus `bar`,
3. `area`: path/primitive-derived filled polygon family,
4. `violin`: separate family, not `boxplot`,
5. `vector_field`: high-level helper using marker arrows/segments initially,
6. `streamline`: future path-derived family/helper.

Reasoning:

These are user-facing sci-viz features, but adding them now would distract from the renderer
contract. They should be acknowledged so they do not get mistaken for forgotten scope.

Owner decision:

TODO(user): agree


## Decision D021: Custom Visuals

Question:

Should custom visuals remain in the v0.4 scene spec?

Recommendation:

Keep the concept, but move implementation to v0.4+ unless you consider it essential.

Reasoning:

1. custom visuals require stable shader ingestion, binding layout, picking integration, and
   diagnostics,
2. they are valuable but multiply the first runtime surface,
3. built-in families should prove the converter/runtime first.

Owner decision:

TODO(user): but custom visuals would follow the same path as builtin visuals so i don't quite understand how supporting them in v0.4 would make things more complicated?


## Decision D022: Built-In Shader Library Ownership

Question:

Where should built-in visual shader sources and variant metadata live?

Recommendation:

Make a scene-owned shader registry for semantic variants, with runtime-owned compiled pipeline
caches.

Reasoning:

1. scene knows the visual family and variant identity,
2. runtime knows backend compilation and cache lifetime,
3. the converter can emit deterministic `CreateShaderModule` and `CreateRenderPipeline` commands
   based on registry keys.

Owner decision:

TODO(user): agree


## Decision D023: FramePlan Serialization

Question:

Should `FramePlan` have a JSON/debug serialization before implementation?

Recommendation:

Yes. Define a test/debug serialization shape before implementing the converter.

Reasoning:

1. it lets scene planning be tested without a GPU,
2. it makes converter tests deterministic,
3. it creates a clear bridge to DRP2 fixture generation,
4. it avoids debugging scene planning through Vulkan output.

Owner decision:

TODO(user): agree


## Decision D024: DRP2 Command Stream Serialization

Question:

Should the scene-to-DRP2 converter emit the same JSON shape as DRP2 fixtures?

Recommendation:

Yes for tests and diagnostics. The in-memory path can use C structs later, but the JSON fixture
shape should be the canonical inspectable form.

Reasoning:

1. existing fixture runner already validates command streams,
2. scene examples can become converter fixtures,
3. it keeps the runtime and converter independently testable.

Owner decision:

TODO(user): agree


## Decision D025: Picking Payload Encoding

Question:

What minimum picking payload should the first implementation support?

Recommendation:

Start with a 32-bit or 64-bit encoded object id rendered into an integer target, resolved through a
scene-side pick table:

1. panel id is request-side metadata,
2. encoded id maps to visual id and item/group/aux identity,
3. zero means no hit,
4. richer sampled-value probe payloads are separate readback requests.

Reasoning:

1. integer render targets are already in DRP2 fixtures,
2. it avoids packing many semantic fields into shader output,
3. it keeps grouped and batched rendering compatible with picking.

Open detail:

Choose 32-bit for first implementation simplicity, or 64-bit if you expect very large pick tables.

Owner decision:

TODO(user): 64-bit directly, we may easily have 4B+ items


## Decision D026: Synchronous Picking API

Question:

Should the public C API expose `dvz_panel_pick()` as a blocking helper in addition to async request
and polling?

Recommendation:

Yes. Keep both:

1. blocking `dvz_panel_pick()` for click/query/tool inspection,
2. async hover request/poll/callback for high-frequency pointer motion.

Reasoning:

This matches user expectations for tool queries while keeping hover latency manageable.

Owner decision:

TODO(user): agree


## Decision D027: Selection Mask Ownership

Question:

Should selection be represented as a GPU mask buffer in the first implementation?

Recommendation:

Keep scene-owned selection state as authoritative, and make GPU mask buffers derived resources
created only when a visual variant needs them.

Reasoning:

1. CPU selection is easier to inspect, serialize, and synchronize with UI,
2. GPU masks are useful for large visual highlighting,
3. making GPU masks authoritative would complicate cross-panel and cross-visual linking.

Owner decision:

TODO(user): agree


## Decision D028: Transparency Baseline

Question:

Which transparency modes should be in the first implementation target?

Recommendation:

Implement opaque and ordinary alpha blending first. Keep weighted blended OIT as planned v0.4 work
only if the runtime slice reaches multiple color attachments comfortably. Keep exact per-pixel linked
list OIT deferred.

Reasoning:

Transparency quality matters for sci-viz, but it requires substantial render-pass and attachment
policy. It should not block the first scene-to-DRP2 path.

Owner decision:

TODO(user): no, weighted blended OIT is a hard requirement for v0.4, very important


## Decision D029: Volume Baseline

Question:

Should `volume` be part of the first scene-to-DRP2 implementation slice?

Recommendation:

No. Keep the spec, but defer implementation until 2D images, basic mesh, texture sampling,
offscreen rendering, and picking are working.

Reasoning:

Volume requires ray marching, 3D textures, transfer functions, crop/bounds semantics, and special
picking/probe rules. It is a good second wave, not the first wave.

Owner decision:

TODO(user): agree but still for v0.4


## Decision D030: Image Heatmap Isolines

Question:

Should `image.texture_mode = heatmap` with GPU marching-squares isolines be in the first
implementation?

Recommendation:

Defer GPU isolines. Implement `rgba`, `scalar`, and `none` image modes first.

Reasoning:

The isoline path adds compute-generated geometry and cross-node resource dependencies. It is useful,
but not necessary to validate texture upload/sampling and colormap basics.

Owner decision:

TODO(user): agree


## Decision D031: Mesh Isolines And Edge Overlay

Question:

Should mesh edge overlays and isolines be part of the first implementation?

Recommendation:

Implement basic indexed mesh with color/normal/texture modes first. Defer mesh isolines and possibly
edge overlay until the basic mesh path is stable.

Reasoning:

The first mesh target should prove indexed geometry, depth, culling, and optional lighting. Edge and
isoline overlays add extra passes or shader variants.

Owner decision:

TODO(user): ok but mesh edge overlay is pretty important, and already implemented in v0.3 with special trick in the shaders and special baking


## Decision D032: Draft Header Authority

Question:

Should `spec/scene/headers/scene_api.h` remain the authoritative C spelling?

Recommendation:

Yes, but after cleanup it should be internally consistent with the docs and marked as the C API
draft, not just an informative sketch.

Reasoning:

1. one draft C spelling prevents prose from drifting,
2. implementation will need concrete names,
3. it should not contradict runtime-boundary ownership.

Owner decision:

TODO(user): agree


## Decision D033: Nonlinear Transform Naming

Question:

Should the transform API use `projection` terminology or `coord_transform` terminology?

Recommendation:

Use `coord_transform` for data-space conversion and reserve `projection` for camera/render
projection.

Reasoning:

1. `TRANSFORM_PIPELINE.md` correctly separates data-space coordinate transforms from projection,
2. the draft header currently uses `DvzProjectionDesc`, which can be confused with camera
   projection,
3. scientific transforms such as polar and Mercator happen before panel normalization.

Owner decision:

TODO(user): agree


## Decision D034: Texture Format Vocabulary In Scene API

Question:

Should public scene texture constructors accept Vulkan/WebGPU numeric formats or Datoviz enums?

Recommendation:

Use Datoviz-owned format enums in scene-facing APIs, then map to DRP2/WebGPU strings and Vulkan
formats internally.

Reasoning:

1. the scene API should not expose `VkFormat`,
2. DRP2 already uses backend-agnostic format strings,
3. C users need a typed enum rather than strings in hot paths.

Owner decision:

TODO(user): agree but would there be a way to ensure the enum values (which may be hidden to the user) actually match the vulkan formats internally, just to avoid any mismatch/bug? and easier for debugging. as implementation detail. wdyt?


## Decision D035: Resource Creation Surface

Question:

Should users only construct `DvzTexture`/`DvzScale`/`DvzFont`, while buffers/item tables remain
internal?

Recommendation:

Yes for the first public scene API.

Reasoning:

1. visual data should flow through `dvz_visual_set_data`,
2. exposing buffers too early leaks implementation detail,
3. textures, scales, and fonts are genuine user-facing reusable resources.

Owner decision:

TODO(user): agree


## Decision D036: Mutability And Pointer Ownership

Question:

Should `DVZ_MUTABILITY_STATIC` mean the scene borrows the user's pointer, as currently implied?

Recommendation:

No. Make mutability purely a planning hint by default. If zero-copy borrowed data is wanted, expose
it as a separate explicit API or flag with clear lifetime rules.

Reasoning:

1. borrowing user pointers from a C scene API is error-prone,
2. it complicates Python bindings,
3. mutability and ownership are separate concerns,
4. copying by default is safer for the first implementation.

Owner decision:

TODO(user): agree


## Decision D037: Runtime Diagnostics Mapping

Question:

Should scene-facing diagnostics expose raw DRP2 error codes?

Recommendation:

Keep raw DRP2 codes only in diagnostic context/debug detail, not as primary scene API codes.

Reasoning:

1. scene users need semantic diagnostics,
2. DRP2 codes are useful for developers and fixture debugging,
3. this matches the current `DIAGNOSTICS.md` direction.

Owner decision:

TODO(user): agree


## Decision D038: First Converter Acceptance Tests

Question:

What should count as "ready to implement runtime" for the scene-to-DRP2 converter?

Recommendation:

Require converter fixtures for:

1. one-panel static point/pixel plot,
2. dynamic buffer update with no pipeline rebuild,
3. texture upload and image sampling,
4. picking render target plus readback,
5. offscreen render plus readback,
6. compute-assisted pressure case if compute is kept mandatory.

Owner decision:

TODO(user): agree, compute is mandatory


## Decision D039: First Runtime Acceptance Tests

Question:

What should count as "DRP2 Vulkan runtime MVP complete"?

Recommendation:

Require:

1. all positive DRP2 fixtures execute without protocol error,
2. all readback replies have correct shape and size,
3. at least one deterministic readback fixture verifies bytes,
4. runtime reports capabilities from the actual Vulkan device,
5. runtime maps validation/execution failures into DRP2 `Error` records.

Owner decision:

TODO(user): agree


## Decision D040: Spec Cleanup Aggressiveness

Question:

Should the cleanup pass delete or rewrite stale planning examples aggressively?

Recommendation:

Yes, but preserve useful content by moving it to updated examples or deferred notes.

Reasoning:

The branch explicitly allows API/spec churn for v0.4. Stale examples are actively harmful once
implementation starts.

Owner decision:

TODO(user): agree


## Notes From User

Use this section for broad comments that do not fit one decision.

TODO(user):
