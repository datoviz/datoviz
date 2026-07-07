# Datoviz v0.4 GSP Compatibility Preparation Plan

**Recommended location in the Datoviz repository:**

```text
spec/scene/proposals/active/GSP_COMPATIBILITY_PREP.md
```

**Status:** active implementation proposal / agent handoff document
**Audience:** Datoviz v0.4 agents, maintainers, GSP backend authors
**Scope:** Datoviz-side changes only. This document prepares Datoviz v0.4 to become the flagship GPU backend for GSP, while keeping GSP itself backend-neutral.

---

## 1. Purpose

GSP, the Graphics Server Protocol, is intended to be a backend-independent semantic protocol for interactive scientific visualization. Datoviz v0.4 should be able to serve as a first-class high-performance GSP runtime through a raw C API / ctypes path.

This document defines the Datoviz-specific preparation work required so that a future `gsp-backend-datoviz` adapter can translate GSP scenes, resources, transforms, queries, and frame requests into Datoviz v0.4 without depending on private Python objects, v0.3 API assumptions, Vulkan internals, or ad hoc backend escape hatches.

The main goal is:

```text
GSP semantic scene
  -> gsp-backend-datoviz adapter
    -> Datoviz v0.4 public C API
      -> scene FramePlan
        -> DRP2 artifact
          -> Datoviz runtime / native GPU / WebGPU-WASM path
```

Datoviz should not embed GSP as a dependency. Datoviz should expose a clean, stable, ctypes-friendly scene/runtime/query/capability surface that makes a GSP adapter natural and efficient.

---

## 2. Non-goals

The following are explicitly out of scope for this Datoviz-side preparation plan:

1. Implementing the GSP protocol inside Datoviz.
2. Recreating a high-level Python OO plotting API inside Datoviz.
3. Preserving the old Datoviz v0.3 Python backend used by the current GSP prototype.
4. Adding JSON/base64 serialization to the local render path.
5. Exposing Vulkan, swapchain, command-buffer, allocator, or platform-window internals to scene or GSP-facing APIs.
6. Adding backend-specific public shortcuts that bypass scene validation, capability adaptation, FramePlan construction, or DRP2 emission.
7. Reintroducing public pick/probe APIs as a parallel model to panel query.

---

## 3. Existing Datoviz concepts GSP should build on

The Datoviz v0.4 scene design is already very close to what GSP needs.

### 3.1 Scene / figure / panel / visual ownership

Datoviz should continue to expose the retained scene object model:

```text
DvzScene
DvzFigure
DvzPanel
DvzVisual
DvzSceneBuffer
DvzSampledField
DvzSceneCompute
DvzView / DvzApp runtime boundary
```

The GSP adapter should create and mutate these objects through public C functions only.

### 3.2 DRP2 boundary

Scene should remain a producer of DRP2 work, not a backend runtime. GSP should translate to scene-level Datoviz semantics, not to Vulkan or low-level runtime objects.

Required invariant:

```text
GSP adapter may call Datoviz scene/app/view public APIs.
GSP adapter must not call Vulkan, vklite, swapchain, canvas, or backend command-buffer APIs directly.
```

### 3.3 FramePlan and frame artifact

GSP needs a deterministic route from retained scene state to emitted frame work. Datoviz already has `FramePlan` and `DvzSceneFrameArtifact` concepts. The adapter should rely on the normal Datoviz frame lifecycle rather than constructing backend commands itself.

### 3.4 Capability adaptation

GSP requires explicit capability discovery and deterministic adaptation. Datoviz should keep and harden the capability snapshot and adaptation model so the GSP adapter can choose one of:

```text
accept
simplify
deactivate
reject
```

No GSP-facing Datoviz path should silently degrade semantic meaning.

### 3.5 Unified panel query

GSP should map directly onto Datoviz's unified panel query model:

```text
What rendered scene contribution is under this panel coordinate?
```

Datoviz should keep query as the public mechanism and avoid reviving separate public pick/probe APIs.

---

## 4. Required Datoviz readiness targets

Datoviz v0.4 is considered minimally GSP-ready when the following are possible through public C API calls and a raw ctypes wrapper.

### 4.1 Runtime and capability readiness

A GSP backend can:

1. Create a scene, figure, app, and view.
2. Query a live runtime/view/app capability snapshot.
3. Pass a capability snapshot into scene planning when needed.
4. Serialize or inspect capability snapshots for tests.
5. Determine whether native, offscreen, query, readback, WGSL/WebGPU, GLSL, integer render targets, and key visual/query paths are available.

Potential public API shape, exact names open:

```c
DVZ_EXPORT bool dvz_view_capabilities(const DvzView* view, DvzCapabilitySnapshot* out);
DVZ_EXPORT bool dvz_app_capabilities(const DvzApp* app, DvzCapabilitySnapshot* out);
DVZ_EXPORT bool dvz_scene_default_capabilities(DvzCapabilitySnapshot* out);
```

If `DvzView` is the actual owner of runtime reuse, prefer a `DvzView`-centered function.

### 4.2 ctypes-friendly public ABI

The GSP adapter must be able to call Datoviz from a raw generated or hand-written ctypes wrapper.

Requirements:

1. Public functions exported with `DVZ_EXPORT`.
2. Public descriptors use `struct_size` and `flags` where future growth is expected.
3. Public structs use bindable scalar fields, fixed-size arrays, or explicit pointer/count pairs.
4. Ownership is documented for every pointer.
5. Borrowed vs copied data is explicit.
6. Functions return clear success/failure status.
7. Diagnostics are retrievable in scene-visible terms.
8. No required use of private Python classes or internal C headers.

### 4.3 Stable semantic identity route

GSP needs stable protocol identities for scenes, figures, panels, visuals, resources, transforms, and query results.

Datoviz should provide or confirm:

1. Stable scene-visible IDs for panels, visuals, resources, and query targets.
2. A way to associate optional user/protocol IDs or labels with Datoviz objects.
3. Query results that return scene-visible identity, not backend identity.
4. Diagnostics that mention scene-visible identity and resource/plan identity.

Potential API direction, exact names open:

```c
DVZ_EXPORT uint64_t dvz_visual_id(const DvzVisual* visual);
DVZ_EXPORT uint64_t dvz_panel_id(const DvzPanel* panel);
DVZ_EXPORT bool dvz_visual_set_user_id(DvzVisual* visual, uint64_t id);
DVZ_EXPORT bool dvz_panel_set_user_id(DvzPanel* panel, uint64_t id);
```

If IDs already exist internally, expose them consistently. If user IDs are not desirable as core API, provide labels/resource keys sufficient for the GSP adapter to maintain a robust mapping.

### 4.4 Resource and attribute update readiness

A GSP backend can:

1. Create scene-owned buffers.
2. Upload full buffer contents.
3. Upload subranges without reallocating the whole visual.
4. Bind scene buffers to visual attributes.
5. Bind scene buffers to index slots.
6. Bind scene buffers to compute passes.
7. Support copied CPU data and external/live runtime resources where declared.
8. Report dirty ranges and upload failures through diagnostics.

Existing APIs such as dense visual data updates, range updates, scene buffers, and visual attribute buffers should be hardened and covered by tests.

The GSP local path must be:

```text
NumPy / memoryview / ctypes pointer
  -> Datoviz public C call
    -> retained scene resource / dirty range
      -> FramePlan upload node
        -> GPU upload
```

It must not require JSON/base64 or whole-scene reconstruction.

### 4.5 Visual-family readiness

For the first GSP slice, Datoviz should support at least:

```text
point
image
mesh or primitive, if feasible
text/glyph later
volume later
```

For every supported family, Datoviz should expose or document:

1. Constructor.
2. Required attributes.
3. Optional attributes.
4. Attribute names, item sizes, dtypes, and counts.
5. Style/material descriptors.
6. Query capabilities.
7. Supported coordinate spaces.
8. Supported controller modes.
9. Fallback/adaptation behavior.

A GSP-facing agent should be able to write a table like:

```text
GSP family point
  -> Datoviz constructor dvz_point()
  -> attributes: position, color, diameter_px, item_state, edge_color, stroke_width_px
  -> query: visual id, item id, optional link key, displayed RGBA
```

### 4.6 Transform readiness

GSP transforms should map to Datoviz's existing staged transform model rather than bypass it.

Datoviz should confirm or expose enough API for:

1. Data-domain normalization.
2. Panel domain and visible-domain updates.
3. Panel view policy.
4. Controller navigation: panzoom, arcball, turntable, and fly where available.
5. Visual attachment coordinate spaces.
6. Matrix/model transforms for visuals.
7. Colormap/scalar-to-color paths for sampled fields and scalar visuals.
8. Material and lighting parameters for mesh/sphere/volume paths.

Important rule:

```text
Navigation changes should not force data normalization rebuild or bulk data reupload.
```

Potential API gap to evaluate:

```text
Is DvzVisualTransformDesc sufficient for GSP matrix/model transforms only,
or should Datoviz expose a more general scene transform descriptor for future
nonlinear, colormap, and data-source transforms?
```

Do not add arbitrary Python transform hooks inside Datoviz. GSP producer-side transforms can run in Python before calling Datoviz. Datoviz should expose backend/runtime capabilities and built-in scene transforms.

### 4.7 Query/readback readiness

A GSP backend can:

1. Queue a panel query from panel-local coordinates.
2. Process pending queries through the figure/view/runtime path.
3. Poll query results.
4. Distinguish hit, miss, outside panel, stale/dropped, unsupported, GPU failure, readback failure, and decode failure.
5. Return scene-visible IDs.
6. Return item/group/face/voxel/texel IDs where family-supported.
7. Return visual/data/UVW positions where family-supported.
8. Return displayed RGBA and scalar/vector/category/text payloads where family-supported.
9. Support latest-wins hover semantics.
10. Make unsupported frontmost visuals explicit rather than falling through silently to background visuals.

The first required conformance case is:

```text
point-over-image panel query
```

Expected outcomes:

1. Query on point center returns point visual ID and item ID.
2. Query on image-only area returns image visual ID and texel/source coordinate where supported.
3. Query outside panel returns outside-panel status.
4. Unsupported visual family returns unsupported status, not a false miss.

### 4.8 Offscreen/export readiness

A GSP backend can:

1. Create an offscreen view.
2. Render one frame deterministically enough for tests.
3. Capture PNG bytes or write a PNG file through public API.
4. Return diagnostics for unsupported deterministic export.

Vector export remains the Matplotlib/reference-backend responsibility in early GSP. Datoviz only needs robust raster/offscreen capture for the first slice.

### 4.9 WebGPU/WASM readiness

Datoviz should remain WebGPU/WASM-compatible where already planned.

GSP-facing Datoviz work should avoid Vulkan-only assumptions:

1. Use logical Datoviz/DRP2 formats in scene-facing contracts.
2. Prefer WGSL-compatible shader paths where feasible.
3. Avoid relying on native 64-bit integer render targets as required baseline.
4. Keep query profile selection capability-driven.
5. Keep readback format/row-pitch constraints in capability snapshots.
6. Ensure custom future GSP extensions can be capability-gated by shader format and runtime features.

---

## 5. Concrete work packets for Datoviz agents

Use these as mission/task seeds in the Datoviz repo. They are intentionally Datoviz-side only.

### DVT-GSP-001 — Public runtime capability query

**Goal:** provide a public C API route to fill `DvzCapabilitySnapshot` from a live Datoviz runtime/view/app.

**Read:**

```text
spec/scene/core/RUNTIME_BOUNDARY.md
spec/scene/validation/ADAPTATION.md
spec/drp2/CAPABILITIES.md
include/datoviz/scene/types.h
include/datoviz/scene.h
src/scene/*cap* if present
src/app/* / src/view* if relevant
```

**Allowed edits:**

```text
include/datoviz/scene*.h
include/datoviz/app*.h or view headers, if that is where DvzView lives
src/scene/
src/app/ or src/view runtime bridge
testing/
examples/c/
spec/scene/ proposals/docs if API shape is updated
```

**Acceptance:**

1. Public C function fills a `DvzCapabilitySnapshot`.
2. Function works for at least native/offscreen runtime path.
3. Snapshot includes query/readback/integer-format fields already present in `DvzCapabilitySnapshot`.
4. C test verifies nonzero/default sane fields.
5. No Vulkan handles or private runtime objects leak into scene API.

---

### DVT-GSP-002 — ctypes smoke path

**Goal:** ensure the v0.4 public C API is straightforward to call from Python ctypes.

**Read:**

```text
include/datoviz/*.h
include/datoviz/scene*.h
examples/c/
```

**Acceptance:**

1. Minimal Python ctypes smoke script can load Datoviz library.
2. It can call scene/figure/app/view creation or a documented minimal subset.
3. It can call capability snapshot function from DVT-GSP-001.
4. It can create a point visual and set dense attributes, or records the exact blocker.
5. It does not depend on private Python classes.

This task may live in Datoviz tests/examples or as a documented external smoke test, depending on repository policy.

---

### DVT-GSP-003 — Stable identity and label route

**Goal:** make sure GSP can map protocol object IDs to Datoviz scene identities and back from query/diagnostics.

**Acceptance:**

1. Document current ID behavior for scene, figure, panel, visual, resource, query targets.
2. Expose missing stable IDs or labels if necessary.
3. Query result identity fields are scene-visible and stable.
4. Diagnostic reports can identify visual/resource/panel/plan subjects.
5. Tests verify that a query result can be mapped back to the visual created by public API.

---

### DVT-GSP-004 — Resource and range update hardening

**Goal:** ensure GSP can use efficient direct local data paths.

**Acceptance:**

1. Dense visual updates support copied data and clear item-count validation.
2. Range updates update only dirty ranges.
3. Scene buffers can back visual attributes where supported.
4. Scene buffers can be used by compute where supported.
5. Tests cover full update, range update, and buffer-backed visual attribute for at least point or primitive/mesh.
6. No JSON/base64 path is required.

---

### DVT-GSP-005 — Visual-family metadata table

**Goal:** document and, if useful, expose per-family capabilities for the GSP adapter.

**Deliverable:** a table in `spec/scene/visuals/` or implementation docs with:

```text
family
constructor
attributes
item count source
dtypes/item sizes
coordinate support
material/style support
query capability
known unsupported/deferred behavior
```

**Acceptance:**

1. First table covers point, image, mesh/primitive if available.
2. Query capabilities are explicit.
3. Unsupported features are named.
4. GSP adapter author can implement mappings without reading private visual internals.

---

### DVT-GSP-006 — Panel query GSP readiness

**Goal:** make the unified panel query path reliable enough for GSP's first interactive proof.

**Acceptance:**

1. Public query request/result structs are stable enough for ctypes.
2. Public functions cover queue/process/poll or queue/render/poll flow.
3. Point-over-image test exists.
4. Query status enum distinguishes miss vs unsupported vs readback/GPU/decode failures.
5. Query result includes scene-visible visual ID and item/texel payload where supported.
6. Latest-wins hover behavior is documented and tested or explicitly deferred.

---

### DVT-GSP-007 — Offscreen render/capture smoke

**Goal:** make GSP tests able to render and capture Datoviz output without a Python OO wrapper.

**Acceptance:**

1. Public API can create an offscreen view for a figure.
2. Public API can render once.
3. Public API can capture PNG or deterministic image bytes/file.
4. C example demonstrates point visual offscreen capture.
5. ctypes smoke test can call the same path or records exact blocker.

---

### DVT-GSP-008 — Transform and coordinate mapping audit

**Goal:** verify Datoviz exposes enough transform/control API for GSP point/image/query demos.

**Acceptance:**

1. Document mapping for GSP coordinate spaces to Datoviz coordinate spaces.
2. Document mapping for panel data domains and view extent.
3. Document mapping for panzoom and arcball.
4. Confirm navigation changes do not reupload bulk data.
5. Identify whether any additional public C API is needed.

---

### DVT-GSP-009 — GSP compatibility smoke example

**Goal:** create a Datoviz-side C example equivalent to the first GSP demo.

**Scene:**

```text
one scene
one figure
one panel
one image or background sampled field if available
one point visual overlay
one query at a point location
one offscreen PNG capture
```

**Acceptance:**

1. Builds in normal Datoviz test/example workflow.
2. Uses only public C API.
3. Produces image output in offscreen mode if available.
4. Produces query result with expected visual/item identity.
5. Documents any temporarily skipped path.

---

## 6. GSP adapter mapping target

The future GSP Datoviz backend should be able to implement the following mapping without private Datoviz internals.

| GSP concept | Datoviz v0.4 public target |
|---|---|
| GSP server/session | `DvzApp`, `DvzView`, runtime reuse |
| canvas/figure | `DvzFigure` |
| panel/viewport | `DvzPanel` |
| point visual | `dvz_point()` + dense/buffer attributes |
| image visual | sampled field / image visual path |
| mesh visual | mesh/primitive visual path |
| resource buffer | `DvzSceneBuffer` |
| attribute data | `dvz_visual_set_data_many`, `dvz_visual_set_data_range`, `dvz_visual_set_attr_buffer` |
| transform: panel domain | `dvz_panel_set_domain`, panel view APIs |
| transform: controller | panzoom/arcball/turntable/fly APIs |
| scalar colormap | `DvzScale`, `DvzColormap`, sampled field or color mapping path |
| query request | `dvz_panel_query` or equivalent |
| query completion | `dvz_scene_poll_query` or equivalent |
| offscreen render | `DvzView` offscreen render/capture path |
| capabilities | public `DvzCapabilitySnapshot` query |
| diagnostics | `DvzDiagnosticReport` or scene-visible diagnostic path |

If any row cannot be implemented through public C API, create a `GSP_API_GAP` note as described below.

---

## 7. API gap reporting protocol

When a Datoviz or GSP agent discovers that GSP cannot be implemented cleanly through public Datoviz v0.4 API, create a gap file:

```text
spec/scene/proposals/active/GSP_API_GAP_<short-name>.md
```

Template:

```markdown
# GSP API Gap: <short name>

## Summary

One paragraph.

## Blocked GSP capability

What GSP adapter cannot implement cleanly.

## Current Datoviz state

What exists today.

## Missing public API or behavior

Exact missing function, struct field, status, diagnostic, test, or lifecycle behavior.

## Proposed Datoviz-side solution

Preferred minimal API or implementation change.

## Alternatives rejected

Why private/internal workaround is not acceptable.

## Acceptance tests

Concrete C/ctypes tests or examples that prove the gap is closed.

## Requires ChatGPT Pro consultation?

Yes/No. If yes, include the exact prompt and expected output.
```

---

## 8. ChatGPT Pro consultation trigger

Most implementation tasks should be handled by coding agents. Use ChatGPT Pro / high-reasoning consultation only when a decision is architectural, cross-repo, hard to reverse, or affects GSP semantics.

Create a consultation file:

```text
.agent/consultations/PXXX-<topic>.md
```

or, if `.agent/` is not present in Datoviz:

```text
spec/scene/proposals/active/GSP_CONSULT_<topic>.md
```

Template:

```markdown
# ChatGPT Pro Consultation: <topic>

## Context

Relevant Datoviz files/specs and current state.

## Decision needed

The precise question.

## Options

A, B, C with tradeoffs.

## Constraints

Datoviz v0.4 pre-RC, GSP compatibility, WebGPU/WASM, ctypes, scene/DRP2 boundary.

## Prompt to paste into ChatGPT Pro

<exact prompt>

## Expected output from ChatGPT Pro

- Recommended decision
- Rationale
- Datoviz API impact
- GSP adapter impact
- Concrete tasks
- Acceptance criteria
```

Recommended consultation topics:

1. Generalized transform descriptor vs current matrix-focused transform descriptor.
2. Runtime capability query ownership: `DvzView`, `DvzApp`, or runtime service.
3. Stable user/protocol ID design.
4. GSP virtual data-source support and what belongs in Datoviz vs GSP adapter.
5. WebGPU-compatible query profile policy.
6. How far Datoviz v0.4 should go before RC1 vs defer to v0.5.

---

## 9. Minimal GSP-ready checklist for Datoviz v0.4 pre-RC

Before declaring Datoviz v0.4 GSP-ready, verify:

```text
[ ] Public C API exposes or can fill DvzCapabilitySnapshot from a live runtime/view/app.
[ ] Public C API supports scene -> figure -> panel -> view -> render-once flow.
[ ] Public C API supports offscreen PNG capture or documented equivalent.
[ ] Public C API supports point visual creation and dense attribute upload.
[ ] Public C API supports image/sampled-field visual creation or has a clear first-slice path.
[ ] Public C API supports range updates or dirty subrange updates for visual data/resources.
[ ] Public C API supports scene-owned buffers and visual attribute buffer binding where needed.
[ ] Public C API supports panel domains/view policy needed for GSP DataSpace mapping.
[ ] Public C API supports at least panzoom and one 3D camera/controller path if available.
[ ] Public C API supports panel query queue/process/poll or equivalent flow.
[ ] Query result maps back to scene-visible visual/item identity.
[ ] Query statuses distinguish miss/outside/unsupported/stale/GPU failure/readback failure/decode failure.
[ ] Point-over-image query example exists or exact blocker is documented.
[ ] Capability/adaptation diagnostics are scene-visible and deterministic.
[ ] No GSP path depends on private Python classes or Datoviz v0.3 API.
[ ] No GSP path requires JSON/base64 for local data transfer.
[ ] WebGPU/WASM constraints are not broken by new query/capability/format assumptions.
[ ] ctypes smoke script can exercise capabilities and at least one visual path, or blockers are listed.
```

---

## 10. First recommended Datoviz mission

Start with this mission before changing large implementation areas.

### Mission DVT-GSP-M001 — Datoviz GSP readiness audit

**Goal:** produce a precise, code-backed audit of what is already ready, what is missing, and what must change before the GSP adapter can target Datoviz v0.4.

**Read:**

```text
spec/scene/README.md
spec/scene/AUTHORITY.md
spec/scene/core/RUNTIME_BOUNDARY.md
spec/scene/pipeline/FRAME_PLAN.md
spec/scene/pipeline/TRANSFORM_PIPELINE.md
spec/scene/validation/ADAPTATION.md
spec/scene/interaction/PANEL_QUERY.md
spec/scene/interaction/GPU_QUERY_SYSTEM.md
spec/drp2/CAPABILITIES.md
include/datoviz/scene.h
include/datoviz/scene/types.h
include/datoviz/scene/interaction.h
examples/c/
testing/
src/scene/
```

**Do not edit implementation source in this mission.**

**Deliverables:**

```text
spec/scene/proposals/active/GSP_COMPATIBILITY_AUDIT.md
spec/scene/proposals/active/GSP_API_GAP_*.md, if needed
updated version of this document, if obvious corrections are found
```

**Audit table:**

```text
Capability | Existing public API | Existing tests/examples | Gap | Proposed task
```

**Acceptance:**

1. Audit covers capabilities, resources, visuals, transforms, queries, offscreen capture, ctypes, and WebGPU constraints.
2. Every gap has a proposed task and acceptance test.
3. No implementation changes are made.
4. The next implementation mission is clearly identified.

---

## 11. First implementation mission after the audit

The first implementation mission should usually be:

```text
DVT-GSP-001 — Public runtime capability query
```

Reason: GSP planning and adaptation must start from runtime capabilities. Without this, every later adapter decision becomes ad hoc.

---

## 12. Maintainer guidance

When evaluating Datoviz changes for GSP readiness, prefer changes that:

1. Strengthen existing scene/DRP2 boundaries.
2. Improve public C API clarity.
3. Make ctypes binding easier.
4. Improve capability introspection.
5. Improve query/readback determinism.
6. Add tests/examples that are useful even outside GSP.
7. Preserve WebGPU/WASM portability.

Reject changes that:

1. Add GSP-specific hacks to Datoviz internals.
2. Expose backend handles to scene/GSP-facing code.
3. Add hidden fallback behavior without diagnostics.
4. Reintroduce public pick/probe as a parallel model.
5. Make local rendering depend on JSON/base64 serialization.
6. Tie the scene API to Python-only assumptions.

---

## 13. Summary

Datoviz v0.4 is already architecturally close to what GSP needs. The remaining work is not to embed GSP into Datoviz, but to make sure the public v0.4 C API is complete, stable, explicit, and ctypes-friendly enough for a GSP backend adapter.

The most important Datoviz-side priorities are:

1. Public runtime capability query.
2. Stable scene-visible identity route.
3. Efficient resource/attribute update paths.
4. Reliable unified panel query/readback path.
5. Offscreen render/capture smoke path.
6. Visual-family metadata and query capability clarity.
7. Transform/coordinate mapping audit.
8. WebGPU/WASM-compatible capability and query assumptions.

Once these are in place, GSP can provide the Pythonic high-level API and protocol layer, while Datoviz remains the fast GPU execution backend.
