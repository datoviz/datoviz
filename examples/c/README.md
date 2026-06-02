# Datoviz C Examples

This directory is being reset for the v0.4 public example suite. Public examples should be small,
polished, and scenario-indexed; older smoke-test-style examples are parked in `legacy/` as source
material.

## Active folders

| Folder | Purpose |
| --- | --- |
| `features/` | Public feature examples: one API capability or one feature family. |
| `visuals/` | Future public one-visual examples. Old visual smoke tests currently live in `legacy/visuals/`. |
| `composites/` | Public semantic composite examples such as polygons and graphs. |
| `workflows/` | Public compact multi-feature workflows with synthetic or generated data. |
| `showcases/` | Public gallery-facing examples. Synthetic, generated, and domain-flavored fake data are allowed when stated clearly. |
| `scientific/` | Public real-data scientific examples with source, license, attribution, preprocessing, and encoding notes. |
| `lab/` | Non-public experiments, diagnostics, stress/perf demos, and development workbenches. Flat folder only. |
| `legacy/` | Temporary archive of older examples that are not built by default. Promote or delete from here deliberately. |

Shared helpers live directly in this directory (`example_common.*`, `example_style.*`,
`example_gui_controls.*`). There is intentionally no `tools/`, `regression/`, or `stress/` example
lane in the active C tree: tests own regressions, and lab owns temporary diagnostics/stress work.

## Metadata

`MANIFEST.yaml` indexes the current public/lab examples using the canonical scenario IDs from
`../../spec/scene/examples/PLANNING.md`. Required scenarios that are still absent from the manifest
remain explicit gaps in the planning table; `MIGRATION.md` records the reset and where legacy
material went.
