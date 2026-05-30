# Datoviz C Examples

This directory is being reset for the v0.4 public example suite. Public examples should be small,
polished, and scenario-indexed; older smoke-test-style examples are parked in `legacy/` as source
material.

## Active folders

| Folder | Purpose |
| --- | --- |
| `features/` | Public feature examples: axes, panels, scale bars, probing, picking, annotations, and composed reusable scene capabilities. |
| `visuals/` | Future public one-visual examples. Old visual smoke tests currently live in `legacy/visuals/`. |
| `showcases/` | Public gallery-facing scientific examples. |
| `lab/` | Non-public experiments, diagnostics, stress/perf demos, and development workbenches. Flat folder only. |
| `legacy/` | Temporary archive of older examples that are not built by default. Promote or delete from here deliberately. |

Shared helpers live directly in this directory (`example_common.*`, `example_style.*`,
`example_gui_controls.*`). There is intentionally no `tools/`, `regression/`, or `stress/` example
lane in the active C tree: tests own regressions, and lab owns temporary diagnostics/stress work.

## Metadata

`MANIFEST.yaml` indexes the current public/lab examples. `MIGRATION.md` records the reset and where
legacy material went.
