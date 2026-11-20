# Gallery Reference Review, 2026-11-16

Status: all 69 screenshots outside the reference tolerance have an explicit disposition. Of these, 64 are consistent with intentional source changes, two polygon cases were corrected and locally revalidated on 2026-11-17, and three small raster residuals retain a comparison/disposition gate. This is a review record, not approval to replace canonical media.

## Evidence And Limits

The reviewed development captures were generated at runtime source `4689dc47f`; documentation-only commit `4f39d031e` preserves that runtime. The protected `data` tip is `6a9091c10e08e477af5b07496cac602aefc7c999`. Two independent runs produced 107 byte-identical PNGs and full-pixel/nonblank checks pass. The prior Linux reference provenance records source `c5fcaeb3a` on an NVIDIA RTX 5090 with driver 595.84; current checks use the RTX 5090 with driver 595.99.02. Individual canonical files also retain their later source-specific promotions, including AO, GUI widgets and histogram. The driver difference prevents treating this review as a matched-driver regression campaign.

All 69 reference/current pairs and enhanced differences were inspected in comparison sheets. The 2026-11-16 focused run passes 45/45 tests with zero failures/skips: 15 GUI tests, 11 text-atlas tests and 19 scene offscreen tests covering relevant lighting, AO, peeling, text, colormap and color-role behavior. All 16 course steps also pass reproducible in-tree rendering. Neither passing tests nor visual inspection proves every interaction or backend; exact-candidate, hosted and physical-platform gates remain separate.

Build-local evidence is under `build/gateway-docs/reference-review/`: comparison sheets, `classification.json`, `focused-tests.json`, `focused-tests.log`, diagnostic C sources/binaries and PNGs, and the course checker/smoke logs. These generated artifacts are excluded from Git. No reference image, `data` commit or parent gitlink was changed.

## Findings Requiring Action

The first two findings below describe the 2026-11-16 defects that motivated the completed 2026-11-17 correction. They are historical diagnosis, not outstanding implementation work. The third finding retains a scoped exact-candidate provenance/disposition gate; general benign visual differences have already been accepted, so do not restart that broad review. Canonical promotion remains a separate action.

1. **Polygon data colors inherit lighting.** `src/scene/domain/polygon_composite.c` creates its fill with `dvz_mesh(scene, 0)` and does not choose an unlit material. Mesh defaults are Phong, so `composites_polygon` and `showcases_choropleth` change data colors with scene lighting. For example, the polygon-set green authored as RGB `(92, 189, 132)` is shaded rather than reproduced. An isolated diagnostic copy of each canonical C example changes only the fill role to an unlit material and restores the authored RGB values. This is a concrete semantic concern, not evidence that the scene-owned lighting implementation itself regressed.
2. **The polygon transparency demonstration needs an explicit alpha policy.** The polygon example describes transparent fills and supplies alpha 210/220, but its fill mesh inherits opaque alpha mode. In diagnostic captures, explicit unlit/opaque produces the raw source RGB, while explicit unlit/blended gives RGB `(86, 177, 124)` for the green region and `(34, 139, 164)` for the holed polygon over background `(22, 27, 34)`, matching linear-light source-over composition. Do not approve a reference merely because the new shading resembles an older screenshot.
3. **Three small residuals need disposition, not speculative runtime changes.** Point Cloud has maximum delta 7, with 95% of changed components at delta 3 or less, but changes 30.6494% of components. The grid has maximum delta 19 with only nine components above delta 8; the volume has maximum delta 15 with 145 components above delta 8. No obvious lost geometry or new blank output was observed. Driver/rasterization is a hypothesis rather than an established cause.

Accepted decision, 2026-11-17: the independent architecture audit supersedes the initial unlit/alpha-only fix with the [polygon paint contract and implementation plan](../../spec/scene/semantics/POLYGON_PAINT.md). Stable source-over for both roles also requires explicit item/collection ordering, persistent ownership, and semantic query behavior. The correction and local C/Python/WASM validation are complete as recorded below. At that decision checkpoint, headed-browser acceptance and exact canonical-media approval remained RC3 gates. The configured-provider follow-up below closes the focused browser proof for its recorded routes/environment; exact-candidate validation and canonical promotion remain separate.

For the three residuals, compare the same source on the reference provider/driver when practical, or prepare a new provenance-bearing two-run candidate set and obtain explicit maintainer disposition. Do not loosen the tolerance globally or label an unavailable driver comparison a pass.

## Polygon Paint Implementation, 2026-11-17

Rendering implementation `e919403ab`, query guard `9dbdbce10` with zero-coverage follow-up `7fa7a432b`, regression suite `6ae5c2fd8`, and C/Python example update `c18907140` implement the [accepted contract](../../spec/scene/semantics/POLYGON_PAINT.md). Defaults are unlit source-over without scene depth; item order is fill/stroke per region, while explicit COLLECTION batches fills then boundaries. FramePlan owns ordered spans over shared visual bindings, with existing DRP2 indexed draws; color-only updates reuse cached triangulations and position/index buffers.

Local validation uses Linux, NVIDIA RTX 5090, driver 595.99.02, and Vulkan validation. The complete native suite passes 1,244/1,244 with zero failures/skips, including 777 scene tests. The final focused polygon run passes 10/10, draw-contract checks 2/2, and technique-eligibility checks 2/2. After the zero-effective-alpha query follow-up, the focused query campaign passes 65/65 with zero failures/skips; the earlier full-suite count is retained as its own validation snapshot. Bindings refresh/policy/ABI, shader ABI, specification/DRP2 and boundary checks, generated documentation/manifests, documentation status, and the strict docs build pass. Bounded cppcheck covers four changed implementation units; its nullable-cache warning was addressed and the domain recheck is clean, with the tool's default 12-configuration limit retained.

C captures and Python direct-engine smoke pass for polygon and choropleth. Two independent 1280×720 C capture sets under ignored `build/polygon-paint-review/run-1/` and `run-2/` are byte-identical and were visually inspected: holes and outlines remain intact, and the map's data colors are flat. The polygon green pixel is `(86, 177, 124)` and the holed fill is `(34, 139, 164)`, matching the earlier linear-light alpha diagnostic. Capture SHA256 values are `1822da64b688994d0389812d246c9a0677addae41e3fbf97ee5109bfdbc50954` (polygon) and `dfe84ff28a42eb990b4d3fc29eee6d29370e4040be7129b7c95e99aa7f37699f` (choropleth). These are development candidates, not promoted references.

The large-set fixture retains two visuals for 302 regions. An 80-region ordered set emits 160 draws; COLLECTION coalesces to two draws. A zero-alpha fill gap and an additional hidden region correctly split runs into three and five draws rather than painting skipped geometry. The canonical choropleth explicitly selects COLLECTION and emits two paint draws covering 32,517 fill and 65,898 stroke indices. WASM build and packet smoke pass with exact order/range, source-over blend, and no-depth assertions.

At the implementation checkpoint, both focused browser routes reached `QueueSubmit` but skipped on the previously recorded external WebGPU instance-loss failure. Those results remain unavailable browser rendering proof, not passes. The configured-provider follow-up below subsequently passed both routes headless and headed/Xvfb; it supersedes this initial pending check only for the recorded configuration, not physical interaction or exact frozen-candidate proof.

Rendered polygon region picking remains explicitly unsupported: a panel containing visible drawable paint reports `DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY`, including fixed-controller or query-disabled roles, rather than returning internal triangles or background hits. Hidden, empty, and provably zero-coverage paint must not contribute. A future supported implementation must query combined GPU paint coverage in item order and return semantic region IDs; stored IDs alone are insufficient.

For the next documentation pass, preserve the distinction between default item order and explicit map collection order, the phase limits of `z_layer`, and the panel-wide query limitation. The generated polygon/choropleth pages come from canonical C comments, so edit those comments before regenerating gallery pages. Do not overwrite the ongoing course edits with this lane's changes.

At this implementation checkpoint, browser proof, exact candidate validation, and explicit canonical-asset approval remained. The configured-provider browser follow-up below closes the focused browser proof for its recorded environment; exact candidate and asset approval remain. No `data` commit, gitlink, binary payload, or reference image was staged or promoted by this implementation.

## RC3 Browser Validation And Scope, 2026-11-17

The maintainer approved shipping the current polygon rendering and conservative unsupported paint queries in RC3, while allocating composed GPU paint picking and actual WebGPU device-loss replacement to [RC4](../../spec/scene/slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md). The release remains unfrozen. Required native/Python, installed-consumer, exact-artifact, media review, and unrelated platform gates are unchanged.

The bounded browser investigation reproduced plain offscreen WebGPU clear/readback and offscreen DRP2 replay successfully with the default browser configuration. Plain canvas presentation, DRP2 canvas replay, and live routes then failed with Chrome `SharedImageBackingFactory` swapchain-backing errors and the recorded instance-loss signature. The explicit `--gpu-backend=vulkan` harness profile selects `--use-angle=vulkan --enable-features=Vulkan`; both live routes pass headless and headed/Xvfb using Chrome 156.0.8078.79 and a non-fallback NVIDIA Blackwell adapter. This isolates a reproducible configuration-sensitive presentation boundary; it does not diagnose every Chromium internal failure or establish that Datoviz device-loss recovery works.

Implementation `443a71bee` adds the named opt-in profile, GPU-process initialization/metadata capture, a bounded startup deadline and cleanup, detailed browser exception reporting, and `--require-render`. Strict acceptance converts known instance loss into failure rather than an exploratory skip. The successful choropleth load summary now uses stdout; actual errors remain on stderr. Public runtime/context and device ownership are unchanged.

All four strict live-route checks pass: polygon and choropleth in headless mode, and both in headed mode under Xvfb. The seven-probe diagnostic also passes in both configurations: plain clear/readback, queue completion, offscreen DRP2, plain canvas, DRP2 canvas, and both live routes. Polygon checks compare four known fill colors and one hole/background pixel; both screenshots are nonblank. Raw browser version/arguments, adapter features, device-loss/error events, stderr, and screenshots remain ignored under `build/rc3-browser-diagnosis/headless-vulkan-final/` and `xvfb-vulkan-final/`. Commands and support bounds are recorded in [WebGPU compatibility](../../examples/webgpu/COMPAT.md#rc3-browser-reproduction-2026-11-17).

The default-profile failure is preserved as negative evidence, and a strict default-profile run returns nonzero rather than `SKIP`. Successful configured-provider runs supersede earlier skips only for these routes/configurations; Xvfb is a virtual display, not attended physical interaction. Other browsers, GPU providers, actual device replacement, and exact frozen-candidate browser results remain unclaimed. No browser-validation exclusion is needed for these two passing configured-provider runs; the approved narrow exclusion policy remains available only for separately documented provider failures.

Fresh C captures at 1280×720 repeat byte-for-byte and retain the earlier polygon and choropleth SHA256 values. A single-page old/candidate comparison and exact destination/hash list are prepared locally under `build/rc3-media-review/index.html` and `promotion-candidates.json`. No canonical data file or gitlink has been promoted. The comparison is recorded as completed in the private review history, but the canonical polygon and choropleth files still retain their earlier hashes. Preserve that completed review and prepare exact candidate/destination hashes plus the canonical promotion action; do not infer a `data` commit or gitlink approval from an archived review. Course acceptance, candidate freeze and required artifact/platform proof remain separate.

## Disposition Of All 69 Images

“Expected source change” means the visual difference is consistent with identifiable implementation history and the focused checks; it is not maintainer approval of the replacement image. Font/GUI changes retain readable captions in the inspected captures; the old grid-only axis example is intentionally sparse, rather than a missing data plot.

| Example | Disposition | Group |
| --- | --- | --- |
| `composites_polygon` | Corrected locally; canonical promotion pending | polygon-color |
| `features_alpha_blending` | Expected source change | lighting |
| `features_animation_tracks` | Expected source change | lighting |
| `features_annotation_readout` | Expected source change | font |
| `features_axes_2d` | Expected source change | font |
| `features_axis_labels` | Expected source change | font |
| `features_bars_bands` | Expected source change | font |
| `features_bounds_overlay` | Expected source change | lighting |
| `features_builtin_shapes_2d` | Expected source change | lighting |
| `features_builtin_shapes_3d` | Expected source change | lighting |
| `features_camera_manual` | Expected source change | lighting |
| `features_colorbar` | Expected source change | font |
| `features_controller_arcball` | Expected source change | lighting |
| `features_controller_fly` | Expected source change | lighting |
| `features_controller_turntable` | Expected source change | lighting |
| `features_coordinate_system` | Expected source change | lighting |
| `features_datetime_axis` | Expected source change | font |
| `features_gui_cimgui` | Expected source change | gui |
| `features_gui_controls` | Expected source change | gui |
| `features_gui_data_widgets` | Expected source change | gui |
| `features_gui_viewport` | Expected source change | gui |
| `features_guide_lines` | Expected source change | font |
| `features_guide_spans` | Expected source change | font |
| `features_image_probe` | Expected source change | font |
| `features_isolines` | Expected source change | lighting |
| `features_legend_categorical` | Expected source change | font |
| `features_lighting` | Expected source change | lighting |
| `features_marker_symbols` | Expected source change | font |
| `features_material_mesh` | Expected source change | lighting |
| `features_mesh_texture` | Expected source change | lighting |
| `features_obj_loading` | Expected source change | lighting |
| `features_orientation_gizmo` | Expected source change | lighting |
| `features_overlay_card` | Expected source change | font |
| `features_panel_background` | Expected source change | lighting |
| `features_panel_view2d` | Expected source change | font |
| `features_probe_labels` | Expected source change | font |
| `features_reference_grid` | Hold: residual comparison | raster-residual |
| `features_scalebar` | Expected source change | font |
| `features_scalebar_units` | Expected source change | font |
| `features_selection_mesh_instances` | Expected source change | lighting |
| `features_selection_sphere` | Expected source change | lighting |
| `features_technique_ao` | Expected source change | effects |
| `features_technique_depth_cue` | Expected source change | effects |
| `features_technique_depth_test` | Expected source change | effects |
| `features_technique_edl` | Expected source change | effects |
| `features_technique_msaa` | Expected source change | effects |
| `features_technique_transparency` | Expected source change | effects |
| `features_text_block` | Expected source change | font |
| `features_user_scale` | Expected source change | font |
| `showcases_choropleth` | Corrected locally; canonical promotion pending | polygon-color |
| `showcases_cortical_activity` | Expected source change | lighting |
| `showcases_linked_probe_colorbar` | Expected source change | font |
| `showcases_panel_linked_axes` | Expected source change | font |
| `showcases_point_cloud` | Hold: residual comparison | raster-residual |
| `showcases_protein` | Expected source change | lighting |
| `showcases_scalebar_measurement` | Expected source change | font |
| `showcases_scientific_plotting` | Expected source change | font |
| `showcases_spherical_harmonics` | Expected source change | lighting |
| `showcases_streaming_daq` | Expected source change | showcase-defaults |
| `showcases_surface_grid` | Expected source change | lighting |
| `showcases_terrain_relief` | Expected source change | showcase-defaults |
| `showcases_textured_planet` | Expected source change | lighting |
| `showcases_wind_field` | Expected source change | font |
| `visuals_glyph` | Expected source change | font |
| `visuals_mesh` | Expected source change | lighting |
| `visuals_primitive` | Expected source change | lighting |
| `visuals_sphere` | Expected source change | lighting |
| `visuals_text` | Expected source change | font |
| `visuals_volume` | Hold: residual comparison | raster-residual |

## Group Evidence

| Group | Images | Evidence / next action |
| --- | --- | --- |
| font | 25 | Source Sans 3 / Source Code Pro and deterministic MSDF atlas migration; `808f7ecfa`, `f1fe7f74c`, `3ce8118dc`. |
| gui | 4 | Source fonts, GUI scale/style, docking and transparent host panels; `808f7ecfa`, `d9064facf`, `7020bdc2e`, `acd791cf9`. |
| lighting | 27 | Scene/panel-owned lights and shared material evaluation replace older defaults; `1506c96fb`. Review appearance without restoring old light ownership. |
| effects | 6 | Render-product/GTAO/MSAA/EDL/transparency changes, shared panel lights and effective-alpha corrections; `545c99379`, `1506c96fb`, `d0e904705`. Focused GPU tests pass. |
| showcase-defaults | 2 | Explicit terrain material/default tuning and deterministic showcase animation changes; `26e78f8e6`, `d89db0ea7`, `2bd302627`. |
| polygon-color | 2 | Historical defect: polygon fills inherited lit mesh defaults. The completed 2026-11-17 paint implementation restores unlit data colors and intentional source-over alpha; canonical promotion remains pending. |
| raster-residual | 3 | Small-amplitude raster differences; reference driver 595.84 versus current 595.99.02. Driver causation is unproven; use a matched-source/provider comparison or explicit disposition. |
