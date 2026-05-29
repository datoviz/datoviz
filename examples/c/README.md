# Datoviz C Examples

This directory contains the canonical native C examples for the Datoviz v0.4 scene, app, runtime,
and low-level rendering stack. The active overhaul process is documented in
`../../spec/scene/examples/EXECUTION.md`.


## Public Lanes

| Lane | Purpose |
| --- | --- |
| `fundamentals/` | Small create, render, update, window, offscreen, resize, and frame-loop examples. |
| `visuals/` | One active visual family per file, with minimal unrelated features. |
| `features/` | Scene/app capabilities such as axes, colorbars, panels, controllers, picking, probing, and selection. |
| `techniques/` | Pass-level rendering behavior such as EDL, SSAO, MSAA, WBOIT, depth cueing, and materials. |
| `showcases/` | Polished, composed, gallery-facing scientific examples. |
| `runtime/` | Hosting, capture, frame callbacks, live windows, video, and app execution examples. |
| `advanced/` | Low-level DRP2, vklite, canvas, stream, interop, and diagnostic examples. |
| `regression/` | Deterministic examples kept primarily for screenshot, readback, or fixture validation. |
| `stress/` | Capacity, performance, long-loop, and repeated-update examples. |
| `lab/` | Historical demos and diagnostics kept buildable as source material, not public gallery items. |


## Transitional Folders

Some existing folders predate the final lane split:

| Folder | Transitional handling |
| --- | --- |
| `showcase/` | Historical singular showcase folder. Promote selected files to `showcases/` when they are polished and indexed. |
| `tools/` | Keep developer tools and low-level generators here until they move to `runtime/`, `advanced/`, or remain intentionally non-gallery tools. |


## Metadata

`MANIFEST.yaml` is the initial source for scenario IDs, final lanes, style presets, and validation
expectations. `MIGRATION.md` maps existing files to their intended final role.
