# Test Suite Refactor Plan

Status: post-RC cleanup plan. Do not block v0.4.0 unless the current test layout causes release
validation failures, flakiness, or excessive runtime.

This plan keeps the current coverage bias intact while reducing navigation, build-wiring, and
validation-routing friction. The problem is not primarily too many tests. The problem is that suite
ownership, lanes, and large files are uneven.


## Goals

1. make test ownership obvious from paths, groups, and runner lanes;
2. keep fast local validation useful during scene, DRP2, and runtime work;
3. keep release-proof graphics coverage without duplicating every smoke case;
4. reduce CMake and runner registration duplication;
5. preserve explicit resource, isolation, skip, fixture, JSON, and sharding metadata;
6. make future deletion decisions evidence-based rather than count-based.


## Non-Goals

1. Do not replace the in-tree C/C++ runner before v0.4.0.
2. Do not collapse scene, DRP2, vklite, app, and WebGPU validation into one layer.
3. Do not delete broad release-surface coverage until a lower-cost layer proves the same contract.
4. Do not make WebGPU-only, scene-only, or backend-only semantics.
5. Do not move protocol fixtures out of `spec/drp2/fixtures/`.


## Current Shape

The active runner already has useful infrastructure: resources, isolation, skips, fixtures, JSON,
process sharding, exact filters, group filters, and module runners. Keep that.

Pain points:

1. scene tests dominate the suite and several files mix unrelated contracts;
2. DRP2 validation mixes stream construction, recording, runtime semantics, and vklite execution in
   one large file;
3. CMake manually repeats target construction, include paths, link sets, compile definitions, and
   auxiliary runner setup;
4. shared GPU fixture coverage intentionally duplicates isolated app/offscreen smokes, but this
   should become a bounded proof set;
5. Python tests mix tool unit tests, architecture lint, generated binding smoke, and fixture
   conformance wrappers without first-class lane names.


## Target Lanes

Use runner tags, groups, `just` recipes, or CTest labels to expose these lanes explicitly:

1. `fast-cpu`: CPU-only module and scene semantic tests.
2. `scene-semantic`: retained scene object, field, frame-plan, query, controller, and transform
   behavior without GPU execution.
3. `drp2-contract`: stream, schema, semantic validation, recording, replay, and fixture corpus.
4. `runtime-vklite`: DRP2 streams executed through vklite with readback or nonblank assertions.
5. `render-smoke`: small deterministic offscreen scene/app renders.
6. `render-conformance`: image or pixel-region assertions with named expectations and tolerances.
7. `webgpu-portability`: WebGPU fixture preflight, runner smoke, and WASM-emitted fixture checks.
8. `bindings-smoke`: generated raw `ctypes`, ABI facts, and low-level Python helper smoke.
9. `architecture-lint`: source guards and policy checks.
10. `release-proof`: the compact validation set required for RC notes.
11. `slow-churn`: repeated-frame, resize, recreate, long-run resource, and failure-injection loops.


## File Split Plan

Split large files only when doing nearby work or as a focused cleanup pass. Preserve test names when
possible so existing filters continue to work.

1. Split `src/scene/tests/scene_visuals.c` by visual-family or closely related family groups:
   point/pixel/marker, primitive/segment/path/vector, image/volume/labels, mesh/sphere/splat, and
   shared visual helpers.
2. Split `src/scene/tests/app.c` into offscreen rendering, shared fixture proof, capture/readback,
   transparency/occlusion, text/annotation, volume rendering, query steady-state, and app scheduler
   files.
3. Split `src/drp2/tests/test_drp2.c` into stream API, JSON/recording, render-pass state, runtime
   validation, vklite runtime execution, compute/sync, and shared builders.
4. Split `src/scene/tests/fields.c` into field storage, typed upload, partial updates, texture
   lowering, runtime upload, and error-path tests.
5. Split `src/scene/tests/scene_techniques.c` into transparency, lighting, occlusion/postprocess,
   and shader/runtime execution contracts.
6. Keep focused helper files small and internal to their test subsystem. Avoid one catch-all
   `helpers.c` absorbing unrelated builders.


## Runner And Build Refactor

1. Add a small CMake helper that declares a Datoviz test runner from:
   target name, source set, enabled target predicates, link modules, extra includes, compile
   definitions, shader dependencies, and CTest labels.
2. Replace repeated target setup in `testing/CMakeLists.txt` incrementally. Start with module
   runners that already share the same shape.
3. Keep the aggregate `dvztest` runner and focused module runners. The aggregate catches accidental
   cross-module interactions; module runners keep local loops cheap.
4. Clarify whether `dvztest_integration` proves a distinct contract. If it duplicates `dvztest`
   minus app/GUI coverage, rename it to the specific lane it owns or remove it after CI and release
   recipes stop depending on it.
5. Attach CTest labels or runner metadata for the target lanes above.
6. Add `just` recipes for the lanes developers actually need, for example:
   `just test-fast`, `just test-scene-cpu`, `just test-render-smoke`, `just test-release-proof`,
   and `just test-slow`.


## Shared Fixture Policy

The shared GPU fixture lane should prove fixture reuse and catch fixture-specific bugs, not double
the whole offscreen suite.

Keep shared-fixture versions for:

1. clear color;
2. simple nonblank point, pixel, image, mesh, and volume smokes;
3. one multi-panel smoke;
4. one depth or lighting smoke;
5. one retained second-frame smoke.

Keep isolated versions for:

1. app creation and destruction;
2. resize and recreate;
3. descriptor refresh and resource lifetime;
4. query/readback steady-state;
5. failure injection and recovery;
6. GLFW, presentation, external surface, video, filesystem, environment, and log-capture tests.


## Deletion Criteria

A test can be deleted or merged only when all of these are true:

1. the same public or internal contract is covered by a cheaper test layer;
2. the remaining test fails for the same bug class;
3. the deleted test is not the only release-proof example for a declared capability;
4. the change records the replacement in the commit message or nearby review notes;
5. the relevant lane passes before and after deletion.

Prefer converting duplicated expensive render tests into CPU semantic, DRP2 contract, or small
pixel-region tests before deleting them outright.


## Rollout

### Phase 1: Inventory

1. Generate a runner JSON inventory with case id, module, group, resources, isolation, fixture, and
   elapsed time.
2. Mark every case with one target lane.
3. Identify duplicate isolated/shared fixture pairs.
4. Identify the slowest groups and files.
5. Record release-proof cases separately from broad regression cases.

Initial tooling: [LANE_INVENTORY.md](LANE_INVENTORY.md) defines the metadata-only inventory command
case-list lane routing, initial `just test-*` lane recipes, and optional timing merge. Keep generated
inventory files under `build/testing/` while lanes are being reviewed.

### Phase 2: Low-Risk Structure

1. Add lane labels or tags without changing behavior.
2. Add `just` lane recipes.
3. Refactor repeated CMake runner setup for one or two simple runners.
4. Keep `just test` behavior unchanged.

### Phase 3: File Splits

1. Split the largest scene and DRP2 files along existing group boundaries.
2. Preserve helper ownership with the files that use the helpers.
3. Run the narrow module runner after each split.
4. Avoid moving test logic and changing assertions in the same commit unless necessary.

### Phase 4: Duplicate Reduction

1. Trim shared-fixture duplicates to the policy above.
2. Move broad visual behavior proof to semantic, DRP2, or small render-conformance cases where
   possible.
3. Keep release-proof graphics smokes explicit and easy to run.

### Phase 5: CI And Release Routing

1. Route pull-request CI through fast CPU, selected GPU smoke, fixture conformance, architecture
   lint, and binding smoke lanes.
2. Route nightly or manual CI through slow churn, broader render conformance, sanitizer lanes, and
   release-proof examples.
3. Make RC notes cite named lane commands instead of ad-hoc command lists.


## Acceptance Checks

Each refactor commit should run:

```sh
git diff --check
just build
just test <affected-filter-or-module>
```

Runner or scheduling changes should additionally run:

```sh
./build/testing/dvztest --list
./build/testing/dvztest --list-groups
./build/testing/dvztest --json /tmp/dvztest.json <small-filter>
.venv/bin/pytest -q testing/test_dvztest_scheduler.py
```

Release-lane routing changes should run:

```sh
just spec-check
just test scene
```
