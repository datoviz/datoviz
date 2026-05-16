# Scene Worked Examples

This directory contains worked examples that pressure-test the current scene specification.

Most examples are not API proposals. Files under [api/](api/) are deliberate API-shape sketches.
They are informative, and may use installed-but-not-yet-implemented function names to
pressure-test the public API shape.

These files are not independent normative sources. They should be read as informative pressure
tests of the main scene-spec documents.

Their role is narrower:

1. instantiate the current family contracts,
2. exercise the transform pipeline,
3. show how resources feed `FramePlan`,
4. show which DRP2 command categories are implied.


## Directory Layout

### `core/`

Small examples that should stay closest to the active v0.4 implementation path.

1. [core/POINT_2D.md](core/POINT_2D.md)
2. [core/MARKER_PICKING.md](core/MARKER_PICKING.md)
3. [core/PATH_AXES_2D.md](core/PATH_AXES_2D.md)
4. [core/VOLUME_SLICE.md](core/VOLUME_SLICE.md)
5. [core/VOLUME_OFFSCREEN.md](core/VOLUME_OFFSCREEN.md)
6. [core/SPHERE_IMPOSTOR.md](core/SPHERE_IMPOSTOR.md)
7. [core/LINKED_PANELS_AXES_PANZOOM.md](core/LINKED_PANELS_AXES_PANZOOM.md)
8. [core/LINKED_PANELS_PROBE_COLORBAR.md](core/LINKED_PANELS_PROBE_COLORBAR.md)
9. [core/MOUSE_BRAIN_ATLAS_EXPLORER.md](core/MOUSE_BRAIN_ATLAS_EXPLORER.md)

### `api/`

API-shape sketches.

1. [api/API_IMAGE_PROBE_PINNED_READOUT.md](api/API_IMAGE_PROBE_PINNED_READOUT.md)
2. [api/API_MESH_SELECTION_LINK.md](api/API_MESH_SELECTION_LINK.md)
3. [api/API_SAMPLED_FIELD.md](api/API_SAMPLED_FIELD.md)
4. [api/API_SCALE_COLORBAR_ANNOTATION.md](api/API_SCALE_COLORBAR_ANNOTATION.md)

### `napari/`

Napari-class image, label, points, tracks, vectors, and orthoslice pressure tests.

### `compute/`

Compute-heavy and shader-heavy examples.

1. [compute/PARTICLES.md](compute/PARTICLES.md)
2. [compute/GRAY_SCOTT.md](compute/GRAY_SCOTT.md)
3. [compute/MANDELBROT.md](compute/MANDELBROT.md)

### `science/`

Scientific domain examples that pressure-test geometry, fields, volume, labels, and interaction.

### `geo/`

Geographic, projection, terrain, atmosphere, and globe examples.

### `dashboards/`

Operational or dense-interface examples with multiple coordinated panels.

### `showcase/`

Gallery-style or polished demonstration examples that combine several lower-level capabilities.

1. [showcase/ANIMATION_VIDEO_EXPORT.md](showcase/ANIMATION_VIDEO_EXPORT.md)
2. [showcase/ASTRONOMY_MANY_LABELS.md](showcase/ASTRONOMY_MANY_LABELS.md)
3. [showcase/GALAXY.md](showcase/GALAXY.md)
4. [showcase/IMAGE_EMBEDDING_LOD.md](showcase/IMAGE_EMBEDDING_LOD.md)
5. [showcase/LATEX_MICROTEX_TEXT_VISUAL.md](showcase/LATEX_MICROTEX_TEXT_VISUAL.md)
6. [showcase/ALLEN_IBL_COORDINATE_MESH_VOLUME_PLAN.md](showcase/ALLEN_IBL_COORDINATE_MESH_VOLUME_PLAN.md)
7. [showcase/ALLEN_MOUSE_BRAIN_SLICE_EXAMPLE_PLAN.md](showcase/ALLEN_MOUSE_BRAIN_SLICE_EXAMPLE_PLAN.md)
8. [showcase/CHOROPLETH_GLOBE_EXAMPLE_PLAN.md](showcase/CHOROPLETH_GLOBE_EXAMPLE_PLAN.md)
9. [showcase/FLIGHT_TRAJECTORIES_DEMO_PLAN.md](showcase/FLIGHT_TRAJECTORIES_DEMO_PLAN.md)
10. [showcase/MARS_TEXTURED_MESH_EXAMPLE_PLAN.md](showcase/MARS_TEXTURED_MESH_EXAMPLE_PLAN.md)


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
