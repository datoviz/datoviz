# Add A Public Gallery Example

Every public example is executable documentation and release evidence. A contribution is complete only when its canonical C scenario, manifest metadata, deterministic screenshot, applicable motion media, honest WebGPU classification, generated documentation, and focused validation agree.

## Definition Of Done

Every public example must:

1. use one canonical C scenario under `examples/c/<lane>/` and register it in `examples/c/CMakeLists.txt`;
2. have one `examples/c/MANIFEST.yaml` entry with its category, primary capability, data kind, validation, portability, WebGPU status, and `agent_copy_safe` decision;
3. declare `screenshot` validation and produce a deterministic, nonblank canonical PNG;
4. declare an explicit WebGPU status, with `webgpu-live` reserved for scenarios that pass WASM packet validation and browser execution;
5. provide deterministic animation or video when motion, interaction, streaming, compute, or a 3D camera is essential to understanding the example;
6. regenerate the committed gallery pages and machine-readable inventories;
7. pass the focused native, media, WebGPU, documentation, specification, and repository-hygiene checks.

A live browser route never replaces the screenshot. The gallery must remain useful without JavaScript, WebGPU, or video playback.

## Start With The Manifest

Use a nearby entry with the same lane and media needs. A portable interactive example with video has this shape:

```yaml
- id: features_example
  title: Example
  stage: v0.4_required
  category: feature
  primary_feature: example
  lane: features
  source: examples/c/features/example.c
  validation: smoke+interaction+screenshot+video
  portability: portable-scenario
  media:
    preview:
      kind: animated-webp
      frames: 90
      fps: 30
      card:
        preferred: video-mp4
        sample_step: 1
        fps: 30
  data:
    kind: synthetic
  webgpu:
    status: webgpu-live
    route: examples/webgpu/live.html?id=features_example
    requirements:
      - path
      - arcball
  agent_copy_safe: true
```

Use `webgpu-planned`, `webgpu-deferred`, or `native-only` when the browser path is not implemented. Include a concrete reason for deferred or native-only classifications. Do not create a separate JavaScript visualization to claim parity; the browser route must use the canonical C scenario through scene, DRP2, and the WASM host.

Real or prepared data also requires the complete dataset attribution, license, citation, preprocessing, and provenance fields defined at the top of `examples/c/MANIFEST.yaml`.

## Implement The Canonical Scenario

Keep the demonstrated public `dvz_*` calls visible in the example. Reuse shared helpers only for theme, deterministic data generation, capture, and boilerplate. The scenario must work through the native scenario runner and must not call Vulkan, GLFW, WebGPU, or browser APIs directly.

Register the native executable in `examples/c/CMakeLists.txt`. A portable `webgpu-live` scenario must also be added to `src/wasm/CMakeLists.txt`, `src/wasm/scene_api_scenario.c`, the scenario-count constant, `tools/wasm_scene_smoke.mjs`, `examples/webgpu/live_examples.js`, and the filtered browser-smoke route table.

Add focused WASM assertions for the example's meaningful packet shape and interaction updates. Compilation alone is not WebGPU proof.

## Validate Without Changing Canonical Media

Run the safe contributor and coding-agent check while iterating:

```sh
just example-check <id>
```

This builds the project, captures a temporary screenshot under `build/example-check/<id>/`, generates a temporary WebP and documentation tree, exercises configured animation/video, runs the live WebGPU packet and filtered browser routes when applicable, runs the specification gates, and finishes with `git diff --check`. It does not write `data/` or stage files.

An environment-related browser skip is not visual proof. Record the exact skip and obtain a successful browser run on a supported environment before treating the route as visually confirmed.

## Promote Canonical Media

Canonical screenshots live in the `data` submodule. Coding agents must obtain explicit approval for the exact data update before promotion. The promotion command requires an acknowledgement and never stages or commits:

```sh
just example-promote <id> --approve-data-update
```

Promotion performs the canonical capture, a second isolated byte or tightly pixel-equivalent verification capture, media validation, static WebP conversion, configured animation/video generation, WebGPU proof, documentation regeneration, local asset staging, specification checks, and `git diff --check`.

Inspect the screenshot at full resolution and thumbnail scale. Check labels, axes, clipping, camera pose, data range, background, and empty space. For deterministic motion, drive state from the scenario preview frame index and count instead of wall-clock time or manual input.

After promotion, inspect both repositories:

```sh
git -C data status --short
git status --short
```

Commit the approved media inside `data` first, then commit the parent repository's updated gitlink with the example implementation. Never stage unrelated data files or generated runtime binaries.

## Manual Commands

Use these when diagnosing one stage:

```sh
python3 tools/capture_gallery.py --id <id> --force --jobs 1
python3 tools/capture_gallery.py --id <id> --cache --verify-existing --jobs 1
python3 tools/check_gallery_media.py --id <id>
python3 tools/build_gallery_webp.py --id <id> --strict --force
just wasm-scene-smoke
just webgpu-browser-smoke --route=<id>
python3 tools/build_gallery_animations.py --id <id> --force --jobs 1 --capture-jobs 1
python3 tools/compare_gallery_media.py --id <id> --site-video-previews --write-site-assets --force --jobs 1 --capture-jobs 1
just docs-generate
just docs-assets
python3 tools/check_generated_docs.py
python3 tools/check_example_manifests.py
just spec-check
git diff --check
```

## Review Checklist

- The example teaches one clear capability or one coherent scientific workflow.
- The native scenario owns no borrowed graphics handles and follows the scene-to-DRP2 runtime boundary.
- Synthetic data and preview motion are deterministic.
- Every public example has a screenshot.
- Motion-dependent examples have an animated preview or MP4 card.
- WebGPU metadata states proven support or a concrete limitation honestly.
- `agent_copy_safe: true` is used only when generated user code can safely adapt the example.
- Generated pages and inventories are current.
- Approved `data` changes and the parent gitlink are isolated from unrelated work.
