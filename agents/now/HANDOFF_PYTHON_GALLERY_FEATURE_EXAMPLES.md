# Python Gallery Feature Examples Handoff

Status: active example-proof lane. Created: 2026-07-07.

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

The last validation loop was:

```sh
python3 tools/build_gallery.py
python3 tools/build_examples_manifest.py
python3 tools/check_example_manifests.py
python3 -m py_compile $(find examples/python/gallery -name '*.py' -print)
git diff --check
```

Live window and screenshot validation were not run.

Last known ledger for v0.4-required examples: 22 have Python entries, 42 remain missing. Recompute
from `examples/c/MANIFEST.yaml` before the next batch, because this count is moving quickly.


## Preferred Next Commit

Start with `feature_bars_bands`. It is the best next example because the missing Python support is
specific and reusable: array adaptation for multi-array plot setters.

Implementation shape:

1. Read `spec/bindings/ARRAY_FACADE.md`, `spec/bindings/CTYPES_POLICY.md`, and the current
   `spec/bindings/ctypes.yml` array facade policy.
2. Add explicit array facade policy, and generator support if needed, for:
   - `dvz_bars_set_intervals(DvzBars*, starts, ends, values, count)`;
   - `dvz_band_set_bounds(DvzBand*, x, lower, upper, count)`;
   - `dvz_band_set_center(DvzBand*, x, y, count)`.
3. Keep all three APIs as direct `dvz_*` calls. Do not add Python object wrappers, plotting
   aliases, or feature-specific helper functions unless the existing gallery common helpers already
   establish that pattern.
4. Regenerate and validate local bindings:

   ```sh
   just ctypes
   just ctypes-check
   ```

5. Add `examples/python/gallery/features/bars_bands.py` and the matching manifest `python.source`
   entry for `feature_bars_bands`.
6. Validate with the standard narrow loop:

   ```sh
   python3 tools/build_gallery.py
   python3 tools/build_examples_manifest.py
   python3 tools/check_example_manifests.py
   python3 -m py_compile $(find examples/python/gallery -name '*.py' -print)
   git diff --check
   ```

Suggested checkpoint commit:

```text
bindings: add plot array facades for Python gallery
examples: add Python bars and bands gallery example
```

Use one commit if the facade and example are small; split them if generator changes or binding
regeneration noise makes review clearer.


## Batch Order After Bars/Bands

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
