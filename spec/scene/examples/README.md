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

Cross-cutting lessons that come out of several examples can live at this level. The current note on
volume, slice, and transparent mesh composition is
[TRANSPARENCY_OCCLUSION.md](TRANSPARENCY_OCCLUSION.md).

Future scientific-visualization directions that extend beyond the current example set are tracked in
[`../proposals/SCIENTIFIC_VISUALIZATION_ROADMAP.md`](../proposals/SCIENTIFIC_VISUALIZATION_ROADMAP.md).
That roadmap links graph/network, unstructured-grid, field, trajectory, ensemble, molecular, and
out-of-core resource proposals. Existing examples should reuse those notes instead of inventing
parallel semantics when they grow into those domains.



## Feature Fixture Matrix

The compact one-feature fixture inventory lives in
[FEATURE_FIXTURE_MATRIX.md](FEATURE_FIXTURE_MATRIX.md). That matrix is the canonical planning list for
minimal C examples, generated DRP2/WebGPU fixtures, GUI/cimgui fixtures, video fixtures, and advanced
low-level `vk`/`vklite`/`canvas`/`stream` examples.

Fixture examples should stay smaller than the worked examples in this directory: one primary feature,
synthetic in-file data where possible, deterministic offscreen artifacts when practical, and explicit
CMake/backend gates for GLFW, GUI, CUDA/NVENC, Kvazaar, WebGPU, or platform interop requirements.

## Example Suite Organization

The cross-repo organization plan for Datoviz C examples, Datoviz raw Python examples, vispy2
GSP examples, vispy2 plot examples, fixtures, showcases, regression examples, and stress tests
lives in [EXAMPLE_ORGANIZATION.md](EXAMPLE_ORGANIZATION.md).

Use [TEMPLATE.md](TEMPLATE.md) for new example specs. Existing examples should keep their
domain-specific detail, but each file should provide enough information for an agent to pick up the
implementation without asking for missing data-planning context. In practice, every example should
answer:

1. what artifact to implement,
2. what data to use, download, synthesize, or cache,
3. what preprocessing script is needed, if any,
4. which scene visuals, panels, resources, transforms, and interactions are required,
5. which Datoviz capabilities are mandatory versus optional,
6. how to validate the result with a smoke command, screenshot/readback check, fixture, or manual
   interaction checklist.


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
10. [core/ANIMATION_VIDEO_EXPORT.md](core/ANIMATION_VIDEO_EXPORT.md)
11. [core/LATEX_MICROTEX_TEXT_VISUAL.md](core/LATEX_MICROTEX_TEXT_VISUAL.md)

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

### `astronomy/`

Astronomy and astrophysics examples.

1. [astronomy/ASTRONOMY_MANY_LABELS.md](astronomy/ASTRONOMY_MANY_LABELS.md)
2. [astronomy/GALAXY.md](astronomy/GALAXY.md)

### `bio/`

Biomedical, molecular, and physiology examples.

1. [bio/PHYSIOLOGY_SIGNAL_WORKBENCH.md](bio/PHYSIOLOGY_SIGNAL_WORKBENCH.md)
2. [bio/PROTEIN_ARCBALL_VIEWER.md](bio/PROTEIN_ARCBALL_VIEWER.md)

### `engineering/`

Engineering simulation and post-processing examples.

1. [engineering/FINITE_ELEMENT_STRESS_VIEWER.md](engineering/FINITE_ELEMENT_STRESS_VIEWER.md)

### `materials/`

Materials-science examples.

1. [materials/CRYSTAL_PHONON_EXPLORER.md](materials/CRYSTAL_PHONON_EXPLORER.md)

### `neuro/`

Neuroscience, brain-atlas, and tractography examples.

1. [neuro/ALLEN_IBL_COORDINATE_MESH_VOLUME_PLAN.md](neuro/ALLEN_IBL_COORDINATE_MESH_VOLUME_PLAN.md)
2. [neuro/ALLEN_MOUSE_BRAIN_SLICE_EXAMPLE_PLAN.md](neuro/ALLEN_MOUSE_BRAIN_SLICE_EXAMPLE_PLAN.md)
3. [neuro/DIFFUSION_TRACTOGRAPHY.md](neuro/DIFFUSION_TRACTOGRAPHY.md)

### `physics/`

Physics, field-line, fluid, plasma, and event-display examples.

1. [physics/CFD_VORTICITY_ADVECTION.md](physics/CFD_VORTICITY_ADVECTION.md)
2. [physics/HEP_EVENT_DISPLAY.md](physics/HEP_EVENT_DISPLAY.md)
3. [physics/TOKAMAK_PLASMA_FIELD_LINES.md](physics/TOKAMAK_PLASMA_FIELD_LINES.md)

### `geo/`

Geographic, projection, terrain, atmosphere, and globe examples.

This includes Earth, Mars, wind, flight, animal migration, choropleth, earthquake, and terrain
examples.

### `dashboards/`

Operational or dense-interface examples with multiple coordinated panels.

This includes market, DAQ, DICOM, and image-embedding explorer examples.


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
