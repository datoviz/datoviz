# Handoff: user_scale depth-layout validation warning

Date: 2026-06-23
Branch: `v0.4-dev`

## Repository Rules

- Follow root `AGENTS.md`.
- Do not stage or commit `data` submodule changes unless explicitly approved in the current turn.
- Do not stage generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`, `*.so`,
  `*.dll`, `*.npy`, `*.npz`, or `.DS_Store`.
- Always run `git diff --check` before finalizing code changes.
- Before committing code, run `git status --short` and `git diff --cached --stat` and verify the
  staged set excludes `data`, generated files, vendored runtime libraries, large binaries, and
  unrelated user changes.

## User Report

The user first reported that `./build/examples/c/features/user_scale` showed layout scaling issues:

- Increasing `user_scale` increased margins faster than expected.
- Horizontal spacing between the left axis and the left title did not depend on scale.

Those were addressed in earlier commits:

- `10df239e3 Fix user scale layout reserve handling`
- `7c765f9ac Scale axis text gaps with user scale`
- `e381f5838 Enable alpha blending for reference grids`
- `aa90f4b38 Clip segment quads to the full view volume`
- `7d5954c68 Tune feature example framing`

After that, the user ran the live feature example and reported this Vulkan validation error:

```text
./build/examples/c/features/user_scale
scenario_runner: feature_user_scale requirements: marker,native-view
validation layer: vkQueueSubmit2(): pSubmits[0] command buffer ... expects VkImage ...
to be in layout VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL--instead, current layout is
VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL.
VUID-vkCmdDraw-None-09600
```

The example has a GUI slider that calls `dvz_view_set_user_scale()` from
`examples/c/features/user_scale.c`.

## Reproduction Status

Manual slider interaction appears important. Bounded non-interactive runs did not reproduce the
validation warning:

```sh
./build/examples/c/features/user_scale --live 120
./build/examples/c/features/user_scale --live 120 --user-scale 2.0
./build/examples/c/features/user_scale --live 180
./build/examples/c/features/user_scale --png --user-scale 0.75
./build/examples/c/features/user_scale --png --user-scale 1.5
./build/examples/c/features/user_scale --png --user-scale 2.25
```

`just build` passed after the WIP runtime edits described below.

## Important Code Paths

- `src/app/app.c`
  - `_app_draw()` emits one frame per native-view frame.
  - It sets `cfg.runtime_resource_scope_id = _app_frame_runtime_scope(frame)`.
  - That means graph runtime textures are scoped per frame target.

- `src/scene/runtime/graph_resources.c`
  - `_runtime_scope_key()` appends `_scope_%016PRIx64` to graph resource keys.
  - `_graph_resolve_texture_2d()` uses that scoped key for graph textures, including depth.

- `src/scene/techniques/graph_wboit.c`
  - `_scene_technique_emit_blended_frame_graph()` creates `fig0_p0.depth`.
  - For source-over transparent passes after opaque depth, depth should be `LOAD + READ`.

- `src/scene/runtime/graph_resources.c`
  - `_stream_apply_graph_depth()` maps graph depth attachment access to DRP2 depth access.

- `src/drp2/pipeline.c`
  - `_vklite_build_bind_group_descriptors()` writes sampled depth descriptors with
    `VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL`.

- `src/drp2/transfer.c`
  - `_vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ)` maps to
    `VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL`.
  - `_vklite_transition_image_access()` is the tracked image-layout transition helper.

- `src/drp2/pass.c`
  - `_binding_texture_access()` maps sampled depth textures to
    `DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ`.
  - `_vklite_begin_render_pass()` builds attachment layouts and transitions bind-group textures
    before `vkCmdBeginRendering`.

## Current Hypothesis

The error is likely a live-runtime synchronization issue involving a depth texture whose descriptor
was written as `READ_ONLY_OPTIMAL` but whose image is still in `ATTACHMENT_OPTIMAL` at draw time.

The suspicious mechanism is not the `user_scale` value itself. The scale change forces live frame
reemission, while native-view app emission also uses per-frame scoped graph resources. That can leave
old bind groups and old graph texture objects alive in the persistent DRP2/vklite runtime while new
frame-scoped graph resources are emitted.

One investigated idea: `src/drp2/pass.c` transitions all live bind-group textures before every render
pass. That is conservative but can include stale bind groups from previous frames. A WIP patch changes
this to scan the current command stream from `BeginRenderPass` to matching `EndRenderPass` and
transition only bind groups actually set in that pass.

This WIP is not proven. It may be useful, but do not treat it as complete.

## Local WIP State At Handoff

The workspace currently has uncommitted WIP edits in:

```text
src/drp2/_runtime.h
src/drp2/backend.c
src/drp2/pass.c
src/drp2/tests/test_drp2.c
src/drp2/tests/test_drp2.h
src/drp2/tests/vklite_runtime.c
```

There is also an existing dirty `data` submodule (` m data`). Do not stage it without explicit
approval.

The WIP runtime patch:

- Changes `_vklite_begin_render_pass()` to receive `const DvzDrp2CommandStream* stream`.
- Changes the vklite backend dispatch in `src/drp2/backend.c` to pass the stream.
- Adds `_transition_render_pass_bind_group_textures()` in `src/drp2/pass.c`.
- Removes the old all-live-bind-groups transition call from begin-render-pass.

The WIP test addition:

- Adds `test_drp2_runtime_vklite_ignores_unused_render_pass_bind_groups`.
- Registers it in `src/drp2/tests/test_drp2.c` and declares it in `test_drp2.h`.
- The test currently passes its assertions but then crashes at teardown.

Crash detail from `lldb`:

```text
PASS  drp2/vklite-runtime/ignores_unused_render_pass_bind_groups
Process stopped: EXC_BAD_ACCESS
frame #0: dvz_device_wait(device=0x...) at src/vk/device.c:786
vkDeviceWaitIdle(device->vk_device);
```

Likely cause: the test uses `drp2_test_vklite_fixture_runtime()` but destroys the fixture-owned
runtime/GPU context manually. Fix the test teardown before committing the WIP test. Compare nearby
shared-fixture tests in `src/drp2/tests/vklite_runtime.c`; they often destroy the stream only.

## Validation Already Run

Passing:

```sh
just build
just test drp2_runtime_vklite_samples_read_only_active_depth
./build/examples/c/features/user_scale --live 180
./build/examples/c/features/user_scale --png --user-scale 0.75
./build/examples/c/features/user_scale --png --user-scale 1.5
./build/examples/c/features/user_scale --png --user-scale 2.25
```

Problematic:

```sh
./build/testing/dvztest_drp2 ignores_unused_render_pass_bind_groups
```

This crashed after reporting PASS because of the teardown issue above.

Filters such as `just test ignores_unused` and
`just test drp2/vklite-runtime/ignores_unused_render_pass_bind_groups` did not select the new case via
`tasks/test_driver.py`; the binary listed the case correctly with:

```sh
./build/testing/dvztest_drp2 --list | rg -n "unused|bind|render_pass|vklite-runtime"
```

## Suggested Next Steps

1. Fix the WIP test teardown first. If it uses a shared fixture runtime, do not destroy the runtime or
   GPU context in the test body.
2. Re-run:
   ```sh
   ./build/testing/dvztest_drp2 ignores_unused_render_pass_bind_groups
   just test drp2_runtime_vklite_samples_read_only_active_depth
   just test drp2
   ```
3. Reproduce the original manual slider warning if possible with `DVZ_DRP2_TRACE=full` or app
   recording. The non-interactive live runs have not reproduced it.
4. If the pass-local bind-group transition patch is kept, verify no stream relies on descriptor sets
   being bound before `BeginRenderPass`; scene emission appears to set required groups inside each
   pass.
5. Run required hygiene before any code commit:
   ```sh
   git diff --check
   git status --short
   git diff --cached --stat
   ```
