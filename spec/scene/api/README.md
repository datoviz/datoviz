# Scene API Specs

This directory contains the public API profile and implementation bridge.

Use these files when drafting public headers, deciding handle/descriptor/result shapes, or mapping
scene concepts into C and language bindings.


## Files

1. [API_DESIGN.md](API_DESIGN.md): preferred scene-facing API profile and rationale.
2. [API_SURFACE.md](API_SURFACE.md): immediate public-header target for interaction, scales,
   colorbars, text, and annotations.
3. [API_IMPLEMENTATION_READINESS.md](API_IMPLEMENTATION_READINESS.md): implementation-readiness
   checklist for the next public API pass.
4. [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md): C object mapping and binding architecture.


## Related Directories

1. [../headers](../headers): implementation-facing draft header sketches.
2. [../slices](../slices): implementation-ready work packets for mature API areas.
3. [../proposals](../proposals): active design addenda that feed future API drafts.
4. [../examples](../examples): API pressure sketches and worked examples.


## Active Proposal Inputs

1. [../proposals/INTERACTION_API_DESIGN.md](../proposals/INTERACTION_API_DESIGN.md)
2. [../proposals/ANNOTATION_TEXT_SCALE_API.md](../proposals/ANNOTATION_TEXT_SCALE_API.md)
3. [../proposals/MESH_API_DESIGN.md](../proposals/MESH_API_DESIGN.md)
4. [../proposals/MATERIAL_LIGHTING_API.md](../proposals/MATERIAL_LIGHTING_API.md)
5. [../proposals/SAMPLED_FIELD_API_DESIGN.md](../proposals/SAMPLED_FIELD_API_DESIGN.md)


## Active Implementation Slice Inputs

1. [../slices/TEXT_RENDERING_SLICE.md](../slices/TEXT_RENDERING_SLICE.md)
2. [../slices/ANNOTATION_LABEL_SLICE.md](../slices/ANNOTATION_LABEL_SLICE.md)
3. [../slices/COLORBAR_RENDERING_SLICE.md](../slices/COLORBAR_RENDERING_SLICE.md)
4. [../slices/LEGEND_SLICE.md](../slices/LEGEND_SLICE.md)
