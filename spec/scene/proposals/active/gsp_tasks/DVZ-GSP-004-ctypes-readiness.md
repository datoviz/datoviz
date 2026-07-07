# DVZ-GSP-004: Ctypes Readiness

## Goal

Ensure the public Datoviz C API required by a GSP adapter is generated, layout-checked, and smoke
tested through `datoviz.raw`.

## Files To Inspect/Change

| File | Reason |
|---|---|
| `spec/bindings/README.md` | Python binding architecture |
| `spec/bindings/ctypes.yml` | binding policy, layout records, smoke symbols, array facade groups |
| `tools/bindings/extract_api.py` | exported API extraction |
| `tools/bindings/generate_ctypes.py` | Python binding generation |
| `tools/bindings/generate_ctypes_abi.py` | ABI fact generation |
| `tools/bindings/validate_ctypes_abi.py` | layout validation |
| `tools/bindings/ctypes_smoke.py` | import/symbol smoke |
| `tools/bindings/ctypes_render_smoke.py` | offscreen render smoke |
| `testing/test_ctypes_raw_smoke.py` | pytest wrapper |
| `examples/python/raw/` | raw examples |

## Non-Goals

1. Do not make `datoviz.raw` a high-level plotting API.
2. Do not hide C ownership/count/pointer rules behind undocumented Python behavior.
3. Do not require generated files to be hand-edited.
4. Do not add GSP as a Datoviz dependency.

## Implementation Notes

Add GSP-critical layout records and smoke symbols for:

1. `DvzCapabilitySnapshot`;
2. `DvzQueryRequest`;
3. `DvzQueryResult`;
4. `DvzSampledFieldDesc`, `DvzFieldDataView`, and `DvzFieldRegion` if not already emitted;
5. `dvz_query_request`, `dvz_panel_query`, `dvz_scene_poll_query`;
6. the public capability query from `DVZ-GSP-001`.

Add a Python binding smoke that creates a tiny scene, uploads point data, renders offscreen when runtime
is available, and either performs a query or records the exact runtime skip reason.

## Tests/Validation

1. `just ctypes`
2. `just ctypes-check`
3. `just ctypes-smoke`
4. `just ctypes-render-smoke`
5. `pytest -q testing/test_ctypes_raw_smoke.py`
6. `git diff --check`

Use the narrowest subset locally if the full binding workflow is unavailable, and record exact
skips.

## Stop Conditions

1. C API uses by-value records, unions, callbacks, or platform handles that the current generator
   intentionally skips.
2. Query/capability structs cannot be layout-checked without exposing private internals.
3. Offscreen runtime is unavailable; keep import/layout smoke passing and record runtime skip.
