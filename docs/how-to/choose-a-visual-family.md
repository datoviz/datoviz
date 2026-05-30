# Choose A Visual Family

Status: planned decision guide.

This page should help users choose the right retained visual family before they write code. It
should be authored decision-oriented prose, not a generated API table and not a gallery substitute.

Compare visual families by:

1. data model: regular grid, sampled field, point samples, indexed geometry, text, labels, or
   semantic object;
2. dimensionality: 2D panel data, 3D world geometry, or screen-space overlay;
3. density: many simple items versus fewer semantically rich objects;
4. value type: continuous scalar field, categorical labels, colors, vectors, geometry, or texture;
5. interaction: panzoom, arcball, picking, probing, selection, or no interaction;
6. backend scope: native-only, WebGPU/WASM experimental subset, or deferred backend behavior;
7. update pattern: static data, full replacement, partial update, or streaming.

The page should answer questions such as:

| If the data is... | Prefer... | Check... |
| --- | --- | --- |
| irregular 2D or 3D samples | point, marker, or sphere | density, shape encoding, picking needs |
| a regular 2D scalar field | image or pixel | interpolation, colormap, probe behavior |
| explicit triangle geometry | mesh | indexing, normals, materials, arcball |
| ordered lines | path or segment | continuity, stroke picking, update pattern |
| integer regions | labels | probe/readout semantics and color mapping |
| 3D sampled data | volume | supported slicing/probing and release status |

Each recommendation should link to the minimal example, the visual-family reference entry, and any
relevant how-to page. Do not duplicate large code blocks here; use links to copy-safe examples.
