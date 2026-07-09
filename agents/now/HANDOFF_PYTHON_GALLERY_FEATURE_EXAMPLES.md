# Python Gallery Feature Examples Handoff

Status: active example-proof lane. Created: 2026-07-07. Updated: 2026-07-08.

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
10. `3949ab6b4` Add Python pixel selection gallery example.
11. `abe407e67` Add Python sphere selection gallery example.
12. `db53aca76` Add Python mesh instance selection gallery example.
13. `5198da3e5` Add Python label probe gallery example.
14. `c334f04a6` Add Python marker symbols gallery example.
15. `787cb7a72` Add Python builtin shapes 2D gallery example.
16. `9b49aba61` Add Python builtin shapes 3D gallery example.
17. `512f618e7` Add Python OBJ loading gallery example.
18. `e838f224e` Add Python isolines gallery example.
19. `c75166cc4` Add Python text block gallery example.
20. `1e0486463` Expose overlay card descriptors to Python.
21. `ef0e2cc44` Add Python overlay card gallery example.
22. `6445a1400` Add Python annotation readout gallery example.
23. `ceb5a5e1a` Add Python axis labels gallery example.

No Python gallery feature checkpoint is currently staged or in progress in this working tree. The
next planned checkpoint is `features_reference_grid`.

Known parity caveat from the geometry batch: the C OBJ and 3D shape examples apply a Phong
material, but Python `DvzMaterialDesc` has no generated fields in the current binding, so these
Python examples use mesh geometry colors without calling `dvz_visual_set_material()`.

The overlay-card checkpoint made `DvzOverlayCardDesc` and `DvzOverlayCardStyle` generated ctypes
layouts and removed `dvz_overlay_card_desc()` / `dvz_overlay_card_style()` from the expected skipped
function list. `just ctypes` and `just ctypes-check` passed after that binding-policy update.

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
python3 - <<'PY'
... construct selection_sphere scene, scan a small offscreen panel grid for one item query hit,
    apply hover and selection ...
PY
python3 - <<'PY'
... construct selection_mesh_instances scene, scan a small offscreen panel grid for one item query
    hit, apply hover and selection ...
PY
python3 - <<'PY'
... construct probe_labels scene, queue one offscreen segment query, verify categorical readback ...
PY
python3 - <<'PY'
... construct marker_symbols scene, register built-in/bitmap/SDF/MSDF/SVG symbols, run one
    offscreen frame ...
PY
python3 - <<'PY'
... construct builtin_shapes_2d scene, upload built-in geometry and an indexed holed polygon,
    render one offscreen frame ...
PY
python3 - <<'PY'
... construct builtin_shapes_3d scene, upload built-in cube/sphere/cylinder/cone/torus/arrow
    geometry, bind arcball, render one offscreen frame ...
PY
python3 - <<'PY'
... write compact OBJ fixture, load it with dvz_geometry_obj, bind arcball, render one offscreen
    frame ...
PY
python3 - <<'PY'
... construct surface-grid heights/colors, extract contour segments, bind arcball, render one
    offscreen frame ...
PY
python3 - <<'PY'
... construct retained text block, render one offscreen frame ...
PY
python3 - <<'PY'
... construct overlay card with view-coordinate signal, render one offscreen frame ...
PY
```

These returned `image_probe offscreen query smoke: 1 True` and
`picking offscreen query smoke: 1 True 57`, and
`selection_pixel offscreen query smoke: 1 True 480`, and
`selection_sphere offscreen query smoke: 1 True 18`, and
`selection_mesh_instances offscreen query smoke: 1 True 30`, and
`probe_labels offscreen query smoke: 1 True 31`, and
`marker_symbols offscreen smoke: OK`, and `builtin_shapes_2d offscreen smoke: OK`, and
`builtin_shapes_3d offscreen smoke: OK`, and `obj_loading offscreen smoke: OK`, and
`isolines offscreen smoke: OK`, and `text_block offscreen smoke: OK`, and
`overlay_card offscreen smoke: OK`. Live window and screenshot validation were not run for the
earlier checkpoint notes.

Current manifest ledger, recomputed from `examples/c/MANIFEST.yaml` on 2026-07-09 after
`features_axis_labels`:

- v0.4-required feature examples: 40 of 64 have Python entries; 24 remain missing.
- all v0.4-required public examples: 52 of 95 have Python entries; 43 remain missing.
- `features_bars_bands` is done: it has `examples/python/gallery/features/bars_bands.py` and a
  matching `python.source` manifest entry.
- `image_probe` is committed: it has `examples/python/gallery/features/image_probe.py` and a
  matching `python.source` manifest entry.
- `features_picking` is committed: it has `examples/python/gallery/features/picking.py` and a
  matching `python.source` manifest entry.
- `features_selection_pixel` is committed: it has
  `examples/python/gallery/features/selection_pixel.py` and a matching `python.source` manifest
  entry.
- `features_selection_sphere` is committed: it has
  `examples/python/gallery/features/selection_sphere.py` and a matching `python.source` manifest
  entry.
- `features_selection_mesh_instances` is committed: it has
  `examples/python/gallery/features/selection_mesh_instances.py` and a matching `python.source`
  manifest entry.
- `features_probe_labels` is committed: it has
  `examples/python/gallery/features/probe_labels.py` and a matching `python.source` manifest entry.
- `features_marker_symbols` is committed: it has
  `examples/python/gallery/features/marker_symbols.py` and a matching `python.source` manifest
  entry.
- `features_builtin_shapes_2d` is committed: it has
  `examples/python/gallery/features/builtin_shapes_2d.py` and a matching `python.source` manifest
  entry.
- `features_builtin_shapes_3d` is committed: it has
  `examples/python/gallery/features/builtin_shapes_3d.py` and a matching `python.source` manifest
  entry.
- `features_obj_loading` is committed: it has
  `examples/python/gallery/features/obj_loading.py` and a matching `python.source` manifest entry.
- `features_isolines` is committed: it has
  `examples/python/gallery/features/isolines.py` and a matching `python.source` manifest entry.
- `features_text_block` is committed: it has
  `examples/python/gallery/features/text_block.py` and a matching `python.source` manifest entry.
- `features_overlay_card` is committed: it has
  `examples/python/gallery/features/overlay_card.py` and a matching `python.source` manifest entry.
- `features_annotation_readout` is committed: it has
  `examples/python/gallery/features/annotation_readout.py` and a matching `python.source` manifest
  entry.
- `features_axis_labels` is committed: it has
  `examples/python/gallery/features/axis_labels.py` and a matching `python.source` manifest entry.


## Preferred Next Commit

The event/query/selection helper batch is complete for the planned feature examples. The geometry
and symbol helper batch has `features_marker_symbols`, `features_builtin_shapes_2d`, and
`features_builtin_shapes_3d`, `features_obj_loading`, and `features_isolines`; the first
text/annotation checkpoint has `features_text_block`, `features_overlay_card`, and
`features_annotation_readout`; and `features_axis_labels` is complete. Next target the small
spatial-layout checkpoint, starting with `features_reference_grid`. `features_coordinate_system`
remains the first missing feature in manifest order, but it involves 3D geometry/material parity
caveats and can follow after the smaller reference-grid route.

Implementation shape:

1. Read the target C feature example and existing Python gallery helpers before adding new Python
   support.
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
examples: add Python reference grid gallery example
```

Use one commit for helper plus example. Split binding facade/generator changes from later example
additions if regenerated files make review clearer.


## Remaining Batch Order

Work in small helper-first batches. Each batch should add the minimal reusable Python support, then
convert the examples that immediately need it.

1. **Event, query, and selection helpers**

   Target examples: guide-line/span interaction if needed, `image_probe`, `features_picking`,
   `features_selection_pixel`, `features_selection_sphere`, `features_selection_mesh_instances`,
   `features_probe_labels`, and `showcases_linked_probe_colorbar`.

   Preferred shape: small common helpers in `examples/python/gallery/common.py` for frame callback,
   request/query readback, deterministic pick/probe inputs, and windowless/offscreen-safe scenario
   structure. Keep browser/WebGPU semantics in C/WASM; Python examples should remain direct engine
   usage, not a reimplementation of the browser host.

2. **Geometry and symbol helpers**

   Target examples: `features_builtin_shapes_2d`, `features_builtin_shapes_3d`,
   `features_marker_symbols`, `features_obj_loading`, and likely `features_isolines`.

   Preferred shape: reusable NumPy geometry construction helpers only where the C example already
   depends on generated arrays or built-in geometry. Do not introduce a Python geometry API that
   promises more than the C engine exposes.

3. **Text, annotation, legend, and GUI examples**

   Target examples: `features_text_block`, `features_overlay_card`, `features_annotation_readout`,
   `features_legend_categorical`, `features_gui_controls`, `features_gui_viewport`, and
   `features_gui_cimgui`.

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

`features_coordinate_system`, `features_axis_labels`, `features_orientation_gizmo`,
`features_reference_grid`, `features_bounds_overlay`, `features_controller_fly`,
`features_mesh_texture`, `features_material_mesh`, `features_lighting`, `features_user_scale`,
`features_gui_controls`, `features_gui_viewport`, `features_gui_cimgui`,
`features_animation_tracks`, `features_technique_ssao`, `features_technique_msaa`,
`features_technique_depth_cue`, `features_technique_transparency`, `features_input_events`,
`features_view_size_policies`, `features_bezier_curve_path`, `features_path_join`,
`features_scalebar`, `features_scalebar_units`, `features_annotation_readout`, and
`features_datetime_axis`.


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
