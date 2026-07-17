# Linux SPIR-V Compute Warning Handoff

Status: ready for Linux implementation. Updated: 2026-07-17.


## Context

The macOS arm64 RC candidate run at `1b1ea2eb0` passes all commands but records one Vulkan
best-practices warning while creating the scene compute pipeline:

```text
vkCreateComputePipelines(): pCreateInfos[0].stage is using the SPIR-V Workgroup built-in which
SPIR-V 1.6 deprecated. When using VK_KHR_maintenance4 or Vulkan 1.3+, the new SPIR-V LocalSizeId
execution mode should be used instead.
```

This is not a correctness or synchronization error. Both affected tests execute successfully,
including GPU readback. The warning should nevertheless be removed before final RC evidence because
Datoviz already uses Vulkan 1.3 while runtime GLSL compilation explicitly targets Vulkan 1.0 and
SPIR-V 1.0.


## Reproduction And Current Evidence

The warning is isolated to these tests:

```sh
just test scene/frame-plan-emit/runtime_compute_two_frames_glsl_executes
just test scene/frame-plan-emit/runtime_compute_two_frames
```

It does not appear in the lower-level vklite compute tests checked on the M1.

Relevant source:

- `src/drp2/pipeline.c`: `_vklite_compile_glsl()` sets
  `shaderc_env_version_vulkan_1_0` and `shaderc_spirv_version_1_0`.
- `src/scene/shaders/glsl/compute_copy.glsl`: fixed `local_size_x = 1` compute shader.
- `src/scene/tests/frame_plan_emit.c`: both warning-producing integration tests.
- `CMakeLists.txt`: build-time `glslc` invocation currently has no explicit target environment.
- `src/vk/instance.c`: the default Datoviz instance requests Vulkan 1.3.

Local disassembly confirmed:

```text
Vulkan 1.0 / SPIR-V 1.0 -> OpDecorate %gl_WorkGroupSize BuiltIn WorkgroupSize
Vulkan 1.3 / SPIR-V 1.6 -> deprecated WorkgroupSize built-in absent
```

The SPIR-V specification keeps deprecated features valid but directs 1.6 client APIs away from the
constant `WorkgroupSize` built-in:
<https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#_deprecation>.


## Preferred Implementation

Make one focused commit, expected to touch three files and roughly 10-25 lines:

1. Change runtime shaderc compilation in `src/drp2/pipeline.c` to target Vulkan 1.3 and SPIR-V 1.6.
2. Add matching `glslc` target flags in `CMakeLists.txt` so runtime and build-time shader policies do
   not diverge.
3. Strengthen an existing test in `src/scene/tests/frame_plan_emit.c` to keep this warning out of
   validation output, or add an equivalent focused SPIR-V target assertion.

Do not merely suppress the Vulkan warning. Keep the compiler target consistent with the runtime's
Vulkan 1.3 baseline.


## Linux Validation

Start from a clean `v0.4-dev` checkout at or after `1b1ea2eb0` and use the configured Vulkan SDK:

```sh
just build
just test scene/frame-plan-emit/runtime_compute_two_frames_glsl_executes
just test scene/frame-plan-emit/runtime_compute_two_frames
just test vklite/compute_1
just test vklite/technique_compute_graphics
just test
just spec-check
git diff --check
```

Also disassemble the rebuilt compute shader or a focused probe and confirm that the deprecated
`BuiltIn WorkgroupSize` decoration is absent. Run the strict MkDocs/WASM build separately on this
designated Linux documentation host if that release lane is being completed in the same checkout;
it is not part of the shader fix itself.

After committing, regenerate `just release-candidate 0.4.0rc1` and confirm its test command records
zero error diagnostics and no `WorkgroupSize` warning. Do not tag, upload, publish, or dispatch wheel
workflows without separate explicit approval.


## Risks And Follow-Up

- The source diff is small, but it changes generated SPIR-V for runtime-compiled GLSL; retain the
  full native and spec validation loop.
- Confirm the configured Linux shaderc/glslc versions accept Vulkan 1.3 and SPIR-V 1.6 targets.
- Normal cross-platform wheel CI will provide later Windows and macOS compiler/runtime coverage.
- After the Linux commit lands, pull it on the M1 and rerun only the two affected compute tests plus
  the candidate diagnostic scan.


## Relevant Commits

- `f94ef4de5`: retained-scene reopen runtime fix and physical IPython validation update.
- `1b1ea2eb0`: Python 3.10 release automation hardening and designated documentation-host policy.
- No SPIR-V target implementation commit exists yet.
