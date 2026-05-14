# Scene WGSL Shader Variant Plan

> **Execution Status**
> - **Status:** `PICKUP PLAN`
> - **Updated on:** `2026-05-14`
> - **Purpose:** document how scene-owned GLSL/WGSL shader variants should feed DRP2 WebGPU
>   emission without moving shader semantics into the browser replay prototype.


## Goal

The scene layer should own built-in visual shader semantics and should emit DRP2 streams in the
shader format requested by the target backend.

For the native Vulkan path, the scene may continue to use GLSL or precompiled SPIR-V. For WebGPU,
the scene should emit WGSL shader modules. The browser WebGPU runner should execute the stream it is
given; it should not translate GLSL, substitute shaders by hash, or understand scene visual
semantics.


## Current Context

The first browser WebGPU feasibility lane is now useful as a contract test:

1. `examples/webgpu/index.html` runs small hand-authored DRP2 streams in WebGPU.
2. `examples/webgpu/fixtures.html` runs the positive DRP2 fixture dashboard.
3. `examples/webgpu/COMPAT.md` records the current PoC compatibility decisions and the latest
   all-pass dashboard status.

The scene side already has a shader registry boundary:

1. `src/scene/shader_registry.c` owns current fallback GLSL strings and fixture WGSL helpers.
2. `src/scene/_shader_registry.h` exposes internal shader registry helpers.
3. `src/scene/visual_pipeline.c` resolves `DvzSceneVisualShaderDesc` for built-in visuals and
   currently fills GLSL fields for the retained visual path.
4. `agents/now/SCENE_CONVERTER_REFACTOR_PLAN.md` already recommends file-backed runtime visual
   shaders under `src/scene/glsl/` and extending the existing root CMake shader pipeline.


## Ownership Decision

Use this boundary:

1. The scene shader registry owns built-in shader variants.
2. The scene-to-DRP2 emitter selects one backend-ready shader format per emitted stream.
3. DRP2 transports the selected shader module source or bytecode.
4. The WebGPU backend accepts WGSL and rejects unsupported shader formats.

Do not make the WebGPU runner responsible for GLSL-to-WGSL translation or scene shader
substitution. That would duplicate scene knowledge in the browser prototype and make later native
WebGPU work harder to align with the scene runtime.


## WGSL Storage Recommendation

Prefer parallel source directories with matching semantic shader keys:

```text
src/scene/glsl/
  point.vert
  point.frag
  primitive.vert
  primitive.frag
  image.vert
  image.frag

src/scene/wgsl/
  point.vert.wgsl
  point.frag.wgsl
  primitive.vert.wgsl
  primitive.frag.wgsl
  image.vert.wgsl
  image.frag.wgsl
```

Rationale:

1. `src/scene/glsl/` is already the documented target for file-backed built-in scene shaders.
2. A sibling `src/scene/wgsl/` keeps generated browser-ready variants discoverable and reviewable.
3. Matching basenames keep registry entries deterministic and make later generation checks simple.
4. This avoids burying WGSL in `examples/webgpu/`, which should remain a backend/fixture harness.

If the shader set grows large, a later cleanup may move both languages under
`src/scene/shaders/{glsl,wgsl}/`. Do not start there unless the CMake shader pipeline is being
changed at the same time; the current least-disruptive step is the sibling `glsl/` and `wgsl/`
layout.


## Generated Artifact Shape

The generated C resource path should eventually expose both language variants through the registry.
Two acceptable shapes:

1. Extend the existing generated shader resource table so one generated C file contains GLSL,
   WGSL, and SPIR-V entries keyed by semantic shader name, stage, and format.
2. Add a separate generated `src/scene/generated/_shaders_wgsl.c` if that keeps the current SPIR-V
   embedding path simpler.

The registry API should hide this choice from emitters. Visual code should ask for a logical shader
variant and target format, not for a generated file or C symbol.


## Translation Pipeline

The long-term pipeline should run on the desktop side, not in the browser:

```text
canonical GLSL source
  -> glslangValidator or shaderc
  -> SPIR-V
  -> naga or Tint
  -> WGSL source committed under src/scene/wgsl/
```

Generated WGSL should be deterministic and committed at first. That keeps the WebGPU PoC easy to
open from a static page, makes shader diffs reviewable, and lets CI or a local script later verify
that generated WGSL is up to date.

Do not block the first implementation slice on a complete automated translator. Start with one
manual WGSL variant and add the generator after the scene registry contract is proven.


## Maintenance Tool Contract

Add a desktop maintenance tool after the first manual WGSL slice proves the registry path. Its
default behavior should be conservative:

1. scan the canonical GLSL shader directory and the WGSL sibling directory,
2. generate WGSL only for missing `src/scene/wgsl/*` files,
3. never overwrite an existing WGSL file by default,
4. return a clear report listing generated files, skipped existing files, and failed conversions,
5. support a `--check` mode that fails when a GLSL shader has no WGSL sibling,
6. support an explicit overwrite mode only when requested, for example `--force` or
   `--update-existing`.

This preserves the ability to hand-edit WGSL variants for WebGPU quality, browser portability, or
translation cleanup. Running the tool a second time should not erase those edits.

If later we want stronger drift detection, add a sidecar manifest rather than overwriting files:

```text
src/scene/wgsl/.generated.json
```

That manifest can record the GLSL source hash, generator version, and output hash for each generated
file. A manually edited WGSL file can then be reported as "modified since generation" without being
replaced. Keep the registry dependent only on the WGSL files themselves, not on the manifest.


## First Implementation Slice

Start with one simple retained visual and prove the format selection path end to end.

Recommended visual: `primitive` triangle-list. It is lower risk than point sprites and should map
cleanly to ordinary vertex attributes and a fragment color output.

Steps:

1. Inventory the active built-in shader keys in `src/scene/shader_registry.c` and
   `src/scene/visual_pipeline.c`.
2. Move or mirror the chosen GLSL shader into the file-backed `src/scene/glsl/` layout if that has
   not happened yet.
3. Add a manually written WGSL sibling under `src/scene/wgsl/` using the same semantic basename and
   stage suffix.
4. Extend the registry with target-format lookup, for example a helper shaped like
   `_builtin_shader_source(shader, stage, format)` or stage-specific GLSL/WGSL helpers.
5. Extend `DvzSceneVisualShaderDesc` so built-in visual descriptors can carry WGSL sources and
   format-aware pipeline cache keys.
6. Update DRP2 scene emission so `DVZ_SCENE_SHADER_FORMAT_WGSL` emits
   `CreateShaderModule.format = "wgsl"` and never embeds GLSL in that stream.
7. Add a focused scene test that emits a primitive WGSL stream and asserts that the JSON contains
   `"format": "wgsl"` and does not contain `"format": "glsl"`.
8. Save one scene-generated WGSL stream fixture or example and run it through
   `examples/webgpu/index.html` or the dashboard harness.


## Acceptance Criteria

The first slice is done when:

1. native GLSL/SPIR-V scene tests still pass,
2. one retained scene visual can emit a WGSL DRP2 stream,
3. the emitted stream contains backend-ready WGSL with no browser-side shader substitution,
4. the browser WebGPU page renders the scene-generated stream,
5. `examples/webgpu/fixtures.html` still reports no WebGPU errors or warnings for the existing
   fixture dashboard.


## Follow-Up Work

After the first visual works:

1. Add WGSL variants for point, image, mesh/depth, picking, and probe/readback support in that
   order.
2. Add the deterministic GLSL-to-WGSL generation script and an up-to-date check.
3. Decide whether DRP2 should ever carry multiple shader variants in one stream. The current
   recommendation is no: emit a target-specific stream with one selected shader format until a real
   multi-backend stream use case appears.
4. Promote the storage and translation rules into `spec/scene/` once more than one built-in visual
   uses them.
