# v0.4 Interaction And Showcase Follow-Up

Status: former implementation plan mostly completed; remaining items are optional polish or post-v0.4 data work and do not block RC3. Updated: 2026-08-01.

This document preserves only unresolved follow-up from the completed examples and interaction campaign. Current release sequencing belongs in `agents/now/STATUS.md`; example classification belongs in `examples/c/MANIFEST.yaml`; browser parity belongs in `spec/scene/integration/WASM_WEBGPU_PARITY_PLAN.md`.

## Completed Foundation

- Public examples use the v0.4 visuals/features/composites/showcases/advanced/lab taxonomy and manifest.
- Orientation gizmo, reference grid, pixel/sphere/mesh-instance selection, bounds overlay, GUI viewport, panel View2D, compute animation, and the named showcase programs exist.
- GUI viewport resize uses synchronized proposed/committed/displayed sizes and retains the last valid frame during resize.
- Equal-aspect View2D fitting and resize behavior are implemented and tested.
- Continuous compute scheduling is explicit rather than inferred from the presence of compute work.
- Small-window and retained-layout hardening has landed.
- Browser routes and status are maintained by the dedicated parity plan and generated matrix.

## Durable Boundaries

- `dvz_visual_bounds()` remains semantic anchor bounds in data/world/visual space. Pixel-expanded rendered bounds are panel- and frame-dependent and should remain an internal overlay calculation unless a concrete public consumer justifies a separate API.
- GUI viewport display changes only after the source target reaches the committed size; pending resize keeps the prior valid image.
- `DVZ_PANEL_VIEW2D_CONTAIN` plus equal aspect preserves all requested data and equal unit scale, allowing empty space on one axis. Cover/crop modes remain future work.
- Raw glyph examples demonstrate atlas quads; semantic text examples remain the recommended text path.
- Compute work is one-shot unless app, scenario, animation callback, or explicit scheduling state requests more frames.
- Raw datasets, downloaded archives, and derived binary payloads stay outside the main repository and unapproved `data` changes remain prohibited.

## Optional Remaining Polish

1. Improve the polygon triangulation example with a clearer concave shape, restrained fill, and distinguishable boundary/interior edges.
2. Improve the raw glyph example with recognizable font-derived letters and add a focused semantic font-atlas example if it adds value beyond current text coverage.
3. Polish orientation-gizmo and reference-grid presentation without expanding into transform manipulation.
4. Keep focused interaction tests for hit identity, misses, hover, selection, clearing, upload dirtiness, and readback identity.

## Post-v0.4 Or Explicitly Promoted Data Work

- Replace embedding-atlas fallback data with a licensed prepared bundle and retained query/overlay proof.
- Replace lipid-brain fallback data with a compact licensed prepared subset derived outside the repository.
- Replace synthetic mouse fallback geometry with licensed extracted mesh/marker data and preprocessing provenance.
- Image-thumbnail LOD, semantic search, dashboards, lasso selection, mesh sub-element picking, Blender runtime loading, skinning, glTF import, and large asset pipelines remain beyond v0.4.

None of this data/showcase work blocks RC3 or RC4 unless the maintainer explicitly promotes an exact item and its asset/provenance scope.
