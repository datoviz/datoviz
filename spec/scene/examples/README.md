# Scene Worked Examples

This directory contains worked examples that pressure-test the current scene specification.

These examples are not API proposals.

They are also not independent normative sources.

They should be read as informative pressure tests of the main scene-spec documents.

Their role is narrower:

1. instantiate the current family contracts,
2. exercise the transform pipeline,
3. show how resources feed `FramePlan`,
4. show which DRP2 command categories are implied.


## Current Examples

1. `POINT_2D.md`
2. `MARKER_PICKING.md`
3. `PATH_AXES_2D.md`
4. `IMAGE_SLICE.md`
5. `SPHERE_IMPOSTOR.md`
6. `VOLUME_OFFSCREEN.md`
7. `LINKED_PANELS_PROBE_COLORBAR.md`
8. `MOUSE_BRAIN_ATLAS_EXPLORER.md`
9. `LINKED_PANELS_AXES_PANZOOM.md`
10. `ANIMATION_VIDEO_EXPORT.md`


## Common Structure

Each example should describe:

1. the owning specs that define the normative rules it exercises,
2. the scene setup,
3. the visual family and variant,
4. the resource schema instance,
5. the transform pipeline for that case,
6. the resulting `FramePlan` shape,
7. the DRP2 command categories implied,
8. the key pressure on the current spec.
