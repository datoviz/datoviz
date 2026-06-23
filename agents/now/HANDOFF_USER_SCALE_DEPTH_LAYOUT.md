# Handoff: user_scale depth-layout validation

Date: 2026-06-23
Branch: `v0.4-dev`

## Repository Rules

- Follow root `AGENTS.md`.
- Do not stage or commit `data` submodule changes unless explicitly approved in the current turn.
- Do not stage generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`, `*.so`,
  `*.dll`, `*.npy`, `*.npz`, or `.DS_Store`.
- Always run `git diff --check` before finalizing code changes.
- Before committing, run `git status --short` and `git diff --cached --stat`; verify the staged set
  excludes `data`, generated files, vendored runtime libraries, large binaries, and unrelated user
  changes.

## User Report

The user reported this Vulkan validation error while interacting with
`./build/examples/c/features/user_scale`:

```text
validation layer: vkQueueSubmit2(): pSubmits[0] command buffer ... expects VkImage ...
to be in layout VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL--instead, current layout is
VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL.
VUID-vkCmdDraw-None-09600
```

The report occurred when zooming in the live example. The validation message names a depth image
subresource and says a sampled/image descriptor expected `READ_ONLY_OPTIMAL`, while the tracked image
layout was `ATTACHMENT_OPTIMAL`.

## Current Findings

The problem does not appear to be caused by the `user_scale` math itself.

The static/offscreen `user_scale` stream is clean:

- pass `10001` writes the named depth texture as a depth attachment;
- pass `10002` attaches the same depth texture read-only;
- the sampled bind groups in that stream sample glyph/font atlases, not the depth texture.

The important DRP2 behavior is in `src/drp2/pass.c`:

- `_binding_texture_access()` maps sampled depth textures to
  `DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ`;
- `_transition_bind_group_textures()` transitions sampled depth descriptors to read-only layouts;
- `_vklite_begin_render_pass()` skips active attachment texture ids when transitioning sampled
  bind-group textures;
- if the named depth attachment access is not read-only, the named depth texture is then transitioned
  to `DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT`, which maps to `VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL`.

That means this stream shape will reproduce the validation error:

1. a render pass attaches depth texture `D` with `WRITE` or `READ_WRITE` access;
2. a bind group set in the same pass samples texture `D`;
3. the descriptor was written with `VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL`;
4. the pass transitions `D` to `VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL` before draw.

This is invalid Vulkan. The correct scene/DRP2 contract is: sampling the active depth attachment is
only valid when that depth attachment is declared read-only for the pass.

## Reproduction Attempts

These did not reproduce the validation error locally:

```sh
./build/examples/c/features/user_scale --dvzr 1 --size 800x600
./build/examples/c/features/user_scale --dvzr 1 --size 800x600 --user-scale 2.5
./build/examples/c/features/user_scale --png --user-scale 2.5
```

A live X11 run was also attempted:

```sh
./build/examples/c/features/user_scale --live
```

The GLFW window was found and synthetic X11 wheel-up events were sent to its center. The app rendered
additional frames, but no validation output appeared in that run.

Generated artifacts from these checks were removed before this handoff was committed.

## Relevant Code Paths

- `examples/c/features/user_scale.c`
  - Path visual, blended marker visual, axes, panzoom controller, GUI slider.
  - The marker visual uses `DVZ_ALPHA_BLENDED`, causing a second pass.
  - No explicit volume, scene occlusion, depth peeling, SSAO, or sampled-depth feature is enabled.

- `src/app/app.c`
  - `_app_draw()` emits live native-view frames with `external_color_target = true`.
  - It sets `cfg.runtime_resource_scope_id = _app_frame_runtime_scope(frame)`, so graph resources
    can be scoped per swapchain command buffer.
  - `dvz_view_set_user_scale()` marks the view dirty and forces frame re-emission.

- `src/drp2/pipeline.c`
  - `_vklite_build_bind_group_descriptors()` writes sampled depth descriptors with
    `VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL`.

- `src/drp2/transfer.c`
  - `_vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ)` returns
    `VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL`.
  - `_vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT)` and
    `_WRITE` return `VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL`.

- `src/drp2/pass.c`
  - The pass-local interaction between sampled bind-group transitions and active attachment
    transitions is the most likely enforcement point.

## Tests Already Present

`src/drp2/tests/vklite_runtime.c` already contains positive coverage for the valid case:

- `test_drp2_runtime_vklite_samples_read_only_active_depth`
  - writes depth in one pass;
  - samples the same depth while attaching it read-only in a later pass;
  - asserts the depth image finishes as `VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL`.

There is also coverage that unused bind groups do not affect unrelated render-pass transitions:

- `test_drp2_runtime_vklite_ignores_unused_render_pass_bind_groups`

The missing focused coverage is the invalid case: same depth texture sampled by a bind group and
attached with non-read access in the same pass.

## Suggested Next Steps

1. Add a DRP2 runtime validation guard in `src/drp2/pass.c`.
   - While scanning bind groups set between `BeginRenderPass` and `EndRenderPass`, detect whether any
     sampled texture id equals the active named depth attachment id.
   - If so, require the effective depth attachment access to be `DVZ_DRP2_ATTACHMENT_ACCESS_READ`.
   - Return a DRP2 validation failure before Vulkan submission if the stream tries to sample and write
     the same depth attachment.

2. Add a regression test in `src/drp2/tests/vklite_runtime.c`.
   - Build a stream that attaches depth `D` with default/write access and also samples `D` in the same
     pass.
   - Assert `dvz_drp2_runtime_execute()` returns a validation failure and does not rely on Vulkan
     validation layers to catch it.

3. If the original live-only warning persists after the guard, instrument the same detection path.
   - Log pass id, named depth id, effective depth access, and bind group id/binding that samples the
     active depth.
   - That will identify the scene graph pass that emits the invalid stream.

4. If the invalid stream comes from scene emission, fix the emitter rather than papering over DRP2.
   - Any pass that samples its active depth attachment must set graph depth access to read-only.
   - If the pass truly needs to write depth, it must sample a different depth texture from an earlier
     pass.

5. Re-run a narrow validation loop:

```sh
just test drp2_runtime_vklite_samples_read_only_active_depth
./build/examples/c/features/user_scale --png --user-scale 2.5
git diff --check
```

If code changes touch shared DRP2 pass behavior, also run a broader DRP2 test filter before commit.
