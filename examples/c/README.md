# Datoviz C Examples

This directory is being reset for the v0.4 public example suite. Public examples should be small,
polished, and scenario-indexed; older smoke-test-style examples are parked in `legacy/` as source
material.

## Active folders

| Folder | Purpose |
| --- | --- |
| `visuals/` | Public one-visual examples: one visual family per file. |
| `features/` | Public feature examples: one isolated capability or technique per file. |
| `composites/` | Public semantic object examples that lower to one or more visuals. |
| `showcases/` | Public composed examples: workflows, scientific stories, real-data examples, and gallery-facing demos. |
| `lab/` | Non-public experiments, diagnostics, stress/perf demos, and development workbenches. Flat folder only. |
| `legacy/` | Temporary archive of older examples that are not built by default. Promote or delete from here deliberately. |

Shared helpers live directly in this directory (`example_common.*`, `example_style.*`,
`example_gui_controls.*`). There is intentionally no `tools/`, `regression/`, or `stress/` example
lane in the active C tree: tests own regressions, and lab owns temporary diagnostics/stress work.
Use manifest tags, not folders, for `workflow`, `scientific`, `real-data`, `simulated`,
`interactive`, `offscreen`, `technique`, and domain labels.

## Metadata

`MANIFEST.yaml` indexes the current public/lab examples using the canonical scenario IDs from
`../../spec/scene/examples/PLANNING.md`. Required scenarios that are still absent from the manifest
remain explicit gaps in the planning table; `MIGRATION.md` records the reset and where legacy
material went.
