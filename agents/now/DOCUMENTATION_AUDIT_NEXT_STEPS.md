# Documentation Audit Next Steps

Status: active follow-up plan after the v0.4 documentation audit and API surface audit closeout on
2026-06-18.

Use this as the execution order for the next documentation cleanup pass. Keep this file focused on
active work; move durable documentation policy into `spec/docs/` when needed.


## Recently Completed By API Surface Work

1. Public installed-header API contract wording was tightened for ownership, lifetimes, pointer
   counts, status returns, advanced/unstable runtime APIs, and scene inspection escape hatches.
2. Generated C reference/raw `ctypes` policy still needs a local `just ctypes-check` rerun in an
   environment with the Python `clang` module; the local attempt stopped before repo checks started.
3. `agents/now/API_SURFACE_AUDIT.md` is now a completed closeout record, not an active work queue.


## Preferred Sequence

1. Rewrite `README.md` for v0.4.
   Treat it as a fresh v0.4 front door, not a patch over the v0.3-era README. Keep it short and
   release-candidate honest: C-first engine, source build path while v0.4 packages are pending,
   raw/facade Python scope, WebGPU/Qt experimental or optional-provider status, and links into the
   public docs.

2. Fix copy-breaking user docs.
   - `docs/start/install.md`: update to CMake 3.21+, Python >=3.10, and include the shader tool
     dependency path for `glslangValidator`.
   - `docs/index.md`: include `datoviz/app.h` where app APIs are used and remove the nonexistent
     C `dvz_capture()` reference.
   - `docs/how-to/profile-performance.md`: replace `dvz_point(panel, 0)` with
     `dvz_point(scene, 0)`.

3. Reconcile Python docs.
   Make every public page use the same model:
   - `import datoviz as dvz` is the top-level array-aware direct-engine facade.
   - `import datoviz.raw as raw` is the exact generated `ctypes` layer.
   - high-level plotting belongs to external GSP/VisPy2.
   Also clarify that only the policy-declared facade calls are currently array-adapted.

4. Fix scene emission source-of-truth drift.
   Update older spec pages so the active path is consistently:
   `scene frame plans -> DvzSceneFrameArtifact -> DRP2 packet/stream snapshots -> runtime`.
   Prioritize ownership and lifetime wording in `spec/scene/core/RUNTIME_BOUNDARY.md`,
   `spec/scene/pipeline/FRAME_LIFECYCLE.md`, and
   `spec/scene/api/API_IMPLEMENTATION_READINESS.md`.

5. Clean generated example metadata.
   Fix the Textured Mesh/Textured Planets WebGPU route and status conflict at the manifest/source
   metadata level, then regenerate gallery pages, the WebGPU matrix, and public examples metadata.

6. Add real-data attribution.
   For Textured Planets and any similar real-data examples, add source, license/citation,
   preparation command, and provenance notes. Treat this as a release-gate item.

7. Tighten experimental-provider docs.
   - WebGPU: distinguish a published `webgpu-live` route from actual visual render proof on a
     specific browser/adapter.
   - Qt/PyQt: state clearly that `datoviz_qtbridge` is an optional provider that must be built or
     supplied separately; it is not part of the base wheel.

8. Regenerate and validate.
   Run the narrow generators/checks that match the edited scope, then finish with the full
   documentation validation loop below.


## Suggested Checkpoint Commits

1. README plus copy-breaking public docs.
2. Python wording, scene-emission spec consistency, generated gallery/WebGPU metadata, and
   attribution fixes.

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
