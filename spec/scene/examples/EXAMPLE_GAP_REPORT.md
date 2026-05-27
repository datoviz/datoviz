# Scene Example Support Gap Report

> **Execution Status**
> - **Status:** `ANALYSIS REPORT`
> - **Updated on:** `2026-05-27`
> - **Scope:** `spec/scene/examples/*.md`
> - **Purpose:** summarize what Datoviz v0.4 still needs in scene, DRP2, app, and renderer/runtime
>   support before all scene worked examples can be implemented.


## Baseline Observed

The current active stack already supports a useful retained scene slice:

1. retained figures, panels, per-panel viewport/scissor, resize synchronization, offscreen/window
   presentation, app frame callbacks, capture, and a DRP2 runtime path through vklite/canvas;
2. retained built-in visuals for `point`, `pixel`, `marker`, `primitive`, `mesh`, `sphere`,
   path/segment, `image`, `volume`, and text/glyph;
3. dense per-item attributes, partial range updates, scene-owned buffers, index buffers for
   primitive/mesh, external vertex-buffer registration, and basic dirty tracking;
4. sampled-field storage for 2D and 3D descriptors, with image and volume consumers;
5. image/volume scalar colormapping, retained scale/colormap/colorbar objects, rendered continuous
   colorbars, semantic text, annotation, and pinned-readout bookkeeping;
6. panzoom, camera, arcball, fly, and turntable controllers feeding panel transforms;
7. DRP2 buffers, textures, shader modules, render/compute pipelines, bind groups, samplers,
   render/compute pass scopes, copies, readbacks, multiple color attachments, depth attachments,
   WBOIT-shaped accumulation/resolve streams, and a native vklite runtime;
8. a narrow GPU-backed scene request path: point/marker/pixel picking and image/volume-slice
   probing/readout coverage, with broader mesh/path/text picking still deferred.

That is enough for the smallest examples to be close, but most examples require broader scene
semantics, visual families, framegraph construction, UI integration, or Python-facing API support.


## Highest-Value Missing Capabilities

1. **Text/axes release integration, labels, legends, and colorbars.**
   The scene now has rendered text, rendered continuous colorbars, rendered categorical legends, an
   axes/grid API example, first-class integer labels, and retained annotation/readout handles. The
   examples still need release-quality text behavior, richer axis/readout integration, shared
   layout, richer legend/colorbar composition, label GPU probing, and data/world annotation
   placement. This blocks polished versions of `PATH_AXES_2D`,
   `LINKED_PANELS_AXES_PANZOOM`, `API_SCALE_COLORBAR_ANNOTATION`,
   `LINKED_PANELS_PROBE_COLORBAR`, `MARKET_MICROSTRUCTURE`, and all polished gallery examples.

2. **Visual family expansion and polish.**
   Implemented families now include `marker`, `sphere`, and `volume`, but gaps remain for
   grouped/ragged 3D paths, tubes/ribbons, vector glyphs/arrows, splats, bars/candles,
   cubemap/skybox, fullscreen custom visuals, and postprocess/composite visuals. Retained textured
   mesh is a v0.4 release blocker for the terrain/planet showcase slice: it needs UV attributes,
   mesh-bound sampled textures, a texture color-mode shader variant, lighting/material integration,
   sampler defaults, retained texture updates, fixture coverage, and a deterministic C example.
   Existing families still need richer picking identities, categorical workflows beyond the first
   labels slice, and large-data policies. Splats can be a v0.4 experimental showcase if a retained
   visual lands soon, but they should not become a feature-freeze blocker.

3. **General scene framegraph and per-panel virtual resources.**
   DRP2 can express multi-pass work, compute, render targets, copies, and WBOIT-like passes, but the
   scene layer still emits a small set of built-in pass shapes. The examples need named virtual
   resources, dependencies, pass roles, offscreen intermediate targets, fullscreen/postprocess
   passes, compute-to-render links, ping-pong resources, and per-panel framegraph configuration.

4. **Custom shader/material API at the scene level.**
   Mandelbrot, Gray-Scott, particles, SSAO, volume raymarching, wind projections, skyboxes, and
   advanced glyph/text rendering all need user-supplied shaders, uniform/storage resources, shader
   variants, and material parameters without bypassing scene semantics.

5. **Compute as a first-class scene feature.**
   DRP2 has compute commands and storage-buffer validation, but scene examples need persistent
   compute nodes with resource dependencies, ping-pong buffer/texture binding, per-frame dispatch,
   storage textures, compute-written buffers consumed by render passes, and robust barriers/layout
   transitions expressed at the scene/framegraph level.

6. **Volumetric rendering and 3D texture sampling through scene visuals.**
   `SampledField` and `volume` visuals now cover the first 3D field rendering paths, including
   slice/MIP/composite modes and basic slice probe/readout behavior. Remaining gaps include richer
   transfer functions, arbitrary MPR workflows, categorical label volumes, DVR/MIP picking,
   isosurfaces, bricking/out-of-core policies, and WebGPU parity.

7. **Richer picking, probing, selection, and linking.**
   Current request execution covers broad item picking for point-like, stroke, primitive, image,
   mesh, sphere, and volume proxy targets, plus image pixel and labels segment probe payloads. The
   examples still need exact marker/path semantics, mesh face/region identity, labels probe
   pressure tests, streamline, heatmap-cell, DVR/MIP volume ray-hit, and annotation/text targets; stable
   visual/item/group identities; link-key propagation; selection-driven styling; multi-panel hover
   routing; and CPU/GPU fallback policies.

8. **Linked controllers, scales, and transforms.**
   Basic panzoom/arcball works, but the examples need shared controllers by dimension, independent
   axes per dimension, linked crosshair/probe state, shared scale identity, nonlinear geographic
   transform chains, vector-Jacobian semantics, and panel-local annotation derivation.

9. **Streaming and large-data resource policies.**
   Several examples rely on ring buffers, visible-range LOD, append/subrange updates, non-recreated
   visuals, constant attributes, per-span/per-group attributes, sparse updates, and GPU/CPU resource
   reuse across long live loops. Some metadata exists, but rendering paths still largely assume dense
   per-item data.

10. **Example/runtime infrastructure outside the renderer.**
    The gallery-scale examples need Python bindings, cache/download helpers, ImGui controls,
    screenshot/video export loops, deterministic offline clocks, bundled datasets/assets/fonts, and
    robust fallback paths. Some app/video pieces exist, but the integrated scene example layer is not
    there yet.

11. **Runtime/export variants and engine diagnostics.**
    Native multi-window/fullscreen/HiDPI examples, high-resolution capture, transparent-background
    export, batch/server-side rendering, camera bookmarks, explicit coordinate-space demos,
    visual diagnostics, and remote/cloud/thin-client workflows remain mostly planning topics. Keep
    v0.4 focused on native window, offscreen PNG, bounded video where available, WebGPU subset,
    DRP2/DVZR fixtures, and raw `ctypes` smoke.


## Example-by-Example Status

| Example | Current support | Missing features before full support |
| --- | --- | --- |
| `POINT_2D.md` | Mostly supported. Retained point visual, panel, panzoom, uploads, render pass, and app path exist. | Public data-coordinate normalization/range API, axis-free minimal example polish, and fixture/example coverage that this exact path is stable. |
| `PATH_AXES_2D.md` | Path-as-line-strip and path-native stroked rendering, caps, joins, subpaths, panzoom, semantic axes, generated linear ticks, and rendered axis/tick labels exist. | Grouped/ragged path semantics beyond explicit subpaths, richer axis layout/formatter/clipping policy, release proof for this exact example, dashes, and path/subpath picking. |
| `MARKER_PICKING.md` | Public marker constructor/API, shape subset, code-SDF shader, item identity, bounding-box GPU pick path, and point/marker selection mask highlighting exist. | Exact SDF picking, richer marker style payloads, and polished selection/highlight routing. |
| `SPHERE_IMPOSTOR.md` | First-class `sphere` visual, analytic impostor shader, radius support, lighting/depth, SSAO/G-buffer coverage, item picking, and example coverage exist. | Texture variants and per-item material/PBR. |
| `VOLUME_SLICE.md` | Public `volume` visual, 3D sampled-field binding, slice/composite/MIP modes, bounds, clipping, colormap/transfer parameters, proxy item picking, and basic slice probe/readout exist. | Arbitrary MPR semantics, richer transfer functions, categorical volumes, and DVR/MIP ray-hit picking beyond the first slice. |
| `VOLUME_OFFSCREEN.md` | Offscreen/app capture, volume rendering, and DRP2 readback paths exist. | Deterministic gallery/export conventions and richer transfer/raycast controls. |
| `LINKED_PANELS_PROBE_COLORBAR.md` | Multi-panel rendering, shared image fields, image probe, scales, rendered continuous colorbar, annotations, and pinned-readout bookkeeping exist. | Consolidated scene-level colorbar layout, linked-panel controller semantics, shared mapping identity enforcement, rendered crosshairs, and richer multi-panel probe routing. |
| `MOUSE_BRAIN_ATLAS_EXPLORER.md` | Mesh, WBOIT, image probe, selection bookkeeping, and arcball are partial building blocks. | Volume slice visual, region/group identity in mesh batches, mesh/region picking, selection-driven per-region opacity/highlight, linked 2D panel updates, rendered annotations, and UI tree/filter integration. |
| `LINKED_PANELS_AXES_PANZOOM.md` | Multiple panels, panzoom, path, point/pixel-like rendering, axes, generated ticks, rendered labels, covered-domain caching, and scoped axis invalidation exist. | Shared X-only controller binding, independent Y controller binding, release proof for linked axes behavior, and richer layout polish. |
| `ANIMATION_VIDEO_EXPORT.md` | Timer animation, offline/realtime scene clocks, frame callbacks, app loop, and video module exist. | Transition animation helpers, camera-path animation, public marker/alpha style animation, deterministic video export loop wired through app/scene, and no-spurious-upload animation dirty tracking for style params. |
| `API_MESH_SELECTION_LINK.md` | Mesh visual, item-level mesh picking, link channels, selection objects, link-key storage, and pick result structs exist. | Mesh face/region identity, resolved mesh parent/child identity, linked highlight propagation into visual styling, and public mesh selection examples. |
| `API_IMAGE_PROBE_PINNED_READOUT.md` | Image probe result and pinned-readout bookkeeping exist. | Rendered pinned readouts, shared formatting realization, stronger semantic payloads for scalar/vector/category probes, and polished public API examples. |
| `API_SCALE_COLORBAR_ANNOTATION.md` | Scale, colormap, categorical entries, rendered continuous colorbar, rendered categorical legend, annotation, label, format descriptors, text backend, and placement structs exist. | Non-label annotations, data-anchored transform integration, richer legend/colorbar layout, and shared layout. |
| `API_SAMPLED_FIELD.md` | `SampledField` covers scalar/color/label 2D/3D descriptors, geometry metadata, full/region updates, image binding, labels binding, and volume binding. | Richer probe payloads, labels probe pressure, 3D labels, and broader non-image consumers. |
| `GALAXY.md` | 3D panel, camera/arcball, point/pixel visuals, alpha modes, and WBOIT path are partial fits. | True marker/point-sprite radial falloff, large-star dataset loader/cache, per-star alpha/size style quality, rotation animation helper, overlay text, and gallery screenshot path. |
| Splat / Gaussian-like point cloud showcase | Point/pixel, transparency techniques, camera, and capture are available building blocks. | First-class retained splat visual, per-item center/radius/color/opacity attributes, selected depth/blend policy, shader/pipeline variant, picking disposition, fixture coverage, and deterministic gallery capture. Full Gaussian-splat asset pipelines remain later. |
| `GLOBAL_WIND_PROJECTIONS.md` | Primitive/path/image can approximate some layers. | Projection-aware transform pipeline, vector glyph/arrow visual, vector Jacobian semantics, coastline/graticule helpers, orthographic globe/projection interaction, optional compute particle overlay, and hover labels. |
| `GRAND_CANYON_FLYOVER.md` | Mesh, depth, camera, arcball, app capture, animation callbacks, sampled image resources, and material/lighting paths are building blocks. | v0.4 blocker: true retained mesh texture binding, UV upload, `color_mode = texture`, sampler defaults, and deterministic terrain screenshot example. Full cache/download bundle handling, camera keyframes, sky/background pass, and polished flyover controls can follow. |
| `GRAY_SCOTT.md` | DRP2 compute can exist below scene. | Scene-level compute nodes with storage textures or ping-pong fields, compute-to-render dependencies, brush/input uniforms, reset/preset controls, and rendering the compute output without direct DRP-only code. |
| `LATEX_MICROTEX_TEXT_VISUAL.md` | Retained text/font/annotation handles and rendered bitmap/SDF glyph text paths exist. | TeX/math rendering, HarfBuzz shaping, richer font fallback chains, paragraph/layout behavior, and polished math-text examples. |
| `MANDELBROT.md` | DRP2 can express custom shaders and fullscreen draws at a low level. | Scene custom fullscreen visual/material API, uniform buffers exposed to scene users, event-driven parameter updates, double-single parameter helpers, progressive refinement, HUD text, and Python binding path. |
| `MARKET_MICROSTRUCTURE.md` | Multi-panel, path/point/image primitives, partial updates, image heatmap, panzoom, and app loop exist. | Bars/candles visual, LOD/visible-range aggregation, axes/text/tooltips, crosshair overlays, linked panels, CPU/GPU picking for trades/heatmap cells, ImGui controls, and streaming replay policy. |
| `DIFFUSION_TRACTOGRAPHY.md` | 3D path can be approximated as one line strip or primitive lines. | Ragged path buffers, per-streamline identity, future `tube` rendering, tube impostors, tessellated tubes/ribbons, direction coloring, streamline picking/highlight, LOD/subsampling, and WBOIT/SSAO integration. |
| `EARTH.md` | 3D camera/arcball, mesh/image primitives, sampled fields, and material/lighting paths are available. | v0.4 can use the retained textured-mesh slice for a planet patch or UV sphere-like surface. Full cubemap/skybox visual, environment/background transform, globe spin helper, asset cache, and polished depth/layering example remain later. |
| `PARTICLES.md` | Point rendering, dynamic updates, app frame callbacks, and DRP2 compute/storage buffers below scene are available. `spec/scene/proposals/active/PARTICLE_SYSTEM_DESIGN.md` captures the proposed particle-system semantics. | v0.4 stretch can support CPU-side particle or fluid advection by updating point/path/image data each frame. Full GPU particle simulation still needs scene-level compute-written storage buffers consumed as vertex/instance input, persistent buffer reuse, additive/transparent particle shaders, optional trails accumulation framegraph, and per-frame parameter/UI controls. |
| `CFD_VORTICITY_ADVECTION.md` | Image fields, point/path overlays, partial updates, app frame callbacks, and video/capture infrastructure are building blocks. | v0.4 stretch path is CPU-side bounded advection with scalar image and particle/trail overlays. GPU-side fluid simulation needs scene compute nodes, ping-pong storage textures, compute-to-render dependencies, and UI/reset controls. |
| `STREAMING_DAQ_VIEWER.md` | Partial range updates, path/primitive rendering, panzoom, and app loop exist. | Ring-buffer visual semantics, discontinuity handling at wrap, many-trace layout/stacking helpers, cursor/overlays/text, pause/reset controls, and sustained streaming resource policy. |
| `TOY_DICOM_VIEWER.md` | Multi-panel, image, mesh, camera, arcball, sampled-field metadata, and app interaction are partial pieces. | Shared 3D texture resource bound to slices and volume, oriented slice shader, window/level uniforms, crosshair overlays, 3D raymarch volume visual, transfer functions, slice dragging, and UI sliders. |
| `PROTEIN_ARCBALL_VIEWER.md` | Mesh, primitive, depth, arcball, WBOIT, and app path are useful foundations. | Sphere impostor atoms, cylinder/bond or tube visual, ribbon/cartoon mesh helpers, material controls, ImGui controls, asset cache/download, multi-representation visibility groups, SSAO gbuffer/pass/blur/composite framegraph, and rendered labels. |


## Cross-Cutting Work Queue

1. **Add retained textured mesh before final gallery proof.**
   Land the v0.4-required UV attribute path, mesh texture resource binding, texture shader variant,
   sampler defaults, material/lighting integration, fixture, and terrain/planet C showcase. Do not
   count baked vertex colors as satisfying this lane.

2. **Text and explanatory-object proof.**
   Harden the existing rendered text, axes, labels, legends, annotations, colorbars, and scale bars
   through focused examples and screenshot/offscreen proof. This unlocks many 2D examples and
   improves every gallery scene.

3. **Polish the point-like family.**
   Public marker, point, pixel, and sphere first slices exist with item-id support. The next work is
   exact marker hit testing, richer marker style payloads, large-count stress, and selection styling.

4. **Make FramePlan multi-pass resources explicit in scene.**
   Add named virtual textures/buffers, dependencies, pass roles, fullscreen/postprocess passes, and
   per-panel resource lifetimes. This is the common blocker for WBOIT polish, SSAO, compute examples,
   volume rendering, and custom shader demos.

5. **Expose scene-level custom visual/material resources.**
   Let examples define shaders, uniforms, storage buffers/textures, samplers, and fullscreen draws
   without dropping below scene or leaking Vulkan.

6. **Separate CPU-side fluid/particle stretch from GPU compute.**
   A bounded CPU-side fluid or particle example can use existing image/point/path dynamic updates.
   GPU Gray-Scott or particles require persistent ping-pong resources, compute pass, render pass,
   and deterministic dependency ordering at the scene level.

7. **Allow one experimental splat showcase only if the visual lands cleanly.**
   The first slice should be retained splat items with center, radius, color, opacity, a documented
   depth/blending policy, deterministic fixture coverage, and one synthetic or LiDAR-like capture.
   Defer trained Gaussian-splat asset formats, differentiable rendering, out-of-core splat scenes,
   and advanced LOD.

8. **Add volume and 3D sampled-field rendering.**
   Implement `volume` slice first, then raymarch/offscreen export. Keep `SampledField` as the
   source resource and make transfer/window-level/colormap parameters explicit.

9. **Widen picking/probing payloads.**
   Add marker, mesh face/region, path/streamline, heatmap cell, and volume-slice targets with stable
   ids, link-key resolution, stale-result handling, and CPU fallback hooks where GPU picking is not
   ready.

10. **Add linked-scene interaction primitives.**
   Shared controllers by dimension, linked crosshair/probe state, scale identity, panel-local
   annotation derivation, and UI-mutated selection/visibility state should become first-class.

11. **Add large-data and streaming policies.**
   Build explicit ring-buffer, visible-range, LOD/subsampling, constant-attribute, per-group, and
   per-span attribute support into the scene resource model. Later large-data gallery examples
   should also account for density rendering, progressive refinement, tile streaming, GPU
   instancing, and out-of-core policy.

12. **Finish example harness infrastructure.**
    Python bindings, cache/download helpers, ImGui integration, deterministic screenshot/video
    capture, and asset/font bundling are required for the larger examples to run from a clean
    checkout.

13. **Stage runtime/export and diagnostic examples.**
    Add fixture-level coverage for multi-window/fullscreen/HiDPI behavior, high-resolution and
    transparent-background export, batch/server-side capture, camera bookmarks, coordinate spaces,
    and visual diagnostics before promoting any of them to public gallery promises.


## Near-Term Minimal Example Unlock Order

1. `POINT_2D.md`: already close; use it as the zero-regression retained-scene smoke.
2. `PATH_AXES_2D.md` plus `LINKED_PANELS_AXES_PANZOOM.md`: requires axes/text/ticks, and validates
   controller/link semantics without compute or volumes.
3. `MARKER_PICKING.md`: public marker visual plus item-id picking, building directly on current point
   pick execution.
4. `API_IMAGE_PROBE_PINNED_READOUT.md` and `LINKED_PANELS_PROBE_COLORBAR.md`: rendered annotations
   and colorbars on top of the existing image probe path.
5. `VOLUME_SLICE.md`: first real 3D sampled-field consumer; then `TOY_DICOM_VIEWER.md` becomes
   feasible in stages.
6. `GRAND_CANYON_FLYOVER.md` or `EARTH.md`: v0.4-required retained textured mesh proof with UVs,
   mesh-bound texture sampling, lighting, and deterministic capture.
7. Splat / Gaussian-like point cloud showcase: optional v0.4 experimental lane if the visual lands
   cleanly; keep full Gaussian-splat pipelines later.
8. `CFD_VORTICITY_ADVECTION.md`: optional CPU-side dynamic image/particle/path update stretch
   before scene-level GPU compute.
9. `GRAY_SCOTT.md`: first scene-level compute-to-render framegraph, after virtual resources and
   custom material/resource binding exist.
10. `PROTEIN_ARCBALL_VIEWER.md`: keep as the multi-pass renderer pressure test after mesh/material,
   WBOIT, postprocess, text, and UI controls are in better shape.
