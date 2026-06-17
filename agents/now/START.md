# Datoviz v0.4 Dispatch

Status: active v0.4 release-candidate preparation.

Use [../../AGENTS.md](../../AGENTS.md) as the mandatory entry point. This file only records the
current branch dispatch.


## Current Position

The active stack is:

```text
scene -> drp2 -> vklite/canvas/stream -> app
```

Native scene, app/offscreen rendering, DRP2 command emission, raw `ctypes`, the proposed
array-aware Python facade, retained textured mesh, text, axes, colorbars, labels, scale bars,
picking/query first slices, and the WebGPU fixture runner are active v0.4 surfaces. The WASM
point/panzoom scene bridge is also active as the first experimental browser scene slice. Treat them
as real implementation, not scaffolding.


## Start Work

1. Use [STATUS.md](STATUS.md) for the Pre-RC1 execution order, current blockers, and active lanes.
2. Use [RELEASE.md](RELEASE.md) for release sequencing.
3. Use [DOCUMENTATION.md](DOCUMENTATION.md) for public documentation gates.
4. Use [../../spec/scene/README.md](../../spec/scene/README.md) before changing scene semantics.
5. Use [../../spec/drp2/README.md](../../spec/drp2/README.md) before changing DRP2 commands,
   schemas, fixtures, or scene DRP2 emission.
6. Use [../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md) before changing
   the top-level Python package, raw-binding generation, or NumPy/array argument adaptation.
7. Use [TEST_REFACTORING.md](TEST_REFACTORING.md) only when explicitly picking up the deferred
   test-suite split checkpoint.


## Guardrails

1. Keep the runtime path unified; do not create parallel renderers, presentation layers, frame
   streams, or Vulkan wrappers.
2. Prefer small generalizations and cleaner subsystem boundaries over ad-hoc patches.
3. Keep examples and focused tests in lockstep with retained v0.4 slices.
4. Do not add texture/resource-name-based render shortcuts. Uploads define resources; render nodes,
   visual descriptors, and draw contracts define draws.
5. For documentation-only work, run `git diff --check` and inspect `git status --short`.
