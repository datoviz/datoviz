# GSP, VisPy2, and Datoviz Backend Strategy

This note records the intended split between Datoviz v0.4, GSP, VisPy2, and
Matplotlib-oriented publication export.


## Summary

Datoviz v0.4 should be released as the low-level rendering engine: the C library,
the public C API, and one generated Python `ctypes` binding. It should not ship a
replacement for the v0.3 Python object-oriented API.

The higher-level Python object model should live in GSP. The user-facing plotting
layer should live above GSP, likely in the VisPy2 repository. GSP should be the
semantic scene and plot specification that can target multiple renderers,
including Datoviz for interactive GPU rendering and Matplotlib for publication
and vector export.


## Datoviz v0.4 Python Scope

Datoviz v0.4 is the engine release. Its Python scope is intentionally narrow:

1. package and load the Datoviz shared library,
2. expose the generated `ctypes` binding for the public C API, with `datoviz.raw` as its exact
   pointer/count call form,
3. keep low-level smoke examples that prove the binding works,
4. avoid committing to a v0.3-style Python object-oriented API in the Datoviz
   repository.

This keeps the Datoviz release focused on the C runtime, scene-to-DRP2 path,
canvas/app integration, packaging, and low-level binding correctness.


## Role Of GSP

GSP is the semantic object layer. It should describe figures, panels, axes,
visuals, data bindings, scales, cameras, materials, lights, annotations, and
export intent without depending on a specific renderer.

GSP should preserve semantic objects rather than exposing backend draw calls as
its primary model. That gives each backend enough information to lower the same
scene to the best representation it supports:

- Datoviz can lower the scene to retained scene objects, DRP2 streams, and GPU
  resources.
- Matplotlib can lower the scene to artists, collections, layout objects, and
  vector-capable output backends.
- Future backends can implement the same semantic subset without inheriting
  Datoviz internals.


## Role Of VisPy2

VisPy2 should own the Python user experience above GSP:

1. high-level plotting functions such as `plot()`, `scatter()`, and `imshow()`,
2. object-oriented figure, axes, subplot, visual, and interaction APIs,
3. notebook and hosted-UI integration,
4. galleries and examples aimed at scientific users,
5. backend selection policies and user-facing warnings.

Datoviz remains one backend beneath that stack, not the home of the high-level
Python plotting API.


## Scientific Plotting Boundary

Datoviz should provide rendering-native scientific building blocks that GSP and
VisPy2 can target. Examples include guide lines and spans, bars/intervals for
pre-binned data, uncertainty bands/ribbons, thick paths, and efficient stacked
trace collections.

GSP and VisPy2 should own the higher-level plotting layer: `hist()`,
`fill_between()`, `axhline()`/`axvline()` aliases, bin-selection policy, weighted
or cumulative histogram semantics, spike-train autocorrelogram computation,
NumPy/pandas/xarray adaptation, subplot grammar, and user-facing plot defaults.

The intended split is:

| Feature | Datoviz | GSP/VisPy2 |
|---|---|---|
| guide lines/spans | scene annotations lowered to built-in visuals | plotting aliases and defaults |
| histogram rendering | bars/intervals or pre-binned histogram composite | `hist()` statistics and data adaptation |
| uncertainty bands | band/ribbon composite plus path styling | `fill_between()` and interval computation |
| stacked traces | efficient path/trace rendering | ergonomic multi-series plotting API |
| spike-train workflows | deterministic C example with prepared data | neuroscience analysis and recipe layer |

The durable design boundary lives in
[`spec/scene/composites/SCIENTIFIC_PLOTTING_BOUNDARY.md`](../../spec/scene/composites/SCIENTIFIC_PLOTTING_BOUNDARY.md).


## Backend Roles

### Datoviz Backend

The Datoviz backend is optimized for interactive GPU rendering:

- large point clouds, images, paths, meshes, volumes, and other GPU-heavy
  visuals,
- interactive pan/zoom, arcball, picking, probing, and live updates,
- offscreen and windowed raster rendering,
- high-DPI raster screenshots and video/capture workflows,
- low overhead when data or camera state changes frequently.

Datoviz-native export should be treated as raster-first. Datoviz v0.4 should not
grow a native structural SVG/PDF exporter; publication-oriented vector output
belongs to the GSP Matplotlib backend.

### Matplotlib Backend

The Matplotlib backend is optimized for static, publication-oriented output:

- PDF and SVG vector export,
- publication typography and layout,
- axes, ticks, labels, legends, annotations, and colorbars,
- static 2D plots and static 3D scenes,
- mesh, lighting, texture, and wireframe support where Matplotlib can represent
  them, even if rendering is slower than Datoviz.

The Matplotlib backend should not be described as merely a weak fallback. It is
the publication and vector-export backend for the shared GSP scene description.


## Capability Model

GSP should expose backend capability information early. Backend support should be
classified per feature, not reduced to a global yes/no answer.

Useful support states include:

| State | Meaning |
|---|---|
| `supported` | The backend implements the feature directly. |
| `approximate` | The backend can render a close but not identical representation. |
| `slow` | The backend can render the feature, but users should expect poor performance. |
| `rasterized` | The backend can include the feature only as raster content in an otherwise vector output. |
| `unsupported` | The backend cannot implement the feature. |

Useful capability groups include:

- interactivity: pan/zoom, arcball, picking, probing, live updates,
- export: PNG, high-DPI PNG, SVG, PDF, vector paths, vector text, mixed
  vector/raster output,
- geometry: points, markers, lines, paths, images, meshes, wireframes, textures,
  volumes,
- style: colormaps, alpha, lighting, materials, line joins, line caps,
- layout: axes, ticks, labels, legends, colorbars, subplots.

Backend warnings should be explicit when a scene is approximate, slow,
rasterized, decimated, or unsupported.


## Expected User Workflow

The preferred user story is one semantic scene with multiple realizations:

```python
fig = vispy2.figure()
ax = fig.subplot(projection='3d')
ax.mesh(vertices, faces, texture=texture, wireframe=True, lighting='phong')

fig.show(backend='datoviz')
fig.savefig('figure.pdf', backend='matplotlib')
fig.savefig('figure.svg', backend='matplotlib')
fig.savefig('figure.png', backend='datoviz', scale=4)
```

The core promise is complementary rather than competitive:

> Explore interactively with Datoviz. Export publication-quality vector figures
> with Matplotlib. Keep the scene semantics shared through GSP.


## Datoviz v0.3 Migration Message

The v0.3 Python object-oriented API should be documented as not being part of the
Datoviz v0.4 release scope. The migration message should be:

1. use Datoviz v0.4's generated Python binding for low-level engine access,
2. use GSP/VisPy2 for the future Python object-oriented and plotting APIs,
3. use the Datoviz backend for interactive and high-throughput GPU rendering,
4. use the Matplotlib backend for publication-oriented PDF/SVG export.


## Open Questions

1. Should GSP live as an independently packageable subpackage inside the VisPy2
   repository, or as its own repository?
2. What is the minimum Datoviz backend capability report that GSP needs for the
   first VisPy2 integration?
3. Which v0.3 Python examples should become GSP/VisPy2 examples rather than
   Datoviz examples?
4. Which Matplotlib 3D mesh features are reliable enough to advertise as
   supported rather than approximate or experimental?
