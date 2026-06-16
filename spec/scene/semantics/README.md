# Scene Semantic Specs

This directory contains the user-visible meaning of scene objects and visual behavior.

Use these files when changing axes, scales, annotations, legends, visual-family rules, lighting,
transparency, clipping, transforms, or geometry utilities.


## Files

1. [VISUAL_FAMILIES.md](VISUAL_FAMILIES.md): visual-family taxonomy.
2. [VISUAL_CONTRACT.md](VISUAL_CONTRACT.md): shared producer contract for all visuals.
3. [VISUAL_FAMILY_RULES.md](VISUAL_FAMILY_RULES.md): family boundary rules and anti-patterns.
4. [COLOR_MANAGEMENT.md](COLOR_MANAGEMENT.md): sRGB-authored colors, linear rendering, texture
   color roles, and output encoding.
5. [SCALES.md](SCALES.md): scale and colormap semantics.
6. [SAMPLED_FIELD_INTERPRETATION.md](SAMPLED_FIELD_INTERPRETATION.md): sampled-field format,
   semantic, colorizer, visual-technique, and query behavior.
7. [AXES.md](AXES.md): axes, ticks, labels, and domain behavior.
8. [ANNOTATIONS.md](ANNOTATIONS.md): labels, guides, probes, overlays, and callouts.
9. [LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md): explanatory mapping objects.
10. [LIGHTING.md](LIGHTING.md): scene lighting model and ray-tracing forward compatibility.
11. [TRANSPARENCY.md](TRANSPARENCY.md): alpha modes and transparency planning.
12. [EFFECTS.md](EFFECTS.md): screen-space outline, edge enhancement, bloom, and effect ordering.
13. [TEXT.md](TEXT.md): text content, placement, resources, and DPI behavior.
14. [SYMBOLS.md](SYMBOLS.md): reusable marker/annotation/vector-head symbol resources.
15. [CLIPPING.md](CLIPPING.md): clip modes and data-area clipping.
16. [NONLINEAR_TRANSFORMS.md](NONLINEAR_TRANSFORMS.md): non-linear coordinate transforms.
17. [GEOMETRY_UTILITIES.md](GEOMETRY_UTILITIES.md): CPU-side geometry utility layer.


## Active Proposal Inputs

1. [../proposals/promoted/ANNOTATION_MEASUREMENT_DESIGN.md](../proposals/promoted/ANNOTATION_MEASUREMENT_DESIGN.md)
   for dimensions/callouts beyond landed label and scale-bar slices
2. [../proposals/promoted/AXES_DOMAIN_DESIGN.md](../proposals/promoted/AXES_DOMAIN_DESIGN.md)
   for linked-domain and unit rationale beyond landed linear axes
3. [../proposals/promoted/COLORBAR_COLORMAP_DESIGN.md](../proposals/promoted/COLORBAR_COLORMAP_DESIGN.md)
4. [../proposals/promoted/GEOM_DESIGN.md](../proposals/promoted/GEOM_DESIGN.md)
5. [../proposals/promoted/MESH_SHADING_DESIGN.md](../proposals/promoted/MESH_SHADING_DESIGN.md)
6. [../proposals/future/RAY_TRACING_FORWARD_COMPAT.md](../proposals/future/RAY_TRACING_FORWARD_COMPAT.md)
7. [../proposals/active/SCIENTIFIC_COORDINATE_NORMALIZATION.md](../proposals/active/SCIENTIFIC_COORDINATE_NORMALIZATION.md)
8. [../proposals/promoted/TEXT_DESIGN.md](../proposals/promoted/TEXT_DESIGN.md)
9. [../proposals/promoted/TRANSPARENCY_WBOIT_DESIGN.md](../proposals/promoted/TRANSPARENCY_WBOIT_DESIGN.md)
