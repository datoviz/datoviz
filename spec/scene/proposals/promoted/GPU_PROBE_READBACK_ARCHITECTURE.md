# GPU Probe And Readback Architecture

This proposal records the longer-term architecture for GPU-side scene probes. It complements the
first labels readback slice, which preserves raw integer label IDs but currently copies the full
labels texture before downloading only the selected texel.


## Problem

Scene probes need semantic answers, not just rendered colors. For example, a labels visual renders
an integer sampled field through palette, opacity, selection, hidden-label, and boundary logic. The
displayed RGBA pixel is therefore not a reliable representation of the raw label ID.

The same issue appears in other visual families. Image probes may need scalar or categorical values,
point and marker picks need item identity, and volume probes may need a field value plus a position
inside the volume. These payloads should not be reconstructed from presentation colors when the
semantic data is still available to the GPU.


## Current State

The first 2D labels slice in `src/scene/request_execute.c` maps a panel request to visual UVs,
converts the UVs to an integer labels texel, creates a temporary integer texture from the retained
field data, copies that full texture into a readback buffer, and downloads only the selected sample.

This path is correct for raw IDs. It preserves signed values such as `-7` and high unsigned values
such as `4000000000`, which the older hidden-RGBA image compatibility route could not represent
reliably. Its performance limitation is that a large labels field can require a large texture copy
for a single hover or click result.


## Goals

1. Preserve raw semantic payloads for probes and picks.
2. Reuse the visual resources already realized by the runtime when possible.
3. Support labels, image, point, marker, mesh, and volume probe families through shared machinery.
4. Allow both render-pass and compute implementations.
5. Keep the CPU-facing `DvzProbeResult` API stable across implementation tiers.
6. Batch multiple probe requests when that is more efficient than one GPU submission per request.


## Non-Goals

1. Do not redesign the public probe API as part of the first architecture pass.
2. Do not require compute shaders for all backends.
3. Do not replace every existing pick path before labels and image probes have a narrow validated
   migration path.
4. Do not expose rendered colors as semantic labels, categories, or item identities.


## Architecture

The generic flow should be:

```text
scene probe request
-> probe planner resolves panel, visual order, camera, and target metadata
-> GPU probe pass samples or intersects visual resources
-> GPU writes a small typed payload
-> runtime copies or maps that payload for CPU readback
-> scene converts the payload into DvzProbeResult
```

The generic layer owns request batching, payload layout, runtime submission, readback, freshness, and
fallback selection. Each visual family owns the shader logic that interprets its resources.


## Probe Payload ABI

Define a C/GLSL/WGSL-compatible GPU payload structure with explicit integer widths and stable
alignment. A first version can be deliberately small:

```c
typedef struct DvzGpuProbePayload
{
    uint32_t status;
    uint32_t visual_family;
    uint64_t visual_id;
    uint32_t target;
    uint32_t value_kind;
    int64_t category_id;
    uint64_t item_id;
    float uvw[3];
    float depth;
} DvzGpuProbePayload;
```

The payload should contain raw facts. Scene policy remains CPU-side: background-label misses, scale
label lookup, stale request handling, public ID normalization, and result formatting.


## Probe Request ABI

GPU probe shaders need a compact request input. The exact fields may grow by family, but a shared
header should include request identity, panel-local coordinates, normalized device coordinates, and
optional ray data:

```c
typedef struct DvzGpuProbeRequest
{
    uint64_t request_id;
    uint32_t target;
    uint32_t reserved;
    float panel_xy[2];
    float ndc_xy[2];
    float ray_origin[3];
    float ray_dir[3];
} DvzGpuProbeRequest;
```

For 2D labels and images, `ndc_xy` plus visual geometry may be enough. For mesh and volume probing,
the planner should provide a ray in the same coordinate convention used by the visual family.


## Shader Strategies

### Render-Pass Probe

A render-pass probe uses a tiny offscreen integer or structured render target, typically `1x1` for a
single request. The normal visual geometry and panel transform are reused, but a family-specific
probe fragment shader writes a raw payload or integer ID instead of display color.

For labels, the fragment shader can fetch from the integer labels texture and write `R32_SINT` or
`R32_UINT`:

```glsl
layout(binding = 0) uniform isampler2D labels_tex;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out int out_label_id;

void main()
{
    ivec2 size = textureSize(labels_tex, 0);
    ivec2 p = clamp(ivec2(floor(in_uv * vec2(size))), ivec2(0), size - ivec2(1));
    out_label_id = texelFetch(labels_tex, p, 0).r;
}
```

This path does not require compute support. It is also a good match for "what is visible at this
pixel" semantics because render order, clipping, depth, viewport, and visual transforms are already
render concepts.

### Compute Probe

A compute probe dispatches one or more invocations over a request buffer and writes one payload per
request into a storage buffer. For labels, the compute shader can fetch a single texel and write the
raw category ID directly.

This path is more direct for resource queries and easier to batch, but it requires mature compute
pipeline, storage buffer, binding, and synchronization support in the active runtime backend.


## Runtime Resource Requirements

A clean GPU-side probe should bind the same realized resources that rendering uses. The scene runtime
needs stable lookups from semantic resources to DRP2 runtime IDs:

```text
visual sampled field -> texture id / texture view id / sampler id
visual vertex attributes -> buffer ids and byte ranges
visual params/uniforms -> buffer ids and byte ranges
panel camera state -> uniform buffer id
draw or visual instance -> public visual id and family metadata
```

These mappings must survive resource recreation, descriptor refresh, frame-to-frame reuse, resize,
and visual updates. If a probe path creates independent temporary resources for every request, it is
a fallback rather than the final architecture.


## Fallback Tiers

The public result should stay stable while the implementation chooses the best available tier:

1. CPU lookup when retained CPU data is valid and the visual mapping is simple.
2. One-texel texture copy when the exact texel coordinate is known.
3. Render-pass probe into a small integer or payload target.
4. Compute probe into a storage/readback buffer.
5. Deterministic miss or unsupported status when no valid tier is available.

The first labels hardening step should be tier 2: copy only the selected texel instead of the full
labels texture. The generic architecture should make tiers 3 and 4 available without changing
`DvzProbeResult`.


## Labels Migration Plan

1. Extend DRP2/vklite texture-to-buffer copies with a source origin so labels probes can copy a
   `1x1` texel region into a tiny readback buffer.
2. Keep the current labels result semantics: raw signed and unsigned IDs, background miss policy,
   categorical scale lookup on the CPU, and `DVZ_SCENE_VISUAL_FAMILY_LABELS`.
3. Add transformed-panel and keep-aspect coverage so UV mapping is validated beyond the identity
   quad case.
4. Prototype a labels render-pass probe shader that writes raw IDs into a `1x1` integer target.
5. Move labels probing onto the shared request/payload/readback machinery once at least one other
   visual family can use the same executor.


## Validation

Focused tests should cover:

1. raw signed labels IDs such as `-7`;
2. high unsigned labels IDs such as `4000000000`;
3. background labels returning a miss by default;
4. scale-label resolution after raw ID readback;
5. panzoom and keep-aspect transformed panels;
6. topmost visual selection when multiple visuals overlap;
7. readback failure and stale request handling;
8. batched requests returning results in request order or with explicit request IDs;
9. render-pass probe parity with the current labels visual shader convention;
10. compute probe parity when compute is enabled.
