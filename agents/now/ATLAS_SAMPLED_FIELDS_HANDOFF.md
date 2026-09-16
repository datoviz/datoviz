# Atlas Sampled-Fields Handoff

This note records the release allocation exposed by a high-resolution brain-atlas consumer after RC3 candidate preparation. It is a current handoff, not a claim that deferred work is already implemented or release-proven.

Large-mesh query, mesh-part state, and embedded GUI viewport performance are tracked separately in [INTERACTIVE_MESH_PERFORMANCE_HANDOFF.md](INTERACTIVE_MESH_PERFORMANCE_HANDOFF.md). This note remains authoritative only for sampled-field and slice/volume concerns.

## Current capability

- Sampled fields retain 2D and 3D data in scalar, integer, or RGBA formats and support full replacement, resize, subregion updates, and explicit row/image strides.
- Image visuals support scalar fields with scales as well as color fields, with nearest or linear sampling. Native and live WebGPU paths cover the 2D image and label families.
- Label visuals accept 2D integer categorical fields. Native 3D categorical display is available through the volume family; it is not a live WebGPU volume route.
- Volume visuals support dense native 3D scalar and label fields with composite, MIP, and slice modes, transfer functions, clipping, sampling, bounds, and axis permutation/flip.
- `dvz_view_post()` provides a bounded callback handoff to the view owner thread. Applications may load and decode atlas slices on workers, but must implement their own cancellation, latest-wins coalescing, cache limits, and result lifetime.

## Known gaps

- Scalar images are colorized into an RGBA8 staging texture on the CPU before emission instead of sampling the scalar texture and colormap directly in the image shader.
- The 2D labels lookup is uniform-backed and limited to 64 categories; larger catalogs fall back to hash colors. The volume label path already uses a scalable storage-buffer lookup.
- The labels boundary control is selected-label oriented and its width is a texel-neighborhood radius, not a general screen-space boundary renderer. Mapping-aware ontology boundaries remain an application-level vector overlay.
- DRP2 and the scene layer expose one mip level and one texture layer. There is no scene-level texture array, sparse/bricked residency, out-of-core volume, asynchronous field-transfer queue, or custom scene visual shader.
- Field geometry metadata is retained but does not automatically transform cursor/world coordinates for visuals. Mixed-resolution consumers currently own those transforms.

## Release allocation

| Milestone | Allocation | Exit evidence |
| --- | --- | --- |
| RC3 | Correct capability/threading documentation and add realistic large R16/R16_UINT image/label regression coverage without changing runtime semantics. | Existing API, focused native/WebGPU tests, docs checks, and candidate gates remain green. |
| Optional RC4 | If measurements support the work, keep scalar R8/R16/R32 images native on the GPU and apply scales in GLSL/WGSL; replace the fixed 2D label lookup with the scalable storage-buffer design already used by volumes; settle whether label boundaries mean selected labels only or expose an explicit all/selected mode. | Existing public calls remain valid, GLSL/WGSL and probe semantics agree, large-label and scalar-image benchmarks demonstrate the gain, and the work does not block RC4. |
| Final v0.4.0 | Add no new sampled-field semantics; resolve release feedback, regenerate final documentation/media, and retain only claims supported by exact artifact and platform evidence. | Reproducible final artifact/docs gates and maintainer-approved exceptions for unavailable proof. |
| Early v0.5 | Add binding-local field views/logical crops and a designed asynchronous streaming handoff with completion and back-pressure. Evaluate mipmaps, texture arrays, and bounded multi-region dirty tracking from measurements. | New specification, representative streaming benchmarks, explicit ownership/lifetime rules, and backend coverage. |
| Later v0.5+ | Consider bricked/out-of-core 3D residency, volume ray-hit identity, and live WebGPU volume rendering as separately scoped capabilities. | Device-limit strategy, memory budgets, cache/page policy, native/browser backend design, and end-to-end atlas evidence. |

## Benchmark gates

The initial atlas consumer should use three current 10 um 2D planes rather than upload the complete 1.2-billion-voxel volume. Benchmarks must compare the current CPU-RGBA path with any native scalar image path at representative AP, ML, and DV plane sizes, and compare the fixed/hash label path with a complete Allen lookup. Record CPU preparation, conversion, staging/upload, render submission, frame latency, peak CPU/GPU memory, cache hit rate, and steady-state slice-navigation throughput.

Keep the first mixed-resolution application architecture outside Datoviz: world ML/AP/DV micrometres are authoritative; 10 um anatomy, annotation, and exact vector boundaries drive the three slice panels; the current 50 um field drives dense 3D volume rendering; D070 drives surfaces. A new Datoviz feature is justified only when this path is correct and a benchmark isolates a renderer-level bottleneck.

## Next owner actions

1. Keep RC3 claims limited to validated current routes and add focused large-field regression cases only where they do not broaden semantics.
2. Measure the mixed-resolution consumer before promoting the GPU-native scalar-image or scalable 2D-label lookup candidates into RC4.
3. Keep mipmaps, texture arrays, generalized streaming, logical field views, bricking, and WebGPU volumes out of v0.4 final unless release ownership explicitly changes.
