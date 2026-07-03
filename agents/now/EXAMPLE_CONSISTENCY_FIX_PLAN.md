# Example Consistency Fix Plan

Status: active plan. Scope: public example consistency cleanup for v0.4 RC preparation.

This plan covers consistency fixes found in the July 2026 audit of public examples, WebGPU example
metadata, Python/Qt examples, integration examples, and benchmark examples. Leave
`examples/c/runtime/multi_window.c` alone; another agent owns that file.


## Goals

1. Make public examples predictable to copy, run, capture, and compare.
2. Make `examples/c/MANIFEST.yaml` the authoritative source for public example IDs, titles,
   categories, data contracts, WebGPU routes, and gallery metadata.
3. Align C example internals around one implementation style: shared runner where appropriate,
   consistent helper names, consistent scenario IDs/titles, project allocation/logging helpers, and
   explicit data-preparation failures.
4. Align Python, Qt, integration, and benchmark examples with the v0.4 API story.
5. Keep release hygiene intact: do not touch `data`, generated binaries, or unrelated user changes.


## Non-Goals

1. Do not preserve or restore v0.3 compatibility.
2. Do not refactor scene/runtime APIs beyond what example consistency requires.
3. Do not change `examples/c/runtime/multi_window.c`.
4. Do not promote lab or legacy examples into public release scope.
5. Do not add new example features, visuals, datasets, or gallery screenshots.


## Commit Plan

### Commit 1: Normalize Example Metadata And WebGPU Routing

Files likely touched:

- `examples/c/MANIFEST.yaml`
- `examples/webgpu/live_examples.js`
- `examples/webgpu/COMPAT.md`
- `examples/webgpu/fixture_manifest.json`
- `examples/webgpu/generate_fixture_manifest.mjs`
- `docs/reference/webgpu-subset.md`
- `spec/scene/integration/WASM_WEBGPU_PARITY_PLAN.md`
- `agents/now/STATUS.md`

Tasks:

1. Add an explicit `webgpu.scenario_id` field for live routes whose public route ID differs from
   the C scenario ID.
2. Decide and encode one data-kind vocabulary. Preferred: keep `generated` as an allowed manifest
   kind because `tools/data/DATA_POLICY.md` already defines it; update the manifest schema prose.
3. Split overloaded WebGPU metadata:
   - keep route/browser-facing capability tags as `webgpu.requirements` only if existing tooling
     depends on the name;
   - otherwise introduce `webgpu.capability_tags` and reserve `scenario_requirements` for runner
     requirement bits.
4. Move lab entries out of the public `examples:` list or mark them under a clearly non-public
   registry so generated docs and audit scripts cannot treat them as release examples.
5. Ensure `start_scatter` and all files built in public folders have complete manifest metadata, or
   explicitly classify non-public files outside release routing.
6. Keep `feature_multi_window` metadata unchanged unless needed for validation, and do not edit its
   source file.
7. Reconcile WebGPU live-route counts. The current source count is 66; remove stale prose counts or
   update them from the manifest.
8. Make `COMPAT.md` clear whether each row is a public route ID, C scenario ID, or an explicit
   `route -> scenario` pair.
9. Make `fixture_manifest.json.generated_from` structured, covering positive fixtures, WebGPU stream
   fixtures, and negative parity fixtures.
10. Add or update a manifest check so route IDs, scenario IDs, and live routes cannot drift again.

Validation before commit:

```sh
python3 tools/check_example_manifests.py
node --check examples/webgpu/live_examples.js
node --check examples/webgpu/examples.js
node --check examples/webgpu/live.js
git diff --check
git status --short
git diff --cached --stat
```


### Commit 2: Standardize Public C Example Structure

Files likely touched:

- `examples/c/example_common.c`
- `examples/c/example_common.h`
- `examples/c/example_gui_controls.c`
- `examples/c/example_gui_controls.h`
- `examples/c/runner/scenario_runner.c`
- `examples/c/runner/scenario_runner.h`
- selected files under `examples/c/features/`
- selected files under `examples/c/visuals/`
- selected files under `examples/c/composites/`
- selected files under `examples/c/showcases/`
- selected files under `examples/c/runtime/`
- selected files under `examples/c/advanced/`

Do not touch:

- `examples/c/runtime/multi_window.c`

Tasks:

1. Define one naming rule for scenario factory functions:
   - exported `dvz_example_<manifest_id>_scenario()` only when a scenario is reused outside the
     file, for example WASM registration or tests;
   - static `_scenario()` or `_example_id_scenario()` for file-local examples.
2. Align `DvzScenarioSpec.id` with manifest identity or add a documented distinction between
   manifest route ID and scenario ID.
3. Set `DvzScenarioSpec.title` to human-readable manifest titles where the runner exposes the title
   to users.
4. Rename shared GUI helpers from `dvz_example_gui_*` to `example_gui_*` unless they are meant to
   read as public Datoviz API. Update all callers.
5. Consolidate duplicated argument parsing between `example_common.c` and `scenario_runner.c`.
   Keep strict parsing for `--frames`, `--size`, `--logical-size`, scales, and output flags.
6. Add a small shared direct-example CLI helper for intentional non-runner examples, or document
   exceptions and align their most visible flags:
   - `--help`
   - `--frames N`
   - `--png`
   - `--video N` or equivalent runtime-specific mode
7. Fix `input_events.c`: `--png` must either write a PNG or be removed as an alias. Preferred:
   keep `--synthetic` for event smoke and reserve `--png` for capture semantics.
8. Fix `view_size_policies.c`: parse `--frames N` explicitly and reject invalid values instead of
   silently falling back to interactive mode.
9. Fix output path semantics:
   - `example_outpath()` must not collapse multi-output examples to one basename;
   - `record_replay.c` must keep distinct `.dvzr`, original PNG, and replay PNG names under
     `DVZ_CAPTURE_BASENAME`;
   - debug screenshots must preserve their numbered suffixes.
10. Make runtime video output honor the same capture directory/basename behavior as PNG smoke paths
    in `video_export.c`.
11. Normalize prepared-data comments and runtime messages so each example prints the exact command
    needed to prepare missing data:
    - `point_cloud.c`
    - `brain_volume.c`
    - `protein.c`
    - `embedding_atlas.c`
    - `lipid_brain_atlas.c`
    - `synthetic_mouse.c`
    - `choropleth.c`
12. Replace raw `calloc/free` with project allocation helpers where ownership is Datoviz-owned and
    no external API requires raw C allocation.
13. Replace raw `printf/fprintf` with `dvz_fprintf` in public examples, except where a minimal
    external integration example intentionally uses C standard library output.
14. Remove non-ASCII punctuation from public example comments unless the file already has a clear
    reason to use it.
15. Update `start/scatter.c` to avoid copy-unfriendly global `srand/rand` and large stack arrays.
    Preferred: tiny local deterministic RNG plus heap allocation through project helpers.

Validation before commit:

```sh
python3 tools/check_example_manifests.py
just build
just test scene
git diff --check
git status --short
git diff --cached --stat
```

If full `just build` is blocked by local graphics/toolchain state, run the narrowest compile/check
loop available and record the blocker explicitly.


### Commit 3: Align Python, Qt, Integration, And Benchmark Examples

Files likely touched:

- `examples/python/direct/offscreen_point.py`
- `examples/python/raw/offscreen_point.py`
- `examples/python/raw/async_click.py`
- `examples/python/features/cupy_particles.py`
- `examples/python/qt/hosted_pyqt.py`
- `examples/qt/README.md`
- `examples/c/integration/fetchcontent/CMakeLists.txt`
- `examples/c/integration/fetchcontent/README.md`
- `examples/benchmarks/benchmark_mpl.py`
- `.gitignore` if benchmark output routing needs a dedicated ignored directory

Tasks:

1. Fix `cupy_particles.py` point data names: use `diameter_px`, not `size`, unless the actual visual
   policy explicitly documents another attribute.
2. Fix raw background-color calls in Python examples to pass the expected `DvzColor` shape.
3. Standardize one-shot offscreen Python examples on `dvz_view_render_once()` where available,
   matching the C runtime example behavior.
4. Remove ignored `__pycache__` artifacts under `examples/python/` from the working tree.
5. Update Qt README commands so bounded smoke commands appear before unbounded interactive commands.
6. Update the FetchContent example default tag or README so v0.4-dev and RC users do not copy a
   nonexistent `v0.4.0` tag before final release.
7. Quarantine or modernize `benchmark_mpl.py`:
   - remove hard-coded `BENCHMARK_ID = "debug"`;
   - avoid eager allocation for all `N_VALUES`;
   - route JSON/PNG outputs to `build/benchmarks` or `.cache/datoviz/benchmarks`;
   - clearly label any remaining old object-oriented Datoviz code as legacy, or migrate it to the
     v0.4 `dvz_*` array-aware facade if feasible within scope.

Validation before commit:

```sh
python3 -m py_compile \
  examples/python/direct/offscreen_point.py \
  examples/python/raw/offscreen_point.py \
  examples/python/raw/async_click.py \
  examples/python/features/cupy_particles.py \
  examples/python/qt/hosted_pyqt.py \
  examples/benchmarks/benchmark_mpl.py
git diff --check
git status --short
git diff --cached --stat
```


### Commit 4: Final Consistency Checks And Follow-Up Fixes

Use this commit only if validation exposes small cross-cutting fixes after the first three commits.

Tasks:

1. Re-run the static consistency probes:
   - every public source file has a manifest row or explicit non-public classification;
   - no public route has ambiguous scenario mapping;
   - no public example uses `--png` for non-capture behavior;
   - no prepared/generated/real data example silently falls back to undeclared synthetic data.
2. Run broad validation:

```sh
python3 tools/check_example_manifests.py
node --check examples/webgpu/live_examples.js
node --check examples/webgpu/examples.js
node --check examples/webgpu/live.js
just build
git diff --check
git status --short
git diff --cached --stat
```

3. If public headers, exported API, binding policy, or binding generators were touched
   unexpectedly, stop and run:

```sh
just ctypes
just ctypes-check
```


## Suggested Static Guardrails To Add

Add or extend a checker so future drift is caught early:

1. Manifest row exists for each built public example, except explicit non-public folders.
2. Manifest `data.kind` is one of the documented values.
3. WebGPU live route IDs map to real scenario IDs.
4. `webgpu-live` rows have routes and scenario IDs.
5. Public route count is generated, not hand-maintained in prose.
6. Public examples do not use `--png` for non-capture behavior.
7. Prepared/generated/real data examples include a preparation command and do not silently synthesize
   undeclared runtime fallback data.


## Commit Hygiene

Before each commit:

```sh
git diff --check
git status --short
git diff --cached --stat
```

Stage only files touched for the current checkpoint. Do not stage:

1. `NOTES` unless the user explicitly requests it.
2. `data` gitlink changes or any `data` working-tree changes.
3. `examples/c/runtime/multi_window.c`.
4. generated/runtime binaries, `.npy`, `.npz`, shared libraries, `.DS_Store`, or Python bytecode.


## Completion Criteria

The cleanup is complete when:

1. Public examples have consistent IDs, titles, metadata, CLI behavior, and output paths.
2. Manifest/WebGPU/docs agree on live route state and counts.
3. Data-backed examples fail with exact preparation commands and no undeclared fallback.
4. Python/Qt/integration examples match the v0.4 API story.
5. `python3 tools/check_example_manifests.py`, targeted JS/Python checks, `git diff --check`, and
   the broadest feasible build/test validation pass.
