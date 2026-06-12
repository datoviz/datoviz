# WebGPU Data Bundles

> **Status:** v0.4 implementation handoff
> **Scope:** prepared example data used by live WebGPU/WASM gallery routes

Live WebGPU examples that require data should load versioned static data bundles at runtime. Do not
grow the base WASM scene module by preloading every promoted gallery dataset. The browser runner is
host glue: it fetches prepared bytes, mounts them into the Emscripten filesystem, and then runs the
same portable C scenario as native validation.


## Decision

Move data-backed live examples from build-time `--preload-file` packaging to per-example fetched web
bundles.

The canonical C example keeps reading ordinary Datoviz data paths:

```text
data/examples/<example_id>/manifest.json
data/examples/<example_id>/prepared/<artifact>
data/assets/<asset_group>/<artifact>
```

The website serves those files from hash-versioned static URLs:

```text
webgpu-data/examples/<example_id>/<bundle_version>/manifest.json
webgpu-data/examples/<example_id>/<bundle_version>/prepared/<artifact>
webgpu-data/assets/<asset_group>/<bundle_version>/<artifact>
```

Browser JavaScript fetches the bundle, verifies the declared artifacts where practical, writes the
files into the Emscripten virtual filesystem at the canonical paths, and only then creates the
scenario. Scenario code must not know whether data came from the local `data` submodule, a local
cache, a WASM preloaded file, or a web fetch.


## Non-Goals

1. Do not create browser-only data formats for gallery routes.
2. Do not parse or reinterpret scientific data in JavaScript beyond fetching, integrity checks, and
   filesystem mounting.
3. Do not silently synthesize fallback data for examples declared as `prepared`, `real-data`, or
   externally sourced.
4. Do not make every data-backed showcase live. Static media remains the default gallery artifact.
5. Do not commit generated site copies or packed runtime data to the parent repository unless a
   release-artifact policy explicitly approves those exact files.


## Bundle Contract

The existing `datoviz.example-data.v1` manifest remains the source of truth. Add a `web` section
only when a bundle is eligible for browser serving:

```json
{
  "schema": "datoviz.example-data.v1",
  "id": "us_state_choropleth",
  "status": "committed",
  "web": {
    "version": "sha256-0123456789abcdef",
    "virtual_root": "data/examples/us_state_choropleth",
    "max_bytes": 8000000,
    "required": true
  },
  "artifacts": [
    {
      "role": "points",
      "path": "prepared/points_xy_f64.bin",
      "format": "raw",
      "dtype": "f64",
      "shape": [123456, 2],
      "byte_size": 1975296,
      "sha256": "..."
    }
  ]
}
```

Required `web` fields:

| Field | Meaning |
| --- | --- |
| `version` | Stable content version used in static URLs. Prefer a short aggregate SHA-256 prefix. |
| `virtual_root` | Emscripten filesystem directory where the bundle is mounted. |
| `max_bytes` | Browser promotion budget. The loader must reject bundles larger than this value. |
| `required` | Whether scenario creation must fail when the bundle cannot be loaded. |

Artifact entries should already carry `path`, `byte_size`, `sha256`, `format`, `dtype`, and `shape`
when known. The web loader should treat `path` as relative to `virtual_root`, and should reject
absolute paths, `..`, empty path segments, and duplicate output paths.


## Example Manifest Fields

`examples/c/MANIFEST.yaml` should point live routes to bundle manifests rather than relying on
implicit preloaded files:

```yaml
webgpu:
  status: webgpu-live
  route: examples/webgpu/live.html?id=us_state_choropleth
  data_bundles:
    - id: us_state_choropleth
      url: webgpu-data/examples/us_state_choropleth/sha256-0123456789abcdef/manifest.json
      virtual_root: data/examples/us_state_choropleth
      required: true
```

The generated `examples/webgpu/live_examples.js` entry should mirror only the browser host fields:

```js
{
  id: "us_state_choropleth",
  label: "U.S. State Choropleth",
  scenarioId: "us_state_choropleth",
  dataBundles: [
    {
      id: "us_state_choropleth",
      url: "../../webgpu-data/examples/us_state_choropleth/sha256-0123456789abcdef/manifest.json",
      virtualRoot: "data/examples/us_state_choropleth",
      required: true,
    },
  ],
}
```


## Loading Flow

The live route should load data before scenario creation:

```text
examples/webgpu/live.html?id=<example_id>
  -> live.js resolves LIVE_EXAMPLES entry
  -> data_loader.js fetches each dataBundles[] manifest
  -> loader validates web metadata, total size, artifact paths, and optional SHA-256 hashes
  -> loader fetches each artifact as ArrayBuffer
  -> loader writes manifest/provenance/artifacts into Module.FS
  -> WasmSceneSession creates the portable C scenario
  -> scenario reads canonical data paths
  -> scene emits DRP2 packets
  -> WebGPU runtime renders
```

The loader should be idempotent. Re-entering the same live route may skip files already mounted with
the same bundle id and version. Different versions should replace the mounted bundle directory
before scenario creation.


## WASM Runtime Requirements

The generated Emscripten module must expose filesystem access to the JS host. The implementation may
use either direct `Module.FS` exposure or a small wrapper in `web/wasm/scene.js`, but the public
browser runner should not depend on undocumented globals outside the WASM session module.

Recommended direction:

1. export `FS` and `NODEFS`/`MEMFS` support needed by the browser build;
2. add `web/wasm/data_loader.js` with `mountDataBundles(Module, bundles, options)`;
3. call `mountDataBundles()` from `WasmSceneSession.load()` before
   `DatovizWasmScene.createScenario()`;
4. remove example-data `--preload-file` entries from `src/wasm/CMakeLists.txt` once fetched bundles
   cover those live routes;
5. keep only tiny, truly shared runtime assets preloaded if they are required before any route is
   known.

This is an intentional API break for the browser host. It is better to break the experimental v0.4
WebGPU loader now than to let data-backed gallery routes depend on a growing monolithic WASM
package.


## Static Site Generation

The MkDocs build should copy only selected web-eligible bundles into the generated site:

```text
examples/c/MANIFEST.yaml
  -> collect webgpu-live entries with webgpu.data_bundles
  -> read each source data manifest
  -> validate status, web metadata, artifact hashes, total size, and provenance links
  -> copy manifest, PROVENANCE.md, BLOCKERS.md when present, and prepared artifacts
  -> write site/webgpu-data/examples/<id>/<version>/
  -> optionally write site/webgpu-data/index.json
```

Suggested source-to-site mapping:

| Source | Site output |
| --- | --- |
| `data/examples/<id>/manifest.json` | `site/webgpu-data/examples/<id>/<version>/manifest.json` |
| `data/examples/<id>/PROVENANCE.md` | `site/webgpu-data/examples/<id>/<version>/PROVENANCE.md` |
| `data/examples/<id>/prepared/*` | `site/webgpu-data/examples/<id>/<version>/prepared/*` |
| `.cache/datoviz/examples/<id>/prepared/*` | Not copied unless promotion policy explicitly allows it. |

Cache-only data may be useful for local development, but it should not be promoted to public
`webgpu-live` without redistribution and provenance review.


## Serving Policy

The site can serve bundles as ordinary static files:

| File kind | Cache policy | Notes |
| --- | --- | --- |
| Versioned binary artifacts | `Cache-Control: public, max-age=31536000, immutable` | URL contains `version`. |
| Versioned manifest/provenance | `Cache-Control: public, max-age=31536000, immutable` | Content-addressed with bundle. |
| Unversioned index files | `Cache-Control: no-cache` or short TTL | Optional convenience only. |

Use `application/octet-stream` for `.bin`, `.npy`, `.npz`, `.gz`, and other binary artifacts.
Correct MIME types are useful, but loaders must rely on manifest metadata, not MIME sniffing.


## Browser Cache

The first implementation can rely on HTTP caching only. A later optimization may add Cache API or
IndexedDB storage keyed by:

```text
datoviz-webgpu-data:<bundle_id>:<version>:<artifact_path>
```

Do not add browser persistence until there is a demonstrated repeat-download problem. If persistent
cache is added, provide a small invalidation path and keep the no-cache path available for tests.


## Failure Behavior

A missing required bundle should fail before scenario creation with a precise diagnostic:

```text
Missing prepared data bundle for us_state_choropleth.
Run: python tools/data/prepare_us_state_choropleth.py
```

Other required diagnostics:

1. bundle manifest fetch failed;
2. artifact fetch failed;
3. total bundle size exceeds `web.max_bytes`;
4. SHA-256 mismatch;
5. artifact path escapes `web.virtual_root`;
6. manifest status is not web-eligible;
7. bundle requires data that is blocked by `BLOCKERS.md`.

The gallery page should still show static PNG or video fallback media outside the iframe. The live
iframe may show the failure status, but it must not pretend that the real example rendered.


## Validation

Add focused validation rather than exhaustive browser coverage:

1. `python3 tools/data/validate_manifests.py data/examples/<id>`;
2. a new static-site bundle checker for `webgpu.data_bundles`;
3. `node --check examples/webgpu/live_examples.js`;
4. `just wasm-scene-smoke`;
5. `just webgpu-browser-smoke` for at least one data-backed live route;
6. `git diff --check`.

The browser smoke should assert that a data-backed route reaches rendered status with a nonblank
canvas when WebGPU is available, and reports the expected missing-bundle diagnostic when served
without the copied bundle.


## Migration Plan

1. Add `webgpu.data_bundles` metadata for `us_state_choropleth` and `protein_arcball_viewer`.
2. Add `web/wasm/data_loader.js` and route it through `WasmSceneSession`.
3. Expose the Emscripten filesystem through the WASM session module.
4. Teach `tools/build_gallery.py` or `tools/mkdocs_hooks.py` to copy declared web bundles.
5. Add a bundle validation script and hook it into the narrow gallery validation loop.
6. Remove dataset-specific `--preload-file` rules from `src/wasm/CMakeLists.txt`.
7. Keep texture/font/runtime assets only if they are shared enough to justify preload.
8. Promote additional examples only when their bundle is compact, redistributable, and documented.

Stop after each migrated example and record browser evidence in `examples/webgpu/COMPAT.md`.


## Promotion Criteria

An example may be `webgpu-live` with data only when:

1. the scenario is the same portable C scenario used by native validation;
2. prepared artifacts are redistributable or generated with clear provenance;
3. the browser bundle is compact enough for gallery use;
4. source, license, citation, and preprocessing are linked from the example page;
5. missing data fails with a preparation command;
6. static media exists as the default gallery fallback.

If any criterion is missing, mark the example `webgpu-planned`, `webgpu-deferred`, or `native-only`
instead of adding synthetic browser fallback data.
