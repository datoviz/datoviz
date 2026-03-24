# Scene Worked Examples

This directory contains worked examples that pressure-test the current scene specification.

These examples are not API proposals.

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


## Common Structure

Each example should describe:

1. the scene setup,
2. the visual family and variant,
3. the resource schema instance,
4. the transform pipeline for that case,
5. the resulting `FramePlan` shape,
6. the DRP2 command categories implied,
7. the key pressure on the current spec.
