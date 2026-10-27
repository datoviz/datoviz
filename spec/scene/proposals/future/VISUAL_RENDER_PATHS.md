> **Execution Status**
> - **Status:** `FUTURE PROPOSAL` (post-v0.4; non-normative)
> - **Updated on:** `2026-10-27`
> - **Purpose:** define a modular path from semantic visuals to alternative GPU representations and execution strategies without introducing another renderer.
> - **Scope:** internal scene architecture, render products, capability resolution, derived resources, and evidence-gated Slug and ray-rendering experiments. No v0.4.0 release gate.

# Semantic Visuals and Multiple Render Paths

## Decision to Explore

Keep one stable semantic visual identity and data contract while allowing multiple **render paths** to realize that visual through different GPU representations or operations. Select paths during scene planning, before FramePlan pass expansion, using required render products, target capabilities, explicit user policy, and deterministic fallback rules. All paths contribute declarative work to the existing scene -> FramePlan -> DRP2 -> runtime stack.

This is an internal post-v0.4 architecture proposal, not a new public renderer API, a replacement for existing family semantics, or permission to add a separate Vulkan/WebGPU execution route.

## Distinct Responsibilities

| Concept | Owns | Does not own |
| --- | --- | --- |
| Semantic visual/family | Authoritative data, parameters, transforms, item identity, query meaning | GPU layout, shader modules, passes |
| Render path | Eligibility, derived representation, resource needs, typed work contributions, supported product contracts | Scene-wide scheduling, device handles, presentation |
| Shader/pipeline variant | Color, pick, lighting, WBOIT, backend shader language and compatible pipeline ABI *within one path* | Choice of semantic family or independent representation |
| Scene technique/composer | Required product closure, coherent color/depth/normal/ID outputs, dependencies, composition | Reinterpreting family data or bypassing render-path contracts |
| DRP2/runtime | Validated GPU operations, resource realization, synchronization, execution | Semantic visual selection policy |

Do not conflate changes in authored meaning (e.g. volume slice versus MIP), changes in a render technique (e.g. WBOIT versus ordinary blending), and alternative implementations of the same meaning (e.g. a rasterized versus analytic filled contour). Hardware ray tracing may also require a **panel/scene-level technique**, not merely an extra per-visual pipeline.

## Current Starting Point (RC4 Integration Source Audit)

- The [visual-family registry](../../../../src/scene/visuals/registry/registry.h) has one `DvzVisualFamilyOps` per visual type, with callbacks for lowering, pass capabilities, shader/pipeline/bind/draw descriptors, and metadata. It is already the correct family ownership boundary.
- Existing alternatives are specialized: point-like work selects native points or instanced quads; sphere has fast and analytic impostor modes (currently sharing the sphere pipeline); volume selects slice/MIP/composite semantics; semantic text produces derived glyph visuals.
- [`DvzFramePlanVisualMeta` and raster descriptors](../../../../src/scene/frame_plan/frame_plan.h) still encode specific visual/renderable kinds, resource fields, and draw assumptions. A second geometry or execution representation would otherwise add branches and fields across lowering and emission.
- [Visual boundaries](../../visuals/BOUNDARY_CONTRACT.md) prohibit new concrete-family switches in generic scene code. [FramePlan](../../pipeline/FRAME_PLAN.md) already owns logical GPU work and dependencies; [graph techniques](../../implementation/GRAPH_TECHNIQUES.md) already own typed products and pass expansion.

**Gap:** a family can select many shader variants, but cannot yet register several independently owned render paths that describe different derived resources, work kinds, and product capabilities through one common contract.

## Proposed Internal Boundary

Keep the existing `DvzVisualFamilyOps` as the semantic/family registry. Add a small, static **render-path registry** (possibly grouped under each family) with a stable internal path ID and contract version. A render path has two primary operations:

1. **Probe:** read an immutable semantic visual snapshot, panel/render request, required products, and target capabilities; report eligibility, limits, fidelity, output contracts, and a diagnostic reason when unsupported. No allocations, GPU calls, cache mutation, or hidden fallback.
2. **Lower:** produce a value-owned, declarative **render recipe** with required derived resources, logical identities, cache dependencies, resource bindings, ordered draw/compute work, and typed product/query contributions. It must not record GPU commands or manage passes directly.

The scene composer first resolves semantic output requests and technique requirements, then selects compatible paths, expands their recipes into existing FramePlan graph resources and passes, and validates the complete graph. The scene-to-DRP2 converter and backend remain generic. An ordinary path may emit one draw; a more complex path may emit multiple linked operations. Do not require every path to produce every possible product or work class.

These are conceptual contracts, **not frozen public C types or ABI**. Start by adapting existing descriptor callbacks instead of rewriting all visual families. Keep shader variants inside paths and avoid one registry entry per feature combination.

## Selection and Correctness Rules

1. Choose by **semantic equivalence first**, then required product availability, target capabilities, explicit quality/performance policy, and deterministic preference order. Cost estimates and heuristics are optional until measured.
2. Default to one coherent representation per visual/panel for color, coverage, depth, query identity, and other geometry-dependent products. A different query or depth path is allowed only after it proves equivalent geometry, clipping, alpha validity, and item mapping.
3. Selection is per consumer/panel where output requirements differ; immutable derived geometry/atlases may still be shared across panels and render requests. Avoid mutable global selected-path state on a shared visual.
4. An unsupported path must produce an early, actionable diagnostic or use a declared, semantics-preserving fallback. Never substitute a color-only shader for depth, WBOIT, or picking without validating the contract. No CPU fallback for rendered-visual GPU picking.
5. The selected path ID/version, adaptation reason, resources, and product contributions should be inspectable in FramePlan diagnostics and test fixtures. Cache keys include any ABI-affecting layout, shader, topology, or resource schema differences.
6. Keep product/technique closure explicit. The composer must reject missing producers, feedback cycles, or incompatible color/depth/normal/coverage/identity contracts; selection must not be deferred until shader lookup after the graph is committed.

## Derived Representation and Lifetime

Semantic inputs remain authoritative. Render-path artifacts are derived and independently invalidated: triangulated meshes, instanced quads, Slug curve/band atlases, GPU-generated geometry, and eventually acceleration structures. Cache identity should cover source revisions, path/algorithm version, relevant generation settings, and capability/device epoch. Separate shareable static artifacts from per-panel instance, camera, and output state.

Camera motion should not regenerate fixed Slug curve data; transform changes should not rebuild mesh vertices or BLAS when a TLAS instance update suffices; item style changes should not rewrite geometry when a parameter update suffices. Switching paths must retire stale logical resources safely and preserve visual/item identity, panel attachments, and readback routing. Follow [resource ownership](../../pipeline/RESOURCE_MODEL.md) and [invalidation/caching](../../pipeline/INVALIDATION_AND_CACHING.md), rather than creating another cache framework.

## Concrete Pressure Tests

| Semantic input | Candidate paths | What must remain equivalent |
| --- | --- | --- |
| Point | Native points, instanced quads (existing lowerings) | Item identity/ranges, sizing and picking where applicable |
| Filled vector shape (future family candidate) | Flatten + triangulate, analytic Slug curve/band rendering | Fill rule, contours, gradients/masks where supported, clipping, coverage, picking |
| Text (`DvzText`) | Bitmap/MSDF, analytic Slug glyph coverage, CPU-raster block when appropriate | Shared shaping/fallback/layout, glyph identity, placement, DPI and sizing contracts |
| Sphere | Analytic impostor, optional tessellated sphere | Physical radius, surface depth/normal/coverage, material and query identity |
| Mesh/volume in a ray-capable scene | Raster geometry/fragment raymarching, future compute or RT geometry contributors | Requested scene-level lighting, visibility, depth, IDs and volume-hit semantics |

### Slug/Slughorn Experiment

Use [Slughorn](https://github.com/AlphaPixel/slughorn), [osgSlug](https://github.com/AlphaPixel/osgSlug), and [the original Slug reference](https://github.com/EricLengyel/Slug) as implementation and testing references; do not assume a mandatory C++ dependency. Begin with an **offline prepared** curve/band asset and a minimal Datoviz graphics path consuming ordinary DRP2 textures/buffers/draws. Compare the existing SVG Tiger triangulation with analytic coverage under zoom and perspective. Measure atlas memory, preprocessing, fragment cost, GPU timings, visual errors, and GPU query agreement. Validate floating/half-precision and band-format edge cases. Audit upstream licenses and preserve attribution before porting code.

Do not repurpose the scientific polyline/stroke `path` family as a closed filled-contour family without revisiting its semantics. A new filled vector-shape family is a candidate, not an approved API. For `DvzText`, preserve text shaping/layout and semantic string identity above the rendering path; do not force a curve atlas into the existing UV-only low-level glyph descriptor.

### Ray Rendering Boundary

Per-visual paths may supply raster draws, compute-derived resources, or future acceleration-structure geometry. **Scene-level ray integration** must own cross-object light transport and visibility, shared acceleration structures, and multi-pass/progressive accumulation, with its outputs combined through typed products. Start with graph-expressible compute/raster work; hardware ray tracing needs explicit new DRP2/runtime capabilities and must not be simulated by shader-name conventions or leaked Vulkan handles. Preserve WebGPU diagnostics and fallbacks.

See [ray-tracing forward compatibility](RAY_TRACING_FORWARD_COMPAT.md) and [unified ray rendering](UNIFIED_RAY_RENDERING.md) for their existing boundaries. This proposal defines only their common visual-realization mechanism, not a ray tracer design.

## Incremental Work and Acceptance Gates (After v0.4.0 Final)

| Stage | Change | Evidence required before advancing |
| --- | --- | --- |
| R0: audit | Inventory current family lowerings, descriptor fields, shader variants, resource cache/retirement paths, pass and query requirements; specify narrow probe/recipe interfaces. | One written interface contract and a map of changes without public API or runtime refactor |
| R1: existing-path proof | Represent point native/instanced lowering behind internal path descriptors while preserving current behavior and public API. | Matching native/WebGPU-appropriate fixtures, stable IDs/ranges, no redundant uploads or new concrete-family dispatch |
| R2: new representation | Prototype triangulated versus Slug-filled shape as separate paths, with offline assets and normal FramePlan/DRP2 emission. | Visual/CPU-reference comparisons, GPU query agreement, resource lifecycle tests, measured costs across zoom and hardware |
| R3: shared abstraction | Generalize typed derived-resource/binding/work recipes only where R1/R2 exposed repeated logic; document selection and fallback policy. | Static and dynamic data reuse, multi-panel consistency, explicit unsupported combinations, regression and performance counters |
| R4: text application | Add analytic Slug as an optional realization behind semantic text/shaping; keep MSDF as default unless evidence justifies another policy. | UTF-8/font fallback and placement preserved, quality/cost benchmarks, no unrelated ABI changes |
| R5: compute/ray pressure | Prove a path with compute-derived work, then consider scene-level ray technique and native RT extensions separately. | Correct graph barriers/products, portable capability reporting, no parallel renderer, independently justified DRP2 extension |

For each stage, test deterministic selection and failure diagnostics, item identity and query coverage, dirty ranges, resource destruction/retirement, batching, multi-panel sharing, screenshot/CPU-reference fidelity, and native/WebGPU support only where the path is advertised. Benchmark before introducing automatic quality switching or more elaborate cost ranking.

## Non-Goals and Promotion

- No RC3, RC4, or v0.4.0 final requirement; no v0.4 architectural rewrite just to prepare for this proposal.
- No public render-path plugin API, dynamic provider loader, extra presentation framework, or scene-private Vulkan commands.
- No blanket conversion of `lit`, `pick`, `WBOIT`, shader format, or semantic volume render mode into separate render-path objects.
- No commitment to automatic fastest-path selection, hardware ray tracing, generalized SVG support, or shipping Slughorn itself before focused proofs.

Promote only validated rules into [visual semantics](../../semantics/VISUAL_CONTRACT.md), [family boundary](../../visuals/BOUNDARY_CONTRACT.md), [FramePlan](../../pipeline/FRAME_PLAN.md), [graph techniques](../../implementation/GRAPH_TECHNIQUES.md), and if necessary [DRP2 contracts](../../../drp2/README.md). Keep this proposal as a short decision/rationale record after promotion.

## Open Decisions

1. Does one internal path descriptor suffice for both a raster draw and a multi-operation recipe, or is a shared work-contribution interface needed only after R2?
2. Where should optional public quality/preference controls live (visual versus panel versus export request), without exposing implementation-specific handles or creating unstable automatic switches?
3. Can a single visual use multiple paths for separate semantic products without compromising geometric/query coherence? Defer until evidence establishes a safe equivalence contract.
4. Is a filled vector-shape visual sufficiently distinct from existing `path`, `vector`, or glyph semantics to warrant a new public family?
