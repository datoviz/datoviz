# Finite-Element Stress Viewer

> **Agent Pickup**
> - **Category:** `engineering`
> - **Implementation target:** Scientific domain example with staged implementation, starting from deterministic or prepared data.
> - **Data policy:** Synthetic or prepared-cache first slice; real public data may be a second stage with license notes.
> - **Preprocessing:** Document any data conversion script, output schema, coordinate normalization, and cache validation.
> - **Validation:** Smoke run plus domain-specific visual, picking/probe, and performance acceptance criteria.

## Summary

Build an engineering post-processing scene example that displays a finite-element result as an
interactive deformed 3D mesh with stress coloring, undeformed reference geometry, load controls,
and element or node readout. The first data path should use a generated or prepared cantilever or
bracket cache with rest nodes, stable surface indices, displacement, stress, boundary flags, load
vectors, and optional diagnostics; real solver exports can be converted later into the same schema.
The first practical slice should render one arcball 3D viewport, update deformed positions and
stress colors without rebuilding indices, expose load/deformation controls, and report a picked face
or element. Validate with smoke execution, FEA-specific visual checks, picking/probe checks, and
staged performance criteria.


## Example Name

`FINITE_ELEMENT_STRESS_VIEWER`


## Purpose

Specify a Datoviz v0.4 showcase example for engineering simulation post-processing. The example
renders a finite-element result as an interactive deformed 3D mesh with stress coloring, load
controls, element picking, and linked diagnostics.

This should not be a generic mesh demo. The scientific and engineering identity is:

```text
finite-element model -> displacement field -> deformed mesh -> stress colormap -> selected element readout
```


## Why This Example Exists

This example fills a gap in the showcase set: structural mechanics and engineering simulation.

It should pressure:

1. indexed mesh rendering with stable topology,
2. per-vertex or per-face scalar coloring,
3. deformation updates without rebuilding mesh indices,
4. 3D arcball inspection,
5. undeformed ghost/wireframe overlays,
6. colorbars and scalar scale semantics,
7. face/element/node picking,
8. linked load, displacement, and stress diagnostics.

The example should look like a compact FEA post-processor rather than a simple lit mesh viewer.


## Recommended Default Scenario

Use a cantilever beam or bracket under load as the Stage 1 default.

Rationale:

- the physical setup is immediately understandable,
- fixed boundary and applied load are visually clear,
- deformation is obvious when exaggerated,
- stress concentration near the support is expected,
- the mesh and fields can be generated or bundled without requiring an external solver.

Preferred default:

- a perforated bracket or cantilever beam with one fixed end,
- downward or lateral force applied near the free end,
- von Mises stress coloring,
- undeformed ghost mesh,
- load-factor animation from `0` to `1`.


## User-Facing Scenario

The default scene should show:

- a 3D deformed mesh colored by von Mises stress,
- a faint undeformed wireframe or ghost mesh,
- fixed support markers,
- load arrows,
- a scalar colorbar,
- an arcball camera,
- a load-factor slider,
- a deformation-scale slider,
- click selection of an element or node,
- a linked diagnostics panel.

The strongest screenshot should clearly show the colored deformed structure and the faint original
shape behind it.


## Scene Layout

Recommended layout:

```text
+------------------------------------------------------------------+
| 3D FEA viewport                                                   |
| deformed mesh, undeformed ghost, supports, load arrows, selection |
+------------------------------------------------------------------+
| diagnostics panel                                                 |
| load factor / tip displacement / max stress / selected stress      |
+------------------------------------------------------------------+
```

Minimum viable version:

1. one 3D panel with arcball interaction,
2. deformed mesh visual,
3. stress colormap,
4. undeformed ghost or wireframe,
5. load-factor and deformation-scale controls,
6. element or face picking readout.

Preferred fuller version:

1. linked diagnostics panel,
2. selected element highlight,
3. fixed-boundary and load-arrow overlays,
4. multiple scalar color modes,
5. modal or load-step animation,
6. optional clipping/cross-section plane.


## Data Strategy

### Stage 1: Generated Or Prepared FEA-Like Cache

The first implementation should not require a full finite-element solver at runtime. It may use a
generated mesh and deterministic synthetic displacement/stress fields, or load a compact prepared
cache.

Suggested cache layout:

```text
~/.cache/datoviz/fea/cantilever_bracket/
  metadata.json
  nodes_f32.bin             # n_nodes x 3 rest positions
  surface_indices_u32.bin   # triangle indices for rendered surface
  element_id_u32.bin        # one element id per rendered face, optional
  displacement_f32.bin      # n_steps x n_nodes x 3, or n_nodes x 3
  stress_f32.bin            # n_steps x n_nodes or n_steps x n_elements
  boundary_flags_u8.bin     # fixed/load/normal node or element flags
  load_vectors_f32.bin      # optional load arrow anchors and vectors
  diagnostics_f32.bin       # optional n_steps x n_metrics
```

Recommended metadata:

```text
case_name
solver_or_generator
n_nodes
n_surface_triangles
n_elements
n_steps
units
stress_units
displacement_units
field_location       # node or element
diagnostic_names
```


### Stage 2: Real Solver Output

A later preparation script may convert real FEA output from tools such as CalculiX, FEniCS,
Code_Aster, Abaqus export files, or custom simulation data into the cache schema above.

Runtime should still consume prepared arrays rather than linking to a solver.


### Synthetic Fallback

If no cache is available, generate a deterministic cantilever-like mesh and fields:

- rectangular or bracket-like geometry,
- fixed nodes near one end,
- displacement increasing toward the free end,
- bending-shaped deformation,
- stress higher near the fixed support and around holes or corners,
- optional several load steps from `0` to `1`.

The fallback should be visually plausible and useful for testing interaction, even if it is not a
validated finite-element solution.


## Runtime Data Model

Logical source data:

```text
rest_position        float32[n_nodes, 3]
surface_index        uint32[n_triangles, 3]
element_id_per_face  uint32[n_triangles]
displacement         float32[n_steps, n_nodes, 3]
stress               float32[n_steps, n_values]
boundary_flags       uint8[n_nodes or n_elements]
```

Derived render data:

```text
deformed_position    float32[n_nodes, 3]
stress_color         uint8[n_nodes or n_faces, 4]
ghost_position       float32[n_nodes, 3]
selection_geometry   optional highlight outline or overlay
load_arrow_geometry  optional path or primitive arrows
```

The mesh topology should remain stable across load steps. Load changes should update vertex
positions and colors, not recreate indices.


## Visual Encodings

FEA viewport:

```text
deformed mesh        solid mesh visual, lit if available
stress               diverging or sequential scientific colormap
undeformed mesh      faint wireframe, transparent mesh, or path overlay
fixed support        colored markers or boundary face tint
load vector          arrows or line primitives
selected element     outline, tint, or overlay marker
```

Default color mode:

- von Mises stress.

Alternative color modes:

- displacement magnitude,
- principal stress,
- strain energy density,
- element id or material region,
- selection/part id.

The scalar colorbar should display field name, range, and units when available.


## Interactivity

The example should be interactive by design.

### MVP Interactivity

Required controls and interactions:

1. arcball/orbit camera,
2. load factor slider,
3. deformation scale slider,
4. color mode selector,
5. show/hide undeformed ghost mesh,
6. show/hide support and load markers,
7. click selection of a face, element, or node,
8. selected item readout.

The central interaction should be:

```text
load slider -> deformed vertex positions + stress colors update
element pick -> highlighted element + linked readout update
```


### Preferred Interactivity

Preferred full-version interactions:

1. play/pause load sweep animation,
2. hover probe with live element/node values,
3. linked diagnostics panel selection,
4. max-stress marker,
5. reset camera,
6. isolate or highlight boundary regions,
7. switch between load cases.


### Advanced Interactivity

Advanced features may include:

1. clipping plane or cross-section,
2. element-set selection,
3. modal shape oscillation,
4. multiple materials or part regions,
5. mesh-quality view,
6. comparative side-by-side load cases.


## Picking And Readout

Picking should identify a face, then resolve it to the owning element when element ids are
available.

Readout should include:

```text
face id
element id
node ids
position
load factor
displacement vector
displacement magnitude
stress value
field units
boundary/load flag
```

The selected element should be highlighted in the 3D panel and in linked diagnostics when relevant.


## Diagnostics Panel

The linked diagnostics panel should show one or more engineering curves:

```text
load factor vs tip displacement
load factor vs max von Mises stress
load factor vs selected element stress
load factor vs strain energy
```

The current load factor should appear as a vertical cursor. Selecting a point in the diagnostics
panel should update the load factor in the 3D view.


## Animation Modes

Load sweep:

```text
position(t) = rest + deformation_scale * load_factor(t) * displacement
```

For multiple precomputed load steps, interpolate between neighboring steps.

Modal oscillation, optional:

```text
position(t) = rest + deformation_scale * mode_shape * sin(omega * t)
```

Modal animation is useful for a later vibration example, but the first implementation should focus
on static/load-step stress.


## FramePlan Shape

### Static Setup

Initial frame:

```text
UploadNode  -> rest/deformed mesh positions
UploadNode  -> mesh indices
UploadNode  -> stress colors
UploadNode  -> undeformed ghost geometry
UploadNode  -> support and load marker geometry
UploadNode  -> diagnostics panel data
RenderNode  -> 3D FEA panel
RenderNode  -> diagnostics panel
```


### Load Factor Or Deformation Scale Change

When load factor or deformation scale changes:

```text
UploadNode  -> deformed vertex positions
UploadNode  -> stress colors, if stress changes with load step
UploadNode  -> diagnostics cursor geometry
RenderNode  -> affected panels
```

Mesh indices, rest geometry, support markers, and static diagnostics curves should remain stable.


### Color Mode Change

When scalar color mode changes:

```text
UploadNode  -> scalar colors or style/colormap parameters
UploadNode  -> colorbar derived resources
RenderNode  -> 3D FEA panel
RenderNode  -> colorbar/annotation layer
```


### Selection Change

When an element, face, or node is selected:

```text
UploadNode  -> selection highlight geometry or selection style buffer
UploadNode  -> linked diagnostics highlight
RenderNode  -> affected panels
```

No full mesh reupload should be required for selection-only changes.


## DRP2 Command Categories

The example is expected to require:

- mesh/index buffers for the finite-element surface,
- dynamic buffer uploads for deformed vertex positions and scalar colors,
- path or primitive buffers for ghost wireframes, supports, load arrows, and highlights,
- panel transform updates from the arcball controller,
- colorbar and annotation draw commands,
- readback/pick requests for face/element selection,
- optional capture/video commands through the app/canvas layer.


## Implementation Notes

The first C implementation can stay focused:

1. generate or load one cantilever/bracket case,
2. render the deformed mesh and undeformed ghost,
3. expose load factor and deformation scale,
4. color by one scalar stress field,
5. implement face/element picking when the current mesh-pick path is available,
6. add diagnostics after the 3D interaction is stable.

The implementation should keep ownership explicit: rest mesh topology is static, dynamic arrays are
owned by the example/runtime object, and element ids are stable across updates.


## Key Pressure On The Scene Spec

This example checks that Datoviz v0.4 can express an engineering simulation viewer where:

- a stable mesh topology has dynamic positions and dynamic scalar colors,
- deformation scale and load factor affect geometry without changing ownership,
- scalar colorbars remain linked to the active engineering field,
- face picking can resolve a semantic element id,
- linked diagnostics update without forcing a full mesh rebuild.
