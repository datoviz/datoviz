# Finite-Element Stress Viewer

> **Example status:** informative pressure test
> **Target:** C showcase plus optional preparation script
> **Data:** generated/prepared FEA cache with real-solver import later
> **Validation:** smoke, visual/picking/performance checklist

See [../SHARED_POLICIES.md](../SHARED_POLICIES.md) for shared worked-example policy.


## Summary

Render a finite-element result as an interactive deformed 3D mesh with stress coloring, undeformed
reference geometry, load/deformation controls, and element or node readout. The first slice should
use a generated or prepared cantilever/bracket cache and update deformed positions/colors without
rebuilding mesh indices.


## User-Visible Result

- A 3D arcball viewport shows a deformed mesh colored by von Mises stress.
- A faint undeformed wireframe/ghost mesh, fixed support markers, load arrows, and scalar colorbar
  provide engineering context.
- Load-factor and deformation-scale controls update geometry and colors.
- Picking a face/element/node highlights it and reports engineering values.
- A later diagnostics panel links load factor to tip displacement, max stress, selected stress, or
  strain energy.


## Feature Pressure Points

- Indexed mesh rendering with stable topology and dynamic vertex positions.
- Per-vertex or per-face scalar coloring and linked colorbar semantics.
- Undeformed ghost/wireframe overlays, support/load markers, and selection highlights.
- Face/element/node picking that resolves stable semantic ids.
- Load-step animation and diagnostics without full mesh rebuilds.


## Required Data And Resources

Default scenario: a cantilever beam or perforated bracket with one fixed end, visible load near the
free end, exaggerated deformation, stress concentration near the support/hole/corner, and load
factor from `0` to `1`.

Suggested cache:

```text
~/.cache/datoviz/fea/cantilever_bracket/
  metadata.json
  nodes_f32.bin
  surface_indices_u32.bin
  element_id_u32.bin
  displacement_f32.bin
  stress_f32.bin
  boundary_flags_u8.bin
  load_vectors_f32.bin
  diagnostics_f32.bin
```

Metadata should record case name, solver/generator, node/triangle/element/step counts, units,
stress/displacement units, field location, diagnostic names, cache version, and preparation script.

Later importers may convert CalculiX, FEniCS, Code_Aster, Abaqus exports, or custom solver output
into this schema. Runtime should still consume prepared arrays rather than link to a solver.

If no cache is available, generate deterministic cantilever-like geometry with plausible bending
displacement, stress concentration, boundary flags, load vectors, and optional load steps.


## Runtime Data Model

Source arrays:

```text
rest_position        float32[n_nodes, 3]
surface_index        uint32[n_triangles, 3]
element_id_per_face  uint32[n_triangles]
displacement         float32[n_steps, n_nodes, 3]
stress               float32[n_steps, n_values]
boundary_flags       uint8[n_nodes or n_elements]
```

Derived render arrays:

```text
deformed_position    float32[n_nodes, 3]
stress_color         uint8[n_nodes or n_faces, 4]
ghost_position       float32[n_nodes, 3]
selection_geometry   optional highlight outline or overlay
load_arrow_geometry  optional path or primitive arrows
```

Mesh topology remains stable across load steps. Load changes update positions/colors and diagnostic
cursor resources, not indices.


## Scene Layout

Minimum viable layout:

```text
3D FEA viewport: deformed mesh, undeformed ghost, supports, load arrows, selection
```

Preferred layout:

```text
3D FEA viewport
diagnostics panel: load factor, tip displacement, max stress, selected stress
```

Default color mode is von Mises stress. Alternative modes: displacement magnitude, principal stress,
strain energy density, element/material id, part id, or selection.


## Interactivity And Readout

Required interactions:

- arcball/orbit camera,
- load factor slider,
- deformation scale slider,
- color mode selector,
- show/hide ghost mesh,
- show/hide support/load markers,
- click selection,
- selected item readout.

Preferred additions: load sweep animation, hover probe, linked diagnostics selection, max-stress
marker, reset camera, isolate boundary regions, switch load cases, clipping/cross-section, modal
oscillation, and side-by-side comparisons.

Readout should include face id, element id, node ids, position, load factor, displacement vector and
magnitude, stress value, field units, and boundary/load flag.


## Scene And Runtime Behavior

Use the normal scene pipeline; see [../../pipeline/FRAME_PLAN.md](../../pipeline/FRAME_PLAN.md) and
[../../drp2/](../../drp2/).

- Initial setup uploads mesh positions/indices, stress colors, ghost geometry, support/load markers,
  and diagnostics curves.
- Load factor or deformation scale updates deformed positions, stress colors when step-dependent,
  and diagnostics cursor.
- Color-mode changes update scalar colors and colorbar resources.
- Selection updates highlight geometry/style and diagnostics marker without full mesh upload.


## Minimal Implementation Target

1. Generate or load one cantilever/bracket case.
2. Render deformed mesh and undeformed ghost.
3. Expose load factor and deformation scale.
4. Color by one scalar stress field.
5. Implement face/element picking when the current mesh-pick path is available.
6. Add diagnostics after the 3D interaction is stable.


## Validation

- Smoke run opens nonblank 3D viewport, changes load/deformation, selects one item, and tears down.
- Deformation and stress colors update without index rebuild.
- Undeformed ghost, supports, load arrows, and colorbar remain aligned with the mesh.
- Picking resolves a stable semantic element/face/node id.
- Load sweep does not allocate unbounded transient resources or recreate static topology.
