# Scene/DRP2 Spec Cleanup Decision Log

> **Status:** `OWNER DECISIONS COMPLETE`
> **Created on:** `2026-04-28`
> **Purpose:** collect the owner decisions needed before a full consistency pass over
> `spec/drp2/`, `spec/scene/`, related examples, fixtures, and draft headers.

This document is intentionally not normative spec text yet.

All owner decisions needed for the first full cleanup pass have been recorded. Remaining ambiguity
should now be treated as implementation detail unless a cleanup edit exposes a concrete conflict.

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

Add these fields to the runtime capability snapshot before scene implementation starts, but do not
require every DRP2 JSON fixture to declare them.

Reasoning:

1. current DRP2 intentionally avoids backend-shaped alignment fields,
2. real Vulkan execution cannot ignore alignment for dynamic offsets,
3. scene planning needs this information if it packs many parameter blocks into shared buffers,
4. making the fields optional lets existing fixtures remain narrow.

Clarification:

The "core fixture corpus" means the executable JSON fixtures under `spec/drp2/fixtures/`. The
cleanup should add alignment limits to the capability model, but individual fixtures should only
mention those limits when they are testing dynamic buffer offsets or texture-copy alignment.

Suggested fields:

1. `min_uniform_buffer_offset_alignment`,
2. `min_storage_buffer_offset_alignment`,
3. `min_texture_copy_bytes_per_row_alignment`.

Owner decision:

TODO(user): clarified; add alignment limits to runtime capabilities, but keep fixture declarations
minimal unless the fixture exercises those limits.


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
6. custom visuals in the first proof slice, while still allowing an experimental v0.4 path later,
7. exact per-pixel linked-list transparency.

Reasoning:

This covers static geometry, dynamic buffers, texture sampling, offscreen/readback, and picking
without starting with font atlases, ray marching, impostors, or statistical composite geometry.

Owner decision:

TODO(user): agree


## Decision D015: Items, Spans, Groups, And Attribute Granularity

Question:

How should users declare structural spans, semantic groups, and attributes that vary at different
granularities?

Recommendation:

Do not use "grouped visual" as formal terminology. It is ambiguous.

Use four distinct concepts:

1. `item`: one logical datum at the visual's primary granularity,
2. `span`: a structural contiguous range of items, such as one path in `path` or one string in
   `glyph`,
3. `group`: a semantic/category identity, such as neuron population, species, condition, or class,
4. source granularity: where an attribute value is indexed from.

Use explicit named data attributes for structure and grouping:

1. `span_sizes` declares structural contiguous spans,
2. `group_id` declares semantic group identity,
3. `group_id` may be per item or per span depending on the visual and attribute contract,
4. a span may have a group id, but span identity and group identity are not the same thing.

Recommended source vocabulary:

1. `CONSTANT`: one value for the whole visual,
2. `PER_ITEM`: one value per item,
3. `PER_SPAN`: one value per structural span,
4. `PER_GROUP`: one value per semantic group/category,
5. family-specific extensions such as glyph `PER_CHAR`.

Reasoning:

1. this matches the string-based `dvz_visual_set_data()` model,
2. it avoids introducing `DVZ_ATTR_*` after the API direction rejected attr enums,
3. it prevents structural topology from being confused with semantic categories,
4. it handles flat visuals, span-structured visuals, and category encodings without overloading
   `PER_GROUP`.

Spec edits if accepted:

1. remove `dvz_visual_spans()` references unless you want that function,
2. replace `DVZ_ATTR_GROUP_ID` wording with `"group_id"`,
3. replace informal "grouped visual" wording with "span-structured visual" or a concrete family
   name,
4. audit every `PER_GROUP` mention and decide whether it really means `PER_GROUP` or `PER_SPAN`,
5. fix `PIXEL.md` to not imply semantic groups require contiguous item ranges unless a structural
   span representation is explicitly used.

Owner decision:

TODO(user): agreed direction from discussion; avoid "grouped visual" terminology and separate
structural spans from semantic groups.


## Decision D016: `PER_CHAR` Source For Glyph

Question:

Should glyph-specific `PER_CHAR` remain a formal attribute source?

Recommendation:

Keep it, but define it as a glyph-only extended source in `ATTRIBUTE_SOURCES.md`.

Clarification:

The question is whether source granularity should include a special glyph case for attributes whose
length is the total number of characters across all strings.

Example: a glyph visual with 10 strings has 10 `PER_ITEM` values, but if those strings contain 80
characters total, `char_color` with `PER_CHAR` has 80 values.

Reasoning:

1. per-character color is useful for text-heavy scientific labels and sequence logos,
2. forcing it into `PER_ITEM` would make string-level and character-level item identity ambiguous,
3. localizing it as a glyph extension keeps the core source model simple.

Owner decision:

TODO(user): clarified; keep glyph-only `PER_CHAR`.


## Decision D017: Missing-Data Semantics

Question:

Should NaN/Inf/missing values be specified before implementation?

Recommendation:

Yes. Add a shared scene rule and allow family overrides. Also add a systematic default-parameter
contract for every visual family.

Recommended defaults:

1. NaN positions: item is skipped and not pickable,
2. NaN scalar color/size: use scale missing-value color or fallback parameter,
3. Inf coordinates: validation warning or skip, depending on strictness mode,
4. texture NaNs: colormap missing color for scalar textures when supported.

Default-parameter contract:

1. every visual family must have a defaults table for optional attributes and parameters,
2. omitted attributes use the visual's default parameter value,
3. missing values inside provided attributes use the attribute's missing-value policy,
4. users may override defaults at visual or style level,
5. missing-value fallback values should be configurable per relevant parameter where feasible.

Reasoning:

Scientific data routinely contains missing values. Leaving this undefined will produce inconsistent
visuals and hard-to-debug backend behavior.

Owner decision:

TODO(user): agreed; add configurable missing-value fallback values and systematic defaults for every
visual family.


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

Keep custom visuals in v0.4 as an experimental/unstable feature, but do not make them part of the
first converter/runtime proof slice and do not treat the public custom-visual contract as stable yet.

Reasoning:

1. custom visuals follow the same broad pipeline as built-in visuals,
2. the complication is that they expose more of that machinery directly: shader contracts,
   attribute declarations, bind layouts, capabilities, picking payloads, transparency mode,
   diagnostics, and invalidation behavior,
3. built-in families should prove the converter/runtime first,
4. once the path is proven, an experimental custom-visual API can reuse it without promising a
   stable long-term contract immediately.

Owner decision:

TODO(user): agreed; v0.4 may include custom visuals, but experimental/unstable and not a first
proof-slice blocker.


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

Start with a logical 64-bit encoded object id resolved through a scene-side pick table:

1. panel id is request-side metadata,
2. encoded id maps to visual id and item/group/aux identity,
3. zero means no hit,
4. richer sampled-value probe payloads are separate readback requests.

Do not require native GPU 64-bit integer render-target support. The default physical encoding should
be two 32-bit unsigned channels, such as `rg32uint`, unless the runtime has a better supported path.

Reasoning:

1. integer render targets are already in DRP2 fixtures,
2. it avoids packing many semantic fields into shader output,
3. it keeps grouped and batched rendering compatible with picking.

Owner decision:

TODO(user): logical 64-bit pick ids, physically encoded without requiring GPU 64-bit target support.


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

Implement opaque rendering and weighted blended OIT in v0.4. Weighted blended OIT is a hard v0.4
requirement, not an optional stretch goal. Keep exact per-pixel linked-list OIT deferred.

Reasoning:

Transparency quality matters for sci-viz. This requirement means the DRP2/vklite runtime must
support the render-pass and pipeline features needed by weighted blended OIT:

1. multiple color attachments,
2. per-attachment blend state,
3. accumulation and revealage targets,
4. a resolve/composite pass.

Owner decision:

TODO(user): weighted blended OIT is a hard v0.4 requirement.


## Decision D029: Volume Baseline

Question:

Should `volume` be part of the first scene-to-DRP2 implementation slice?

Recommendation:

No for the first proof slice. Keep the spec and implement volume in v0.4 after 2D images, basic
mesh, texture sampling, offscreen rendering, picking, and the core transparency path are working.

Reasoning:

Volume requires ray marching, 3D textures, transfer functions, crop/bounds semantics, and special
picking/probe rules. It is a good second wave, not the first wave.

Owner decision:

TODO(user): agree; not first proof slice, but still v0.4 scope.


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

Implement basic indexed mesh with color/normal/texture modes first, and keep mesh edge overlay in
the v0.4 mesh baseline. Defer mesh isolines until the basic mesh and edge-overlay paths are stable.

Reasoning:

The first mesh target should prove indexed geometry, depth, culling, optional lighting, and the edge
overlay path already known from v0.3. Isolines add extra contour-generation semantics and can remain
a later mesh extension.

Owner decision:

TODO(user): edge overlay is important and should be v0.4 mesh scope; mesh isolines may be deferred.


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

Implementation detail:

Use one authoritative internal mapping table from `DvzFormat` to DRP2 format strings and Vulkan
formats. Add implementation tests/static checks so supported scene formats cannot silently drift from
their Vulkan mappings. Debug logs may show both the Datoviz format name and the Vulkan format name.
Do not document or promise numeric enum equality with Vulkan.

Owner decision:

TODO(user): agreed; keep Datoviz-owned public enums and enforce internal mapping consistency without
documenting numeric Vulkan equality.


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


## Follow-Up D041: Should `PER_SPAN` Be Formal?

Question:

After separating spans from groups, should `PER_SPAN` become a formal source granularity?

Recommendation:

Yes. Add `PER_SPAN` formally.

Reasoning:

1. `path` linewidth/color "per path" is structurally per span, not semantically per group,
2. `glyph` per-string attributes are structurally per span when strings are stored as character
   spans,
3. using `PER_GROUP` for this would keep the old ambiguity,
4. `PER_GROUP` should mean category-indexed data keyed by `group_id`, not contiguous topology.

Owner decision:

TODO(user): agree


## Follow-Up D042: Defaults And Missing-Value API Shape

Question:

How should users override visual defaults and NaN/Inf fallback values?

Recommendation:

Use a style/defaults object plus direct per-visual override helpers:

1. each visual family has documented built-in defaults,
2. users may set a `DvzStyle` or family-specific default block on a visual,
3. users may override individual defaults by attribute name,
4. missing-value policies are per attribute where relevant, not global-only,
5. scale objects own missing colors for scalar color mappings.

Reasoning:

This keeps the common path simple while making defaults inspectable, serializable, and reusable
across visuals. It also avoids hiding important sci-viz behavior inside shader constants.

Owner decision:

TODO(user): agree


## Follow-Up D043: Experimental Custom Visual Scope

Question:

What is the minimum v0.4 custom-visual surface if the API is explicitly experimental?

Recommendation:

Support an experimental descriptor-based path only:

1. user supplies visual-family descriptor, attributes, parameters, shader source, and render state,
2. no shader hot reload requirement,
3. no stable ABI/API promise,
4. picking support limited to the default 64-bit pick-id path,
5. custom compute stages are deferred unless already needed by built-in visuals.

Reasoning:

This lets advanced users test the pipeline without forcing the first release to stabilize every
custom extension point.

Owner decision:

TODO(user): agree


## Follow-Up D044: Weighted Blended OIT Fallback Policy

Question:

If weighted blended OIT is a hard v0.4 requirement, what happens on a runtime that cannot provide
the needed attachments/blend state?

Recommendation:

Treat missing weighted blended OIT capability as a runtime capability failure for visuals that
request transparent rendering, not as an implicit downgrade to ordinary alpha blending.

Reasoning:

If WBOIT is required for correctness, silently downgrading changes visual meaning. A user-selected
policy may explicitly request fallback, but the default should report a clear capability diagnostic.

Owner decision:

TODO(user): agree


## Follow-Up D045: Mesh Edge Overlay Contract

Question:

How much of the v0.3 mesh edge-overlay behavior should be specified for v0.4?

Recommendation:

Make edge overlay a v0.4 mesh feature, but specify it as scene semantics rather than committing the
spec to the v0.3 shader/baking trick:

1. edge overlay can be enabled per mesh,
2. edge color, width, and opacity have defaults and overrides,
3. the implementation may use baked edge attributes, barycentric coordinates, or a separate edge
   pass,
4. mesh isolines remain deferred.

Reasoning:

This preserves the important user-facing feature while letting the v0.4 runtime choose the cleanest
implementation path on top of DRP2/vklite.

Owner decision:

TODO(user): agree


## Notes From User

Use this section for broad comments that do not fit one decision.

TODO(user):
