# Scene Color Management

## Status

Normative for scene-level color interpretation, rendering arithmetic, texture color roles, and
standard display/export output.

## Purpose

Datoviz scene colors describe semantic visual intent. They are not framebuffer-encoded values and
they do not expose backend render-target or window-system color-space details.

The scene graph remains backend-agnostic: it records authored color intent, texture color roles, and
output expectations. The runtime chooses the concrete framebuffer formats, swapchain formats,
readback paths, and final encode mechanism.

## Core Rules

1. Datoviz semantic colors are authored in sRGB by default.
2. Datoviz rendering arithmetic is linear RGB.
3. Datoviz display and standard image export are sRGB by default.
4. sRGB conversion happens exactly once, at the final display or standard image-export boundary.
5. Alpha is always linear. It is never sRGB-encoded or sRGB-decoded.
6. The scene API must not expose Vulkan swapchain formats, framebuffer formats, or window-system
   color-space details.

`DvzColor`, `rgba_u8`, and equivalent scene-level color values are interpreted as sRGB-authored
colors unless a more specific field contract says otherwise. Before rendering arithmetic, RGB
channels are linearized. Alpha is copied as a linear scalar.

All lighting, blending, antialiasing, transparency, weighted blended OIT, volume compositing,
multisampling resolves, and post-processing operate in linear RGB. Their output remains linear
until the final display/export encode.

## Default Interpretations

| Scene value or resource | Default interpretation |
|---|---|
| `DvzColor`, `rgba_u8`, and equivalent scene colors | sRGB-authored RGB, linear alpha |
| Built-in and custom colormap tables | sRGB-authored RGB, linear alpha unless explicitly stated otherwise |
| Scalar sampled fields | `data` |
| Masks, labels, ids, picking, depth-like, normal, and position textures | `data` |
| RGBA PNG/JPEG-like image fields and UI atlases | `srgb_color` |
| Diffuse/albedo material textures | `srgb_color` |
| HDR color textures and linear render inputs | `linear_color` |
| Generated intermediate render targets | linear color storage |
| Standard screenshot/export image | sRGB `u8` RGBA |
| Future scientific float readback | explicit linear `f16` or `f32` RGBA |

## Texture Color Roles

Every texture-like scene resource that can be sampled as color or data must declare its color role:

| Role | Meaning | Typical inputs |
|---|---|---|
| `srgb_color` | sRGB-authored color that must be linearized before rendering arithmetic | ordinary color images, PNG/JPEG textures, UI images, perceptual color tables |
| `linear_color` | already-linear RGB color | HDR textures, linear render inputs, physically based shading inputs |
| `data` | non-color data; sampled values are not sRGB-decoded | scalar fields, masks, labels, IDs, normals, positions, depth-like data, lookup/index textures |

Visual and resource specifications may provide defaults for common cases, but the lowered scene
resource contract must make the role explicit. Sampling a `data` texture for colormap lookup keeps
the sampled value in the data domain; the colormap result is then treated as an sRGB-authored
semantic color unless explicitly declared otherwise.

Missing texture roles are invalid once the color-role field is part of the resource contract. During
transition, the scene may infer the documented default and emit a diagnostic.

## Output Boundary

The final panel/composited image is linear until it reaches an output boundary.

If the runtime uses an sRGB-capable final render target or swapchain format, hardware may perform
the final linear-to-sRGB encode. If the final target is not sRGB-encoded, the runtime must insert an
explicit final encode pass when producing standard display or screenshot output.

Standard 8-bit image output, such as PNG screenshots, is sRGB-encoded by default. Future scientific
readback paths may request explicit linear outputs; that deferred choice belongs to the
app/canvas/runtime layer, not to the scene graph.

## Conformance Requirements

1. `rgba_u8` scene colors round-trip as authored sRGB values at API boundaries.
2. RGB inputs are linearized before lighting, blending, antialiasing, transparency, WBOIT, volume
   compositing, multisampling resolve, or post-processing arithmetic.
3. Alpha values are never sRGB-decoded or sRGB-encoded.
4. Standard PNG screenshot output is sRGB `u8` RGBA.
5. Texture resources carry an explicit color role before lowering to backend resources.
6. Final display/export encoding happens exactly once.

Explicit linear `f16`/`f32` scientific image export/readback is deferred beyond RC1 and is not a
v0.4.0 conformance requirement.

## Non-Goals

This document does not define:

1. HDR display management;
2. ICC profile handling;
3. full OCIO-style color pipelines;
4. perceptual colormap design.
