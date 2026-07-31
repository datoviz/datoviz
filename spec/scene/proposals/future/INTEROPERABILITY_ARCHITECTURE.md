> **Execution Status**
> - **Status:** `FUTURE ROADMAP`
> - **Updated on:** `2026-07-31`
> - **Purpose:** preserve a post-v0.4 interoperability architecture, priority order, and evidence gates without freezing speculative APIs or providers.
> - **Scope:** external frame and data sources, external render targets, hosted applications, remote and timed sources, semantic scene interchange, optional providers, and constrained presentation platforms.

# Interoperability Architecture


## Position

Datoviz should be usable as a composable scientific visualization component rather than requiring ownership of the entire data, rendering, UI, and presentation stack.

The architectural rule is to separate semantics from transport. Cameras, frame identity, color, depth, item identity, time, progress, and queries are semantic concepts. CPU pointers, shared buffers, Vulkan images, CUDA handles, video packets, shared memory, and network messages are transports.

Interoperability must preserve the active execution path:

```text
scene frame plans -> drp2 command streams -> vklite runtime -> canvas/stream frame execution -> optional app presentation
```

External inputs should become normal Datoviz resources or explicit frame-plan inputs. External outputs should use explicit app, canvas, or runtime targets. Do not create a parallel renderer, scene planner, frame stream, presentation layer, or Vulkan wrapper in core.

This proposal is not a v0.4 release requirement. It records post-v0.4 pressure and must not delay RC3, RC4, or final v0.4.0 work.


## Ownership And Orchestration Boundary

The first interoperability architecture should remain passive and application-orchestrated:

1. Datoviz core owns frame ingestion, validation, presentation, scene-visible lifecycle, and integration with Datoviz queries.
2. An external provider owns rendering, processing, decoding, simulation, or acquisition.
3. The application owns orchestration: when work is requested, which parameters are sent, how domain state is coordinated, and whether stale display is temporarily acceptable.
4. Optional adapters translate between application or provider objects and Datoviz contracts.
5. Datoviz core does not initially discover providers, create renderer threads, load heavy SDKs, or invoke renderer-specific callbacks.

The normal flow is:

```text
application requests external work
        -> provider produces a result
        -> application submits the result to Datoviz
        -> Datoviz validates, accepts or rejects, and requests presentation
```

A provider may be a Python object, native library, helper process, local service, remote service, decoder, instrument, or simulation. The semantic contracts must not presume one deployment form.


## Capability Lanes

The roadmap has eight independent but composable lanes.

| Lane | External system owns | Datoviz owns | Portable first proof |
| --- | --- | --- | --- |
| External frame presentation | Specialized rendering or image production | Presentation, overlays, interaction, linked panels, freshness validation | Copied CPU RGBA8 |
| External targets and overlays | Host world, compositor, UI, or encoder | Transparent scientific rendering and output metadata | CPU RGBA readback |
| Array and geometry adapters | Loading, processing, simulation, meshing, domain identity | Visual resources, selection, probing, rendering | Copied contiguous arrays |
| Hosted event loops and UI | Window, input, scheduling, application lifecycle | Scene and render-once execution | Existing hosted-surface or offscreen contract |
| Remote, media, and in-situ sources | Simulation, server, decoder, instrument, acquisition | Local interaction, analysis, presentation, steering surface | CPU or compressed frames and copied data |
| Semantic scene interchange | Final renderer or downstream scene graph | Scene authoring and stable semantic state | Immutable snapshot or visitor |
| Depth, identity, and query integration | Specialized geometry or volume hit semantics | User-facing query, selection, reconstruction, and linked views | CPU depth and ID planes or provider-assisted query |
| Provider transports and conformance | Optional SDK and platform integration | Version, capability, lifecycle, diagnostics, and conformance policy | Recipes and source-built adapters before an SDK |

These are not a single linear implementation sequence. Array adapters, transparent output, semantic export, and remote sources may advance independently after their shared contracts are proven.


## Cross-Cutting Contracts

Every lane should reuse the smallest applicable set of cross-cutting contracts.

### Camera And Coordinates

External work may require the exact realized camera, panel viewport, and coordinate conventions. A future snapshot should reuse or derive from existing `DvzCameraView`, `DvzCameraProjection`, and MVP state rather than creating competing authoritative representations.

The contract must eventually cover matrix layout, handedness, view direction, up axis, projection type, near and far planes, ordinary or reversed Z, clip-depth convention, viewport origin, pixel centers, logical and physical pixels, local origin, units, and optional coordinate-reference metadata.

Camera authority has three possible modes: Datoviz authoritative, host authoritative, or explicitly mirrored. Adapters must prevent feedback loops when both systems observe changes.

### Request, Revision, Target Generation, And Time

Asynchronous results need at least request identity, camera revision, scene revision, target generation, and dimensions. Resize or target recreation increments the target generation independently of camera state.

Progressive results may replace earlier results only within the same request, camera, scene, and target identity. Cancellation is advisory; stale-result rejection is authoritative.

The model should leave room for logical simulation time, source timestamp, presentation timestamp, playback or session generation, and predicted presentation time without requiring them in the first MVP.

### Frame Semantics, Payload Lease, And Transport

Keep three conceptual layers distinct:

1. Frame semantics describe dimensions, planes, formats, color role, origin, alpha, camera identity, revisions, time, progress, and accumulation.
2. A payload lease describes availability, immutability, acquisition, readiness, consumption, release, and generation invalidation.
3. A transport describes copied CPU memory, borrowed CPU memory, shared buffer, shared image, video packet, shared memory, or remote message.

An untyped pointer that changes meaning according to a memory-kind enum is not a sufficient long-term ownership contract.

### Color, Alpha, Depth, And Planes

The portable first frame is tightly packed RGBA8 with an explicit `srgb_color` or `linear_color` role, image origin, row pitch, and alpha mode. Standard Datoviz display and capture remain governed by [color management](../../semantics/COLOR_MANAGEMENT.md).

Future synchronized planes may include color, opacity or coverage, depth, object ID, primitive ID, instance ID, normal, albedo, motion, variance, uncertainty, and validity masks. All planes in a bundle must describe the same request, camera, scene, target generation, and accumulation state.

Depth integration requires an exact encoding and projection contract before implementation. Format alone is insufficient.

### Identity, Selection, And Queries

Stable upstream identity should map through Datoviz visual and item identity to rendered ID planes or provider-assisted hits:

```text
external domain identity <-> Datoviz visual/item identity <-> rendered identity
```

Mappings need revisioning, missing-object behavior, grouping, and support for domain identities such as VTK cells, simulation particles, molecule atoms or residues, segmented objects, USD prims, and application entities.

Datoviz queries may resolve through image channels or asynchronous provider-assisted ray queries. Both routes should produce scene-visible `DvzQueryResult` semantics and reject stale results.

### Scheduling, Backpressure, And Diagnostics

Every asynchronous adapter must declare queue capacity, latest-only or FIFO behavior, supersession, producer throttling, hidden-view behavior, interactive versus idle quality, and teardown with outstanding work.

Diagnostics should expose requested, started, completed, accepted, discarded, and failed counts; discard reasons; queue depth; request-to-result and result-to-presentation latency; active generations; provider state; and last error. These may begin as debug diagnostics rather than public API.

### Threading And Failure

Scene mutation and GPU submission remain render-thread responsibilities. Providers may work in background threads or processes, but results must cross an explicit safe handoff. Python adapters must not rely on a C-owned worker invoking arbitrary Python code on a render thread.

Provider failure must not corrupt the Datoviz view. The application should be able to retain the last valid frame, show diagnostics, recover or reconnect, and destroy the view without waiting indefinitely for external work.


## Ecosystem Orientation

Inclusion below indicates architectural relevance only. It does not imply implementation, packaging, testing, endorsement, or support.

| Category | Representative systems | Likely relationship |
| --- | --- | --- |
| Renderer abstraction | ANARI, OpenUSD Hydra | External renderer API, renderer switching, semantic scene consumer |
| CPU and ray-tracing renderers | OSPRay, Blender Cycles, Embree-backed workflows | External color, depth, and ID frames; final rendering |
| GPU and specialized renderers | OptiX, VisRTX, NVIDIA IndeX, custom CUDA renderers | GPU-produced frames, specialized volumes, provider-assisted queries |
| Differentiable and neural rendering | Mitsuba 3, Dr.Jit, NeRF systems, 3D Gaussian Splatting systems | Frames, residuals, gradients, uncertainty, optimization state |
| Scientific visualization and processing | VTK, ParaView, PyVista, yt | Data and geometry adapters, volume rendering, external frames, selection round trips |
| Geometry and point-cloud processing | Open3D, PDAL, CloudCompare, CGAL | Meshes, point clouds, derived attributes, stable source identities |
| CPU arrays and distributed computation | NumPy, xarray, Dask | Copied arrays, chunked data, time-dependent fields |
| GPU computation and arrays | CUDA, CuPy, PyTorch, Taichi, Numba CUDA, JAX, TensorFlow | Existing external buffers and future optional transports through explicit framework policy |
| Array interchange | Python buffer protocol, DLPack, CUDA Array Interface | Transport below semantic array contracts |
| Medical and imaging | 3D Slicer, VTK medical pipelines, ITK, SimpleITK, OpenCV, scikit-image, PyTorch segmentation | Volumes, slices, segmentations, measurements, uncertainty |
| Microscopy | napari, OME-Zarr, Dask, tracking and segmentation systems | N-D processing, volumes, slices, tracks, lineage data |
| Molecular analysis | MDAnalysis, MDTraj, VMD-derived workflows, ChimeraX-exported geometry | Molecular geometry, trajectory time, atom and residue identity |
| Game engines | Unity, Unreal Engine, Godot | Datoviz overlays, engine frames in Datoviz, telemetry, engine-specific texture adapters |
| Robotics and simulation | ROS, NVIDIA Isaac Sim, Omniverse, custom simulators | Sensor frames, point clouds, trajectories, replay, steering |
| Desktop hosts | Qt, PyQt, PySide, SDL, Tk, wx, Dear ImGui, native C and C++ applications | Host-owned windows, input, scheduling, and application lifecycle |
| Interactive Python and browser hosts | Python console, IPython, Jupyter, WebGPU, WASM | Native hosted views, notebook streams, widgets, browser execution or presentation |
| Scene and DCC systems | Blender, OpenUSD, glTF, Houdini-like procedural workflows | Semantic interchange, final rendering, asset pipelines |
| In-situ and scientific transport | ParaView Catalyst, Ascent, SENSEI, ADIOS2, HDF5, Zarr, Apache Arrow | Simulation integration, streaming data, process boundaries |
| Sparse volume and geospatial processing | OpenVDB, GDAL, Rasterio, PROJ, GeoPandas | Sparse volumes, terrain, hazards, remote sensing, CRS adaptation |
| Remote, media, and acquisition | Render servers, HPC simulations, FFmpeg, GStreamer, WebRTC, NVDEC, cameras, microscopes, telescopes | Timed frames, compressed transport, acquisition, steering, provenance |
| Graphics APIs | Vulkan, D3D12, Metal, OpenGL, WebGPU | Adapter-specific transport and execution details below semantics |

Initial reference candidates remain deliberately narrow: a deterministic fake renderer, NumPy CPU frames, one VTK or PyVista data recipe, one ANARI or OSPRay CPU frame source, a minimal transparent compositor, and later reuse of the existing CUDA image-buffer path.


## Application Integration Profiles

### Unity

The first Datoviz-in-Unity route is a transparent CPU RGBA texture rendered offscreen by Datoviz and composited by Unity. Unity owns its world, camera, window, input, and render loop; an adapter forwards camera, viewport, time, telemetry, and pointer events.

The reverse route uses a Unity camera render texture and asynchronous readback as a CPU frame source for a Datoviz analysis application. A later native plugin may exchange backend-specific textures, but Unity may use D3D12, Metal, OpenGL, or Vulkan, so this is a family of adapters rather than one GPU feature.

### Unreal Engine

The first Datoviz-in-Unreal route is a transparent CPU RGBA texture consumed by UMG, Slate, a material, or a postprocess. The reverse route uses scene capture and RHI texture readback as a CPU frame source.

An RHI-specific native resource adapter is possible later, but Unreal render-thread, RHI-thread, render-graph, and resource-lifetime rules remain authoritative. Datoviz must not record into or transition Unreal-owned resources without an explicit narrow contract.

### Blender

The preferred Datoviz-to-Blender route is semantic interchange through glTF, OpenUSD, or a generated Blender script, followed by Cycles or Eevee rendering. This depends on a stable Datoviz semantic snapshot rather than a frame transport.

Blender may also run as an out-of-process progressive frame provider. A Blender add-on can display Datoviz CPU frames or control a separate Datoviz view, but loading an independent Vulkan runtime into Blender's graphics lifecycle is not an initial target.

### Native Vulkan And Engineering Hosts

The current hosted-surface path lets an external UI own the event loop and surface while Datoviz owns its rendering infrastructure. Deeper integration through shared Vulkan images and semaphores is compatible with current ownership direction. Operating directly on arbitrary host-owned devices, queues, command buffers, or render graphs would require a distinct explicit ownership contract.


## Deployment And Presentation Pressure

These platforms are architectural pressure tests, not scheduled support commitments.

| Platform profile | Preferred first route | Architectural pressure | Disposition |
| --- | --- | --- | --- |
| PC or Android OpenXR | Datoviz panel as a host-owned textured quad or cylinder layer | Stereo or multiview identity, predicted display time, pose and ray input, runtime-owned swapchains | After external targets exist |
| Full native XR rendering | Datoviz renders per-eye projection views | Late pose updates, deadlines, reprojection, depth submission | Long-term |
| Apple Vision Pro | Unity, RealityKit, or Metal host consumes Datoviz frames | Metal backend, per-eye textures, spatial lifecycle | No native commitment |
| Android tablets | Native Vulkan application or browser/thin client | Android surface lifecycle, touch, orientation, mobile capabilities, memory and thermal budgets | Plausible post-desktop target |
| iPad | Browser/thin client, Unity host, or CPU texture bridge | Metal backend, UIKit or SwiftUI hosting, touch lifecycle | Native path deferred |
| Android TV | Native Android client or streamed/browser view | D-pad and gamepad focus, TV lifecycle, constrained GPU and memory | Demand-driven |
| Apple TV | Streamed client or Metal host | Metal and tvOS hosting | Demand-driven |
| webOS, Tizen, and similar TVs | Browser or streamed-video client | Browser variation, remote navigation, media transport | Thin-client profile |
| Xbox, PlayStation, Nintendo | Unity or Unreal adapter, remote client, or licensed third-party provider | Proprietary or non-Vulkan graphics APIs, gated SDKs, certification | No native core commitment |
| Linux and Vulkan handhelds | Native Datoviz application | Controller input, suspend and resume, constrained performance | Opportunistic |
| Wall displays and control rooms | Desktop host or remote thin client | Multi-display layout, unattended recovery, remote control | Practical application profile |

The request model should therefore leave room for multiple related views, predicted presentation time, touch, stylus, gamepad, controller rays, gaze, hands, remote focus navigation, independent surface generations, negotiated quality, and thin-client operation.


## Prioritized Milestones

### M0: Preserve The Architecture

Record this capability map, ownership boundary, priorities, non-goals, promotion rules, and pressure tests. Make no implementation or public API commitment.

### M1: Composability Proofs

Build three small proofs in order: a deterministic asynchronous CPU frame presenter, a copied-array external-processing recipe, and a transparent CPU overlay compositor.

The fake renderer deliberately returns frames out of order and exercises resize, queue pressure, provider failure, closing, and repeated lifecycle. The data recipe should use NumPy and one real processing ecosystem such as VTK or PyVista. The compositor should prove exact sRGB and alpha behavior.

### M2: External-View MVP

Provide a reusable cross-platform CPU workflow with tightly packed RGBA8, explicit color and alpha semantics, exact camera and viewport snapshots, request and target generations, latest-only or bounded scheduling, progressive replacement, copy-on-submit ownership, diagnostics, resize, hiding, recreation, and teardown.

At the end, decide among recipe only, experimental Python helper, or a small public C and Python contract. No new API is a valid successful outcome.

### M3: Identity And Scientific Integration

Add stable upstream identity mapping, depth and ID planes, world-position reconstruction, asynchronous provider-assisted queries, stale-query rejection, selection round trips, linked views, and logical time identity.

### M4: Performance Transports

Reuse the current CUDA linear image-buffer route under the established semantics, then consider explicit leases, same-device validation, shared Vulkan images, synchronized multi-plane bundles, float color, and auxiliary planes. A requested zero-copy mode must fail explicitly rather than silently copy.

### M5: External Targets And Media

Promote transparent output if the compositor proof justifies it, starting with CPU readback and later adding GPU-visible targets, optional depth or IDs, timestamps, video encoding, multiview, and stereo where required.

### M6: Remote And In-Situ Operation

Add clock domains, compressed frames, shared memory, frame dropping, jitter policy, reconnection, remote queries, steering, provenance, quotas, and process isolation while reusing the established frame and query semantics.

### M7: Semantic Scene Interchange

Design an immutable semantic snapshot or visitor for cameras, transforms, geometry, fields, materials, identities, and time-varying values. Select glTF, OpenUSD, ANARI, or Blender exporters from demonstrated user demand and keep this contract distinct from DRP2 execution artifacts.

### M8: Provider Ecosystem

Only after independent providers exist, define compatibility checks, capability negotiation, packaging, discovery, device and threading contracts, diagnostics, in-process and out-of-process profiles, and a conformance suite.


## Prerequisite Implementation Audit

Before M1 code, audit actual implementation rather than relying on aspirational specification text:

1. Detect camera changes and obtain the exact realized camera and projection.
2. Obtain panel physical-pixel viewport, effective rectangle, and scale.
3. Post work safely from a provider worker to the render or UI thread.
4. Request host scheduling safely from another thread.
5. Update an image visual without unsafe scene mutation.
6. Apply related state atomically where one result updates several fields.
7. Observe resize and runtime resource generations.
8. Capture transparent offscreen output with correct straight or premultiplied alpha.
9. Observe view visibility, suspension, surface loss, and destruction.
10. Propagate provider errors without damaging normal scene rendering.

The audit may recommend small generally useful camera, viewport, scheduling, or lifecycle primitives before any external-view abstraction.


## Maturity And Promotion

Each lane follows the same maturity ladder:

```text
architecture pressure -> local proof -> optional recipe -> experimental helper -> public contract -> supported provider ecosystem
```

Promotion requires:

1. At least two independent consumers or providers exercising the shared concept.
2. Deterministic semantic tests that do not require specialized hardware.
3. Explicit ownership, thread, resize, recreation, and teardown rules.
4. Diagnostics for unavailable or failed capabilities.
5. A credible cross-platform CPU baseline where the capability is not inherently platform-specific.
6. Installed-package proof rather than source-tree-only execution.
7. Evidence that existing APIs cannot express the workflow cleanly.
8. No parallel renderer, scene planner, frame stream, presentation layer, or backend-shaped scene escape hatch.


## Maintenance And Distribution

Keep the deterministic fake provider and portable NumPy transport in-tree. Maintain at most one real external renderer and one external-processing adapter in-tree until demand and continuous validation justify more.

Large domain SDKs, game engines, proprietary systems, and weakly testable integrations should begin as recipes, companion packages, or external adapters. Core import and startup must remain independent of every optional SDK.

Promoted integrations require clean installation, optional-dependency absence and presence tests, repeated lifecycle proof, explicit provider diagnostics, license and redistribution review, and a maintained validation environment.


## Stop Conditions

1. Do not add a public external-view API if recipes remain concise.
2. Do not generalize GPU planes until multiple plane types or providers demonstrate the same need.
3. Do not add provider discovery before independently distributed providers exist.
4. Do not add depth composition without a compelling integrated-query or overlay use case.
5. Do not maintain an adapter whose dependency cannot be tested regularly.
6. Do not move domain coordination, dashboard orchestration, or high-level plotting into core.
7. Do not promise native mobile, XR, TV, or console support from architectural compatibility alone.
8. Do not make this roadmap a v0.4 release blocker.


## Initial Success Criteria

The first interoperability cycle succeeds when:

1. A deterministic fake and one real provider use the same CPU request and frame semantics.
2. Late or incorrectly sized frames are rejected deterministically.
3. A Datoviz camera can drive an external renderer without convention ambiguity.
4. Datoviz overlays and a linked panel remain responsive during asynchronous work.
5. Resize, close, recreation, failure, and repeated lifecycle leave no worker or borrowed payload alive.
6. One external-processing recipe preserves stable upstream identity through selection.
7. One transparent output proof composites correctly in a host.
8. The evidence supports either a deliberately small API or an explicit decision to retain recipes only.


## Related Specifications

1. [Runtime boundary](../../core/RUNTIME_BOUNDARY.md)
2. [Integration entry points](../../integration/ENTRY_POINTS.md)
3. [Hosted backends](../../integration/HOSTED_BACKENDS.md)
4. [External UI](../../integration/EXTERNAL_UI.md)
5. [Thread safety and async handoff](../../integration/THREAD_SAFETY.md)
6. [Optional providers](../../integration/OPTIONAL_PROVIDERS.md)
7. [GPU array interoperability](../../integration/GPU_ARRAY_INTEROP.md)
8. [GPU image interoperability plan](../../integration/GPU_IMAGE_INTEROP_PLAN.md)
9. [Timed media synchronization](../../integration/TIMED_MEDIA_SYNC.md)
10. [Image export](../../export/IMAGE_EXPORT.md)
11. [Color management](../../semantics/COLOR_MANAGEMENT.md)
12. [Python NumPy adaptation](../../../bindings/ARRAY_FACADE.md)
