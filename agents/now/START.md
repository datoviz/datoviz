# Datoviz v0.4 Dispatch

Status: active v0.4 release candidate.

Use [../../AGENTS.md](../../AGENTS.md) as the mandatory entry point. This file only records the
current branch dispatch.


## Current Position

The active stack is:

```text
scene -> drp2 -> vklite/canvas/stream -> app
```

Native scene, app/offscreen rendering, DRP2 command emission, the generated Python binding
(`datoviz` with NumPy adaptation plus `datoviz.raw` exact calls), retained textured mesh, text,
axes, colorbars, labels, scale bars,
picking/query first slices, and the WebGPU fixture runner are active v0.4 surfaces. The WASM
point/panzoom scene bridge is also active as the first experimental browser scene slice. Treat them
as real implementation, not scaffolding.


## Start Work

1. Use [STATUS.md](STATUS.md) for the RC1 execution order, current blockers, and active lanes.
2. Use [RELEASE.md](RELEASE.md) for release sequencing.
3. Use [DOCUMENTATION.md](DOCUMENTATION.md) for public documentation gates.
4. Use [../../spec/scene/README.md](../../spec/scene/README.md) before changing scene semantics.
5. Use [../../spec/drp2/README.md](../../spec/drp2/README.md) before changing DRP2 commands,
   schemas, fixtures, or scene DRP2 emission.
6. Keep the resolved terminal-IPython hosted close path in macOS RC validation; use
   [HANDOFF_IPYTHON_RUN_CLOSE_HANG.md](HANDOFF_IPYTHON_RUN_CLOSE_HANG.md) only as its completed
   investigation record.
7. Use [../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md) and
   [../../spec/bindings/CTYPES_POLICY.md](../../spec/bindings/CTYPES_POLICY.md) before changing the
   top-level Python package, exact-call binding generation, FFI helpers, or NumPy/array argument
   adaptation.
8. Use [HANDOFF_PUBLIC_API_PRE_RC_AUDIT.md](HANDOFF_PUBLIC_API_PRE_RC_AUDIT.md) as the completed
   pre-RC API cleanup record before changing public headers, exported API, generated C reference,
   generated `ctypes`, or public examples.
9. Use [../../plans/AXIS_GUIDE_VIEWPORT_REFACTOR_PLAN.md](../../plans/AXIS_GUIDE_VIEWPORT_REFACTOR_PLAN.md)
   before changing 2D axes, grid lines, guides, View2D domains, aspect ratio, or plot/panel
   viewport behavior. Keep generated/adornment visual routing semantic and attachment-driven; do
   not reintroduce frame-plan pointer scans over axis, guide, colorbar, legend, panel chrome,
   scale-bar, overlay, or bounds-overlay object fields.
10. Use [HANDOFF_VISUAL_DOCUMENTATION_PASS.md](HANDOFF_VISUAL_DOCUMENTATION_PASS.md) for the
    approved visual-system pilot across Get Started, Core Concepts, Choose Your Layer, and the
    Advanced overview. Complete the pilot and request maintainer review before broad rollout.
11. On a physical RC validation machine, use
    [HANDOFF_RC1_PHYSICAL_WHEEL_SMOKE.md](HANDOFF_RC1_PHYSICAL_WHEEL_SMOKE.md) for current status and
    [the reusable procedure](../../docs/contributors/release-physical-validation.md) for Linux,
    macOS, or Windows execution. The artifact commit may precede the release commit only through an
    explicitly audited artifact-neutral and runtime-neutral diff.
12. Use [HANDOFF_WINDOWS_RC1_WHEELS.md](HANDOFF_WINDOWS_RC1_WHEELS.md) to finish the active Windows
    AMD64/ARM64 shaderc, packaging, deterministic CI, and vcpkg-cache lane. Its first external
    NuGet package write and any push/workflow dispatch require explicit approval.
13. Before any TestPyPI upload, resolve the Release validation/logging blocker in
    [HANDOFF_RC1_RELEASE_SILENCE.md](HANDOFF_RC1_RELEASE_SILENCE.md). Its fix changes native wheel
    bytes, so run `29641789685` is historical evidence rather than the final RC1 payload campaign.


## Guardrails

1. Keep the runtime path unified; do not create parallel renderers, presentation layers, frame
   streams, or Vulkan wrappers.
2. Prefer small generalizations and cleaner subsystem boundaries over ad-hoc patches.
3. Keep examples and focused tests in lockstep with retained v0.4 slices.
4. Do not add texture/resource-name-based render shortcuts. Uploads define resources; render nodes,
   visual descriptors, and draw contracts define draws.
5. For documentation-only work, run `git diff --check` and inspect `git status --short`.
