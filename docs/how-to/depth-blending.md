# Control depth, blending, and transparency

Choose the rendering state for overlapping geometry, transparent visuals, and depth-based visual
cues.

!!! info "At a glance"

    - **Status:** Depth testing and source-over blending are supported; OIT/depth techniques have narrower backend coverage.
    - **Languages:** C-first; Python exposes the same generated visual/figure state calls.
    - **Prerequisites:** Deliberate opaque/transparent grouping, meaningful alpha, and a valid camera/depth range.
    - **Result:** Opaque geometry occludes correctly and transparent geometry uses an explicit approximation or technique.

## Use this when

- 3D objects should occlude each other correctly.
- A visual has meaningful alpha values and must blend with the scene.
- Dense or translucent geometry looks wrong because draw order matters.
- Depth variation is hard to read and needs depth cueing, AO, or another depth technique.
- A browser target needs a status check before using an advanced rendering technique.

## Minimal sequence

Start with an opaque scene. Enable depth testing on eligible 3D visuals that should participate in scene occlusion. Image visuals currently do not test or write raster depth; use a textured mesh for a physical plane. Check the family coverage below before choosing an alpha mode.

```c
dvz_visual_set_depth_test(visual, true);
```

Only then add transparency to the visuals that actually need it.

```c
dvz_visual_set_data(visual, "color", rgba, n);
dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED);
```

Use alpha blending only when the color data carries useful alpha. Keep technique state on the
visual that needs it instead of changing unrelated scene state.

These are C setup excerpts. Check setter results and validate the final result from multiple camera
angles; one static viewpoint can hide order-dependent transparency errors.

<figure class="dvz-output-example">
  <div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
    <img class="dvz-gallery-poster"
         src="../../assets/gallery/v0.4/features/features_technique_depth_test.poster.webp"
         alt="Overlapping 3D points before the depth-test comparison rotates" loading="lazy">
    <video class="dvz-gallery-video" muted loop playsinline preload="none"
           poster="../../assets/gallery/v0.4/features/features_technique_depth_test.poster.webp"
           aria-label="Depth testing comparison preview">
      <source data-src="../../assets/gallery/v0.4/features/features_technique_depth_test.mp4"
              type="video/mp4">
    </video>
  </div>
  <figcaption>
    <strong>Depth testing.</strong> Rotation makes the correct and incorrect occlusion order visible
    from several viewpoints.
    <a href="../../examples/features/technique-depth-test/">Open the example</a>.
  </figcaption>
</figure>

## Choose a technique

| Goal | Use | Notes |
| --- | --- | --- |
| Ordinary opaque 3D geometry | `dvz_visual_set_depth_test(visual, true)` with the default opaque alpha mode | Best default for mesh, primitive, sphere, point, and pixel visuals in 3D. |
| Screen-like overlays or diagnostics | `dvz_visual_set_depth_test(visual, false)` | Useful for overlays; avoid using it to hide an ordering bug in 3D data. |
| Simple translucent visual | `DVZ_ALPHA_BLENDED` | Source-over blending. Result can depend on draw order. |
| Many overlapping translucent items | `DVZ_ALPHA_WBOIT` | Weighted blended order-independent transparency. More stable than source-over, but approximate. |
| Layered transparent surfaces | `DVZ_ALPHA_DEPTH_PEEL` | More accurate order-independent transparency for layered surfaces; native path, higher cost. |
| Depth perception in dense 3D scenes | `dvz_visual_set_depth_cue()` | Fades or darkens by depth without changing geometry. |
| Jagged edges | MSAA example and panel/runtime sample settings | Antialiasing is separate from alpha correctness. |

## Current native technique coverage

This boundary applies to the RC4 development source. It does not replace exact-package or platform validation. Setters retain requested alpha state; `dvz_visual_validate()` and figure planning reject unsupported combinations with a family-specific diagnostic. Setter success alone is not a support check.

| Visual family/path | Raster depth | WBOIT | Depth peeling |
| --- | --- | --- | --- |
| Ordinary untextured mesh/primitive | Supported with depth enabled. | Dedicated lit/unlit variants. | Primitive/ordinary-mesh variants; fixed four-iteration budget. |
| Textured mesh/physical plane | Supported with depth enabled; effective-zero alpha and masked texels discard. | Dedicated sampled/material accumulation variant. | Rejected: no compatible textured variant. |
| Point | Supported with depth enabled. | Dedicated disc/style and depth-cue variants. | Rejected: no coverage-preserving variant. |
| Pixel/marker | Supported with depth enabled. | Rejected: no conformant accumulation variant. | Rejected: no coverage-preserving variant. |
| Analytic sphere | Supported with analytic fragment depth. | Rejected: no accumulation variant. | Rejected: no sphere-preserving variant. |
| Segment/stroked path/vector | Supported for documented 3D stroke paths. | Rejected: no dedicated stroke variant. | Rejected: no coverage-preserving stroke variant. |
| Image/integer labels | Current image-style paths do not test/write raster depth. | Rejected: no accumulation variant. | Rejected: no compatible sampled-field variant. |
| Gaussian splat | Center-depth test; ordinary blending does not write depth. | Dedicated accumulation variant; family remains experimental. | Rejected: no Gaussian-coverage variant. |
| Volume | Mode-specific ray-marched composition. | Rejected: no accumulation variant. | Rejected: no volume-preserving variant. |

`DVZ_ALPHA_MASK` supports textured meshes and physical planes. `dvz_visual_set_alpha_cutoff(visual, cutoff)` retains a finite threshold in `[0, 1]`, defaulting to `0.5`. Fragments below the effective-alpha cutoff discard; equality survives, while zero alpha always discards. Masked planes test and write raster depth. Other families reject masking; sampled/masked mesh queries, AO capture, and scene-occluder production remain unsupported.

Scene-occluder producers are restricted to ordinary untextured mesh/primitive and the existing bounded volume route. Lit mesh/primitive occluders with zero material opacity or base-color alpha are rejected because the current depth producer does not evaluate material alpha. Points, markers, images, textured meshes, spheres, splats, and stroked paths cannot act as scene-occluder producers.

OIT excludes mesh item-state styling; scene-occluder production also excludes item-state, instancing, and view-dependent LIMB material alpha. AO capture excludes item-state and instancing because its producer does not preserve those variants. Ordinary rendering and documented occlusion-consumer paths remain separate capabilities.

WBOIT makes participating fragments order independent within its accumulation group. Mixing source-over layers, separate OIT groups, and volume composition does not provide globally correct transparency. Primitive/mesh, point, and sampled-plane WBOIT have shared WGSL accumulation and resolve routes, requiring two blendable floating-point render targets. Peeling, AO capture, and scene occlusion remain outside this browser slice; shader preflight diagnoses missing sources. For browser coverage, see the [WebGPU subset](../reference/webgpu-subset.md).

## Transparency rules

Transparent rendering is not just a color-alpha setting. Depth testing, alpha mode, and draw order
interact:

- Opaque visuals should usually render first with depth testing enabled.
- Transparent visuals should usually keep depth testing enabled so opaque geometry can occlude them.
- `DVZ_ALPHA_BLENDED` does not sort your data for you; overlapping transparent objects may still
  need a different technique.
- `DVZ_ALPHA_WBOIT` and `DVZ_ALPHA_DEPTH_PEEL` use specialized transparency passes. Check browser
  support before relying on them in WebGPU examples.
- Text, labels, images, and some annotation helpers may configure alpha blending internally because
  their pixels naturally include transparent coverage.

<figure class="dvz-output-example">
  <div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
    <img class="dvz-gallery-poster"
         src="../../assets/gallery/v0.4/features/features_technique_transparency.poster.webp"
         alt="Overlapping translucent cubes before the transparency comparison rotates"
         loading="lazy">
    <video class="dvz-gallery-video" muted loop playsinline preload="none"
           poster="../../assets/gallery/v0.4/features/features_technique_transparency.poster.webp"
           aria-label="Transparency technique comparison preview">
      <source data-src="../../assets/gallery/v0.4/features/features_technique_transparency.mp4"
              type="video/mp4">
    </video>
  </div>
  <figcaption>
    <strong>Transparency order.</strong> A moving view exposes differences between source-over,
    weighted blended, and depth-peel results.
    <a href="../../examples/features/technique-transparency/">Open the example</a>.
  </figcaption>
</figure>

## Depth cue

Depth cueing is a visual perception aid, not a camera or clipping change. It is supported on point,
pixel, primitive, mesh, and sphere visuals.

```c
DvzDepthCueDesc cue = dvz_depth_cue_desc();
cue.near_depth = 0.10f;
cue.far_depth = 0.90f;
dvz_visual_set_depth_cue(visual, &cue);
```

`near_depth` and `far_depth` use the descriptor metric, where lower values are closer. The default
metric is normalized clip depth after the scene transform. Pass `NULL` to
`dvz_visual_set_depth_cue()` to disable depth cueing.

<figure class="dvz-output-example">
  <div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
    <img class="dvz-gallery-poster"
         src="../../assets/gallery/v0.4/features/features_technique_depth_cue.poster.webp"
         alt="A sphere lattice before the depth-cue preview rotates" loading="lazy">
    <video class="dvz-gallery-video" muted loop playsinline preload="none"
           poster="../../assets/gallery/v0.4/features/features_technique_depth_cue.poster.webp"
           aria-label="Depth cue preview">
      <source data-src="../../assets/gallery/v0.4/features/features_technique_depth_cue.mp4"
              type="video/mp4">
    </video>
  </div>
  <figcaption>
    <strong>Depth cue.</strong> Rotation shows how distance-dependent fading reinforces spatial
    ordering without changing geometry.
    <a href="../../examples/features/technique-depth-cue/">Open the example</a>.
  </figcaption>
</figure>

## Color pipeline

Datoviz treats authored RGBA colors as display/sRGB semantic colors. The default figure color
pipeline, `DVZ_COLOR_PIPELINE_LINEAR_SRGB`, converts RGB to linear values before scene arithmetic,
blends alpha in linear color space, and encodes display targets back to sRGB for capture or
presentation.

Use `DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND` only when an application needs compatibility with
renderers that blend RGB directly in display/sRGB values, such as Matplotlib/Agg comparison images.
In that mode Datoviz skips the linear intermediate and final display encode for UNORM external
targets, and semantic RGB colors are passed through unchanged in shaders. Alpha remains linear.

```c
dvz_figure_set_color_pipeline(figure, DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND);
```

The setting is figure-wide for app and offscreen rendering. Select it before rendering the first
frame so Datoviz prepares the intended rendering path.

## Browser support

The native path is the reference for advanced transparency techniques. Check the
[WebGPU subset](../reference/webgpu-subset.md) and the relevant gallery page before choosing a
technique for the experimental browser path.

## Complete examples

- [Depth Test Toggle](../examples/features/technique-depth-test.md) - compare overlapping
  3D points with depth testing on and off. Source:
  `examples/c/features/technique_depth_test.c`.
- [Alpha Blending](../examples/features/alpha-blending.md) - ordinary source-over alpha
  blending. Source: `examples/c/features/alpha_blending.c`.
- [Transparency Order](../examples/features/technique-transparency.md) - compare
  source-over, WBOIT, and depth-peel transparency on overlapping cubes. Source:
  `examples/c/features/technique_transparency.c`.
- [Depth Cue](../examples/features/technique-depth-cue.md) - depth-dependent fading on a
  3D sphere lattice. Source: `examples/c/features/technique_depth_cue.c`.
- [Multisample Antialiasing](../examples/features/technique-msaa.md) - use MSAA for edge
  quality. Source: `examples/c/features/technique_msaa.c`.


## Important details

- Depth testing is per visual in the retained scene API. Do not solve one visual's ordering issue by
  disabling depth everywhere.
- Transparent visuals can still be hidden by opaque depth. That is usually correct.
- Picking and probing follow rendered visibility rules. A visual hidden by depth state may also be
  absent from the expected query result.
- Volume, AO, EDL, and other depth-based techniques have their own examples and status. Do not
  assume a technique is portable just because the visual family is portable.

## Common mistakes

- Disabling depth globally to fix one transparent visual.
- Assuming alpha values change draw order automatically.
- Using `DVZ_ALPHA_BLENDED` for many interpenetrating translucent objects and expecting exact
  order-independent results.
- Turning off depth testing for labels or overlays without checking whether they should be occluded
  by 3D geometry.
- Copying deferred or native-only technique code into a WebGPU target without checking status.

## Next steps

- [Use lighting and materials](lighting-and-materials.md)
- [Debug rendering output](debug-rendering.md)
- [Profile rendering performance](profile-performance.md)
- [WebGPU subset](../reference/webgpu-subset.md)
- [Visual API reference](../reference/c-api/visuals.md)
