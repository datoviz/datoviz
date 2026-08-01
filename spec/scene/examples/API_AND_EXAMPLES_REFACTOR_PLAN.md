# API And Examples Conventions

Status: v0.4 API/example refactor implemented; this file retains the resulting conventions. Updated: 2026-08-01.

## Public Example Structure

Public C examples use `visuals`, `features`, `composites`, `showcases`, and `advanced`; experimental or diagnostic material stays in `lab`. Canonical IDs and release/browser classification live in `examples/c/MANIFEST.yaml`. Examples use public headers and normal runtime paths rather than local parsers, shape builders, or compatibility shims.

## API Conventions

1. Descriptor initializers return complete defaults and constructors use them as the canonical path.
2. Short constructors remain only when defaults are obvious and the helper is a thin wrapper.
3. Units are explicit: logical pixels, framebuffer pixels, device scale, user scale, render scale, seconds, bytes, counts, offsets, and strides are not interchangeable.
4. Public geometry and import examples use `geom` and `fileio`; runtime and host examples use Canvas, stream, app, window, video, and optional-provider boundaries.
5. Examples do not preserve v0.3 names or duplicate the scene, DRP2, vklite, Canvas, or presentation stack.

## Size And Execution Contract

Live and offscreen paths share canonical example/scenario construction. Requested logical size is distinct from framebuffer size; deterministic captures state their output extent. Animation and continuous compute scheduling are explicit. Examples that require prepared data fail with an actionable preparation command rather than silently substituting a different public result.

## Validation

Every public example must compile, run through its declared smoke/scenario route where applicable, have accurate manifest status and requirements, and keep documentation/generated snippets synchronized. Public promotion additionally requires deterministic capture or interaction proof, ownership-safe cleanup, required asset provenance, and honest WebGPU/native classification.

Remaining optional polish belongs in [V04_INTERACTION_AND_SHOWCASE_PLAN.md](V04_INTERACTION_AND_SHOWCASE_PLAN.md), while release sequencing belongs in `agents/now/STATUS.md`.
