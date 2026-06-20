# DVZ-GSP-003: GSP Query Contract

## Goal

Define and harden the first GSP-compatible Datoviz query contract around unified panel queries,
explicit capability failures, and point-over-image behavior.

## Files To Inspect/Change

| File | Reason |
|---|---|
| `include/datoviz/scene/interaction.h` | public query functions |
| `include/datoviz/scene/types.h` | `DvzQueryRequest`, `DvzQueryResult` |
| `include/datoviz/scene/enums.h` | statuses, profiles, capability flags |
| `src/scene/query/` | queue, freshness, profile selection, readback, result mapping |
| `src/scene/visuals/*/query.c` | family-specific query capabilities and payloads |
| `src/scene/tests/query.c` | focused query tests |
| `spec/scene/interaction/PANEL_QUERY.md` | public semantics |
| `spec/scene/interaction/GPU_QUERY_SYSTEM.md` | detailed query invariants |

## Non-Goals

1. Do not restore public pick/probe APIs.
2. Do not add CPU hit-test fallback for rendered visual queries.
3. Do not claim full hit-stack, text/glyph, MIP, DVR, or two-attachment query support unless it is
   implemented and tested.
4. Do not make WebGPU query parity implicit; capability-gate it.

## Implementation Notes

First GSP conformance case:

1. image visual attached to a panel;
2. point visual over the image;
3. query on point center returns point visual id and item id;
4. query on image-only coordinate returns image visual id and pixel/sample payload where supported;
5. query outside panel returns `DVZ_QUERY_STATUS_OUTSIDE_PANEL`;
6. frontmost unsupported visual returns explicit unsupported status.

Document which `DvzQueryResult` fields are guaranteed for each first-slice family/target/profile.
Do not rely on fields that are present in the struct but not populated by current family code.

## Tests/Validation

1. Focused C test for point-over-image query.
2. Tests for missing query profile and readback failure status.
3. Tests that unsupported frontmost visual does not silently fall through to background.
4. Tests for stale/latest-wins behavior when relevant to hover.
5. If WebGPU route is in scope, add fixture/preflight only for supported profiles.
6. Run narrow query tests and `git diff --check`.

## Stop Conditions

1. Required result fields cannot be populated without a new FramePlan/DRP2 readback shape.
2. Multi-output query payloads become required for the first GSP slice.
3. The query contract would require CPU fallbacks or backend-specific ids.
