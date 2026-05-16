# Mars Textured Mesh Example Plan

> **Execution Status**
> - **Status:** `PROPOSAL`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define a polished scientific showcase for future textured-mesh support using real
>   Mars terrain data.


## Target Outcome

Add a native Datoviz example that renders a real Mars DEM as a 3D mesh and drapes a registered
orbital image or derived science layer onto it. The example should feel like a small terrain
analysis tool rather than a decorative textured surface demo.

Working title:

```text
hello_textured_mesh_mars.c
```

The core scene should exercise:

```text
GeoTIFF DEM + orthoimage
  -> preprocessed mesh positions, normals, UVs, indices, texture layers
  -> retained textured mesh visual
  -> arcball or turntable 3D camera
  -> app/GLFW presentation with layer and analysis controls
```


## Data Source

The best first real-data source is the USGS Mars Human Exploration Zone DEM Archive 2023:

1. source: https://astrogeology.usgs.gov/search/map/mars_human_exploration_zone_dem_archive_2023
2. content: roughly 2600 DEM products derived from Mars Reconnaissance Orbiter HiRISE and CTX
   stereo pairs;
3. relevant files per product: HiRISE DEM GeoTIFF, HiRISE orthoimage GeoTIFF, HiRISE good-pixel
   mask, CTX DEM GeoTIFF, CTX orthoimage GeoTIFF, CTX good-pixel mask, and XML metadata;
4. useful resolution: HiRISE DEM and orthoimage products are about 1 m per sample/pixel; CTX
   products are about 18 m per sample/pixel;
5. projection: Mars areoid / GCS Mars 2000 Sphere products that can be handled offline with GDAL.

This source is a good fit because DEM and image are already paired, registered, and available in
standard GIS formats. It avoids needing to infer texture coordinates from unrelated products.


## Scientific Framing

The example should be framed as **Mars terrain assessment from orbital stereo data**. That gives the
visual controls a real purpose and keeps the example close to actual planetary workflows.

Primary use case:

```text
Inspect a HiRISE stereo DEM and orthoimage for landing or rover-traverse hazards. Use elevation,
slope, hillshade, image texture, and stereo-quality layers to identify low-slope regions and verify
that apparent terrain features are supported by reliable DEM correlation.
```

Concrete questions the example should support:

1. Where are slopes too steep for safe traversal or landing?
2. Which flat regions are large enough to be plausible candidate zones?
3. Are there crater rims, scarps, dunes, channels, boulder fields, or rough patches that change the
   risk assessment?
4. Does the good-pixel mask show gaps or low-confidence regions that should not be overinterpreted?
5. How do features seen in the orthoimage align with relief in the DEM?


## Visual Effects

The visual polish should clarify terrain structure and data quality:

1. **Oblique low-sun lighting.** A directional light with adjustable azimuth and elevation makes
   subtle scarps, dune faces, channel banks, and crater rims readable.
2. **Vertical exaggeration.** A slider should scale relief, with a true-scale reset. A default of
   about 2x or 3x is reasonable for visual inspection, but the UI should expose that exaggeration is
   active.
3. **Texture layer selector.** Start with orthoimage, elevation colormap, slope map, hillshade, and
   good-pixel mask. Later layers can include false-color or multispectral products when a specific
   registered product is selected.
4. **Hillshade blend.** Allow the current texture layer to be multiplied or mixed with computed
   hillshade so geometry remains legible even on low-contrast image layers.
5. **Contour overlay.** Thin elevation isolines draped onto the mesh make the scene read as an
   analysis view and help compare relief across the panel.
6. **Hazard overlay.** A slope-threshold layer can tint regions above a user-controlled threshold.
   This turns the example into a plausible landing/traverse assessment view.
7. **Probe readout.** Hover or click should report elevation, slope, image/texture value, mask
   quality, and projected map coordinates. Latitude/longitude can be added when metadata conversion
   is included in the asset manifest.
8. **Camera bookmarks.** Provide a small set of preset views: overview, low-angle relief, nadir,
   and one selected feature close-up.
9. **Scale and orientation cues.** A scale bar, north arrow, and optional grid make the scene feel
   scientific rather than purely illustrative.


## Example Modes

The first version can be one example with a layer dropdown rather than multiple examples.

Suggested modes:

| Mode | Texture | Purpose |
| --- | --- | --- |
| `Image` | HiRISE or CTX orthoimage | inspect visible morphology |
| `Elevation` | colormapped DEM | inspect relative topography |
| `Slope` | derived slope raster | find steep or hazardous terrain |
| `Hillshade` | derived shaded relief | read terrain structure without image albedo |
| `Quality` | good-pixel mask | identify unreliable DEM regions |
| `Hazard` | slope threshold overlay | classify candidate safe/unsafe terrain |


## Preprocessing Pipeline

Keep the runtime example simple by converting source data into a compact Datoviz-ready asset bundle
offline.

Input:

1. DEM GeoTIFF;
2. registered orthoimage GeoTIFF;
3. good-pixel mask;
4. product metadata and citation text.

Offline processing:

1. crop to a visually interesting bounded region;
2. decimate or resample to an example-friendly grid size;
3. fill or mark NoData samples;
4. build a regular-grid indexed mesh from DEM samples;
5. compute vertex normals from local height gradients;
6. assign UVs from raster coordinates;
7. derive slope, hillshade, and optional contour/hazard rasters;
8. encode texture layers as RGBA images;
9. write a small manifest with source product id, spatial scale, elevation units, texture layer
   names, coordinate bounds, and citation.

Recommended bundle shape:

```text
examples/assets/mars_terrain/
  manifest.json
  mesh.bin
  indices.bin
  ortho_rgba.png
  elevation_rgba.png
  slope_rgba.png
  hillshade_rgba.png
  quality_rgba.png
```

The source GeoTIFFs should not be committed if they are large. Keep only a small derived example
bundle in-tree, or add a cache/download helper later when example infrastructure is ready.


## Datoviz Feature Requirements

This example should wait for, or directly motivate, a retained textured-mesh path:

1. mesh `position`, `normal`, `texcoords`, and `index` attributes;
2. a 2D sampled-field or texture binding accepted by mesh visuals;
3. a mesh shader variant that samples the texture and combines it with lighting/material state;
4. stable texture-layer switching without rebuilding mesh buffers;
5. sampler parameters appropriate for terrain textures;
6. optional mesh probe/pick path that returns face, barycentric coordinate, elevation, slope, and
   texture samples;
7. enough UI controls in the example to switch layers, adjust light direction, vertical
   exaggeration, hillshade mix, and hazard threshold.

Existing image visual texture upload and sampled-field machinery should be reused where possible.
The missing feature is the mesh-side UV, texture-binding, and shader/pipeline variant contract.


## Minimal First Slice

The smallest useful version is:

1. one preprocessed Mars terrain asset;
2. one `dvz_mesh()` textured variant with UVs and a single RGBA texture;
3. arcball camera;
4. directional lighting;
5. vertical exaggeration baked into the mesh or controlled through a uniform;
6. layer switching between orthoimage and elevation color texture;
7. screenshot-friendly bounded run.

After that works, add slope, hillshade, quality mask, contour, probe readout, and hazard threshold
controls.


## Selection Criteria For The First Mars Product

Pick a source product that has:

1. a visually obvious terrain feature such as a crater rim, channel, dune field, scarp, or layered
   outcrop;
2. good overlap between DEM and orthoimage;
3. a mostly clean good-pixel mask in the crop;
4. enough relief to make lighting and vertical exaggeration useful;
5. source files small enough to preprocess quickly;
6. a citation and product id that can be included in the manifest.

The first checked-in asset should be intentionally small. The point is to prove textured mesh
rendering and scientific controls, not to ship a full planetary GIS dataset.
