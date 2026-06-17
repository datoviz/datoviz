# Adding A WebGPU Fixture

WebGPU fixtures validate the experimental browser/backend path against the same scene and DRP2
contracts used by the native runtime.


## Start From A Shared Scenario

A public WebGPU route should reuse a canonical C example or portable C scenario. Browser JavaScript
is host glue; it must not reimplement scene behavior, visual state, animation, picking, selection,
query/probe, or data semantics.

Before adding a fixture, check:

1. `spec/scene/integration/WASM_WEBGPU_PARITY_PLAN.md`;
2. `examples/c/MANIFEST.yaml`;
3. `examples/webgpu/live_examples.js`;
4. `examples/webgpu/COMPAT.md`;
5. existing assertions in `tools/wasm_scene_smoke.mjs` and `tools/webgpu_browser_smoke.mjs`.


## Fixture Route

For a new promoted route:

1. export or reuse the canonical C scenario;
2. register it in `src/wasm/scene_api.c` and `src/wasm/CMakeLists.txt` when needed;
3. add or verify the live route in `examples/webgpu/live_examples.js`;
4. update `examples/c/MANIFEST.yaml` with the WebGPU status;
5. add targeted stream-shape assertions in `tools/wasm_scene_smoke.mjs`;
6. add a browser smoke row only when the route protects a release blocker or new capability
   cluster;
7. record local compatibility evidence in `examples/webgpu/COMPAT.md`.

Keep automated browser smoke representative, not exhaustive.


## Validation

Use the relevant subset:

```sh
node --check examples/webgpu/live_examples.js
node --check tools/wasm_scene_smoke.mjs
node --check tools/webgpu_browser_smoke.mjs
python3 tools/check_example_manifests.py
just wasm-scene-smoke
just webgpu-browser-smoke
git diff --check
```

Headless browser runs may skip live routes because of external WebGPU instance-loss diagnostics.
Record that honestly; do not promote skipped coverage as rendered proof.
