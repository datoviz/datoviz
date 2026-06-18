# Documentation Audit Next Steps

Status: active follow-up plan after the v0.4 documentation cleanup pass on 2026-06-18.

Use this as the execution order for the next documentation cleanup pass. Keep this file focused on
active work; move durable documentation policy into `spec/docs/` when needed.


## Recently Completed By API Surface Work

1. Public installed-header API contract wording was tightened for ownership, lifetimes, pointer
   counts, status returns, advanced/unstable runtime APIs, and scene inspection escape hatches.
2. Generated C reference/raw `ctypes` policy still needs a local `just ctypes-check` rerun in an
   environment with the Python `clang` module; the local attempt stopped before repo checks started.
3. The completed API surface audit file was removed from `agents/now/`; commit history is the
   closeout record.


## Completed In Current Cleanup Pass

1. `README.md` was rewritten as a short v0.4 front door.
2. Copy-breaking public docs were fixed in install, front-page, and performance pages.
3. Public Python layer wording was reconciled where drift remained.
4. Scene-emission spec wording now uses:
   `scene frame plans -> DvzSceneFrameArtifact -> DRP2 packet/stream snapshots -> runtime`.
5. Textured Mesh/Textured Planets WebGPU route/status metadata was fixed at the manifest level and
   generated public metadata was refreshed.
6. Textured Planets now records source, license/citation, preparation, and provenance notes.
7. WebGPU and Qt/PyQt provider docs now distinguish route publication from adapter visual proof and
   base-wheel support from optional providers.
8. `uv run --with libclang just ctypes-check` passed locally.
9. Real/prepared showcase attribution was expanded for U.S. State Choropleth, Protein, Lipid Brain
   Atlas, and Allen Mouse Brain.


## Remaining Next Steps

1. During final release proof, pair `webgpu-live` status with recorded browser/adapter evidence for
   promoted routes that matter to RC notes. Local `just webgpu-browser-smoke` on 2026-06-18 rebuilt
   the WASM scene target but failed after a headless WebGPU instance-loss skip with:
   `CreateShaderModule: invalid character found`.

Do not include unrelated user changes, generated binary payloads, or `data` submodule changes unless
explicitly approved in the current turn.


## Validation

```sh
just gallery
just check-api-docs
python3 tools/check_example_manifests.py
uv run --with-requirements requirements-dev.txt mkdocs build --strict
git diff --check
git status --short
```
