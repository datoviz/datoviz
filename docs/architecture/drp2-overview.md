# DRP2 Overview

DRP2 is the active contract between high-level Datoviz producers and rendering runtimes.

The architectural point is simple:

1. scene and other producers emit DRP2,
2. native and browser runtimes execute DRP2,
3. backend-specific logic stays behind the runtime boundary.

This document is only a short architectural overview.

The current source of truth for the DRP2 contract is
[spec/drp2/README.md](/home/cyrille/GIT/Viz/datoviz/spec/drp2/README.md) and the documents it
indexes.

Execution-status and roadmap notes may still live under `agents/`, but they are not the canonical
contract.


## Why DRP2 Exists

The current low-level stack is native and Vulkan-centered, but higher-level code now targets DRP2
instead of directly targeting that low-level stack.

But higher-level Datoviz layers need a future-facing boundary that:

1. avoids direct dependency on Vulkan internals,
2. keeps browser/WebGPU support plausible,
3. supports replay, validation, and conformance testing,
4. lets scene logic remain portable.


## Design Direction

DRP2 should be:

1. backend-agnostic,
2. WebGPU-shaped,
3. validated as a strict contract,
4. narrow in its first stable version.

The main failure mode to avoid is over-designing the protocol around every future ambition at once.


## Current Status

DRP2 is no longer only a design direction or fixture lane.

The current source tree now includes:

1. a human-readable Layer 1 contract,
2. command, lifetime, error, and capability documents,
3. machine-readable schemas,
4. executable conformance fixtures,
5. a C command-stream implementation in `src/drp2`,
6. semantic validation and a native vklite-backed runtime,
7. tests that exercise resource commands, render passes, texture copies, sampling, readback, and
   object-table growth,
8. depth/stencil pipeline and render-pass attachment support used by the scene mesh/primitive path,
9. queue readbacks used by the current point-pick and image-probe request slice.

That makes DRP2 an active contract-definition surface rather than a vague future interface.


## Relationship To Current Work

The current branch priority is hardening the active scene -> DRP2 -> vklite/canvas/app vertical
slice while beginning a narrow WebGPU feasibility lane. DRP2 should remain narrow and executable:
implementation changes that alter contract semantics should update prose, schemas, fixtures, and
focused tests together.


## Relationship To Scene

The scene layer consumes DRP2 through a runtime-facing boundary, not through backend internals.

The current scene source of truth is
[spec/scene/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/README.md).

The intended layering remains:

1. scene and other producers define high-level semantics,
2. producers emit DRP2,
3. runtimes validate and execute DRP2,
4. backend-specific logic stays below that runtime boundary.
