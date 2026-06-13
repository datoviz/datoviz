# Selection and Picking Follow-up Plan

Status: active investigation handoff. Created: 2026-06-13.

Context: `features/selection_pixel` mirrors Y during hover, while
`features/selection_sphere` and `features/selection_mesh_instances` appear ineffective in live
interaction. Static inspection and focused query tests suggest the family query paths mostly work,
but the shared query coordinate path and example click wiring need hardening.


## Preferred Sequence

1. Add focused regression tests for query Y orientation:
   - two vertically separated pixel items;
   - query near the top and bottom;
   - assert returned item IDs are not mirrored.
2. Fix the shared query transform in `src/scene/query/scratch.c`, not the examples.
   Keep public/panel query coordinates top-origin and adjust only the MVP recentering used by 1x1
   readback passes.
3. Align `examples/c/features/selection_sphere.c` with pixel/mesh click behavior:
   - keep hover for hover styling;
   - queue a separate click query on press;
   - toggle or clear selection from the click query result.
4. Re-run narrow validation:
   - `just test query`;
   - `just example-c features/selection_pixel`;
   - `just example-c features/selection_sphere`;
   - `just example-c features/selection_mesh_instances`;
   - `git diff --check`.
5. If example builds still fail on the current `panel_view.c` / `panel_geometry.c` link errors,
   report that as a separate blocker unless the user explicitly broadens scope.


## Investigation Notes

- `scene/query/sphere_query_resolves_item`, `scene/query/mesh_query_resolves_item`, and
  `scene/query/mesh_query_resolves_instance_item` passed during investigation, so raw GPU item
  readback for those families is not completely missing.
- The observed pixel Y mirroring should be fixed centrally. Do not compensate in individual
  examples by flipping event coordinates.
- `selection_sphere.c` currently toggles from the latest hover query on click; that can make
  selection feel dead when hover is stale, flipped, or absent. Use a dedicated click query as in
  pixel and mesh.
