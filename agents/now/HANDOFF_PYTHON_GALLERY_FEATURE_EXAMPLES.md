# Python Gallery Feature Examples Handoff

Status: active example-proof lane. Created: 2026-07-07. Updated: 2026-07-07.

This handoff continues the Python gallery example lane for v0.4-required examples. The goal is one
clear top-level Python entry per public v0.4 example wherever the direct `datoviz` binding can
faithfully express the C scenario without inventing a high-level plotting layer.


## Current State

Recent checkpoint commits landed:

1. `0f1e45585` Add Python timer animation gallery example.
2. `7b336c45a` Add Python visual update gallery examples.
3. `96a332244` Add Python visual transform gallery example.
4. `a4be48e2b` Add Python depth test gallery example.
5. `292f8fa50` Add Python panel View2D gallery example.
6. `6382cc00` Add Python bars and bands gallery example.
7. `d2621425e` Add Python guide gallery examples.
8. `24e6e2eb8` Add Python image probe gallery example.
9. `33e77c2de` Add Python picking gallery example.

Current uncommitted checkpoint in this working tree:

1. Added `examples/python/gallery/features/selection_pixel.py`.
2. Added `feature_selection_pixel.python.source` / `direct-engine` in
   `examples/c/MANIFEST.yaml` and
   regenerated gallery metadata.

The last validation loop was:

```sh
python3 tools/build_gallery.py
python3 tools/build_examples_manifest.py
python3 tools/check_example_manifests.py
python3 -m py_compile $(find examples/python/gallery -name '*.py' -print)
git diff --check
```

Additional local validation for recent checkpoints:

```sh
python3 - <<'PY'
... construct image_probe scene, queue one offscreen pixel query, run one offscreen frame ...
PY
python3 - <<'PY'
... construct picking scene, queue one offscreen item query, apply hover and selection ...
PY
python3 - <<'PY'
... construct selection_pixel scene, queue one offscreen item query, apply hover and selection ...
PY
```

These returned `image_probe offscreen query smoke: 1 True` and
`picking offscreen query smoke: 1 True 57`, and
`selection_pixel offscreen query smoke: 1 True 480`. Live window and screenshot validation were not
run for the earlier checkpoint notes.

Current manifest ledger, recomputed from `examples/c/MANIFEST.yaml` on 2026-07-07:

- v0.4-required feature examples: 28 of 64 have Python entries; 36 remain missing.
- all v0.4-required public examples: 40 of 95 have Python entries; 55 remain missing.
- `feature_bars_bands` is done: it has `examples/python/gallery/features/bars_bands.py` and a
  matching `python.source` manifest entry.
- `image_probe` is committed: it has `examples/python/gallery/features/image_probe.py` and a
  matching `python.source` manifest entry.
- `feature_picking` is committed: it has `examples/python/gallery/features/picking.py` and a
  matching `python.source` manifest entry.
- `feature_selection_pixel` is done in the current working tree: it has
  `examples/python/gallery/features/selection_pixel.py` and a matching `python.source` manifest
  entry.


## Preferred Next Commit

Continue the event/query/selection helper batch using the helper path created for `image_probe`.
Next target `feature_selection_sphere`, then `feature_selection_mesh_instances`, and
`feature_probe_labels`.

Implementation shape:

1. Read `examples/c/features/image_probe.c`, the relevant query/selection C examples, and existing
   Python gallery helpers before adding new Python support.
2. Add the minimal shared helpers in `examples/python/gallery/common.py` only when at least two
   examples need them.
3. Keep examples as direct `dvz_*` usage. Do not add high-level plotting wrappers, query wrappers,
   or browser/WebGPU behavior in Python.
4. If a public binding gap blocks direct engine usage, first read `spec/bindings/ARRAY_FACADE.md`,
   `spec/bindings/CTYPES_POLICY.md`, and `spec/bindings/ctypes.yml`; then add the narrowest facade
   policy or generator support needed.
5. After public headers, exported API, binding policy, or binding generator changes, regenerate and
   validate local bindings:

   ```sh
   just ctypes
   just ctypes-check
   ```

6. Add Python gallery files and matching manifest `python.source` entries for each converted
   example.
7. Validate with the standard narrow loop:

   ```sh
   python3 tools/build_gallery.py
   python3 tools/build_examples_manifest.py
   python3 tools/check_example_manifests.py
   python3 -m py_compile $(find examples/python/gallery -name '*.py' -print)
   git diff --check
   ```

Suggested checkpoint commit for the current working tree:

```text
examples: add Python pixel selection gallery example
```

Use one commit for helper plus example. Split binding facade/generator changes from later example
additions if regenerated files make review clearer.


## Remaining Batch Order

Work in small helper-first batches. Each batch should add the minimal reusable Python support, then
convert the examples that immediately need it.

1. **Event, query, and selection helpers**

   Target examples: guide-line/span interaction if needed, `image_probe`, `feature_picking`,
   `feature_selection_pixel`, `feature_selection_sphere`, `feature_selection_mesh_instances`,
   `feature_probe_labels`, and `linked_panels_probe_colorbar`.

   Preferred shape: small common helpers in `examples/python/gallery/common.py` for frame callback,
   request/query readback, deterministic pick/probe inputs, and windowless/offscreen-safe scenario
   structure. Keep browser/WebGPU semantics in C/WASM; Python examples should remain direct engine
   usage, not a reimplementation of the browser host.

2. **Geometry and symbol helpers**

   Target examples: `feature_builtin_shapes_2d`, `feature_builtin_shapes_3d`,
   `feature_marker_symbols`, `feature_obj_loading`, and likely `feature_isolines`.

   Preferred shape: reusable NumPy geometry construction helpers only where the C example already
   depends on generated arrays or built-in geometry. Do not introduce a Python geometry API that
   promises more than the C engine exposes.

3. **Text, annotation, legend, and GUI examples**

   Target examples: `feature_text_block`, `feature_overlay_card`, `annotation_readout`,
   `feature_legend_categorical`, `feature_gui_controls`, `feature_gui_viewport`, and
   `feature_gui_cimgui`.

   Preferred shape: thin binding facades for text/annotation data setters where needed, plus compact
   common data builders for repeated label/color arrays. GUI examples may need native-window
   validation and can stay later in the lane if the current environment cannot prove them.

4. **Rendering technique and runtime edge cases**

   Target examples: MSAA, SSAO, EDL, depth cue, transparency, volume occlusion, app/window runtime,
   record/replay, video export, and other examples whose Python path depends on native presentation
   or capture plumbing.

   Preferred shape: classify honestly before coding. Some runtime examples may deserve Python
   entries; others may remain C/native-only or require a narrower public Python runtime helper first.

5. **Large data and composed showcases**

   Target examples: composed polygons/graphs, real-data showcases, brain/volume/terrain/point-cloud
   examples, protein, wind field, embedding atlas, and GPU particles.

   Preferred shape: only convert when the data contract, preparation command, and manifest are
   already clear. Do not silently synthesize fallback data for examples that declare prepared or
   external datasets.


## Missing Required Feature Entries

Current missing `v0.4_required` feature examples with no `python.source` entry:

`feature_coordinate_system`, `feature_axis_labels`, `feature_text_block`,
`feature_overlay_card`, `feature_orientation_gizmo`, `feature_reference_grid`,
`feature_bounds_overlay`, `feature_controller_fly`, `feature_mesh_texture`,
`feature_material_mesh`, `feature_lighting`, `feature_user_scale`, `feature_gui_controls`,
`feature_gui_viewport`, `feature_gui_cimgui`, `feature_animation_tracks`, `technique_ssao`,
`technique_msaa`, `technique_depth_cue`, `technique_transparency`, `feature_input_events`,
`feature_view_size_policies`, `feature_bezier_curve_path`, `feature_path_join`, `scale_bar`,
`scalebar_units`, `annotation_readout`, `feature_selection_sphere`,
`feature_selection_mesh_instances`, `feature_isolines`,
`feature_builtin_shapes_2d`, `feature_builtin_shapes_3d`, `feature_obj_loading`,
`feature_probe_labels`, `feature_datetime_axis`, and `feature_marker_symbols`.


## Per-Example Checklist

For each Python gallery example:

1. Use `import datoviz as dvz`, not `datoviz.raw`, unless the example is deliberately about exact
   FFI calls.
2. Mirror the canonical C example's scene semantics and data contract.
3. Put shared helpers in `examples/python/gallery/common.py` only when at least two examples need
   them.
4. Add or update only the manifest fields needed for `python.source`.
5. Rebuild generated gallery/example manifests with the repo tools; do not hand-edit generated
   gallery pages.
6. Keep checkpoint commits narrow and leave unrelated worktree changes untouched.


## Validation Defaults

For a pure Python example conversion:

```sh
python3 tools/build_gallery.py
python3 tools/build_examples_manifest.py
python3 tools/check_example_manifests.py
python3 -m py_compile $(find examples/python/gallery -name '*.py' -print)
git diff --check
```

After public headers, exported API, binding policy, or binding generator changes:

```sh
just ctypes
just ctypes-check
python3 tools/build_gallery.py
python3 tools/build_examples_manifest.py
python3 tools/check_example_manifests.py
python3 -m py_compile $(find examples/python/gallery -name '*.py' -print)
git diff --check
```

Run live window or screenshot validation only when the touched example depends on runtime
presentation, native input, picking, GUI, capture, or visual output details that py-compile and
manifest checks cannot prove.
