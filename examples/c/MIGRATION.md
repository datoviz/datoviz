# C Example Reset Migration

The v0.4 C example tree has been reset so public examples are separated from older smoke-test-style
source material.

## Public examples kept active

| Scenario ID | Source | Role |
| --- | --- | --- |
| `path_axes_2d` | `features/axes_2d.c` | Feature proof. |
| `linked_panels_axes_panzoom` | `features/panel_linked_axes.c` | Feature proof. |
| `scale_bar` | `features/scalebar.c` | Feature proof. |
| `image_probe` | `features/image_probe.c` | Feature proof. |
| `marker_picking` | `features/pick_marker.c` | Feature proof. |
| `protein_arcball_viewer` | `showcases/protein.c` | Gallery/showcase seed. |
| `showcase_wind_field` | `showcases/wind_field.c` | Gallery/showcase seed. |

## Lab examples kept buildable

| Source | Previous source |
| --- | --- |
| `lab/axis_lattice_smoke.c` | `tools/axis_lattice_smoke.c` |
| `lab/point_stress.c` | `stress/point_stress.c` |
| `lab/protein_viewer.c` | unchanged |
| `lab/record_dvzr.c` | `tools/record_dvzr.c` |
| `lab/replay_dvzr.c` | `tools/replay_dvzr.c` |
| `lab/scalebar_2d_3d.c` | unchanged |
| `lab/text_msdf_diagnostics.c` | `tools/text_msdf_diagnostics.c` |
| `lab/text_msdf_lab.c` | `tools/text_msdf_lab.c` |

## Legacy archive

| Legacy folder | Contents |
| --- | --- |
| `legacy/visuals/` | Former one-visual smoke examples. |
| `legacy/techniques/` | Former technique/prototype examples. |
| `legacy/showcase/` | Former singular showcase examples and prepare scripts, except `protein.c`. |
| `legacy/tools/` | Low-level/export/debug tool examples no longer in the active tree. |
| `legacy/regression/` | Former regression-style smoke example. |

Legacy examples are not built by default. Promote by copying or moving into `features/`, `visuals/`,
or `showcases/` only after polishing and manifest updates.
