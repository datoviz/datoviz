# Public API Pre-RC Audit Handoff

Status: approved pre-RC API/ABI cleanup campaign.

Purpose: preserve the July 2026 user-side public API consistency audit and give a future agent a
concrete plan to implement, validate, commit, and push the cleanup. The audit focused on function
naming, signatures, argument ordering, constness, ownership, raw `ctypes` shape, and legacy or
accidental public symbols.

Primary recommendation: break API/ABI now where the current surface is accidental, transitional,
hard to bind correctly, or inconsistent with v0.4 architecture. Do not preserve v0.3 compatibility
when it conflicts with a cleaner v0.4 surface.

Maintainer decision, July 2026: use the pre-RC window aggressively. It is critically important that
the v0.4 API is internally consistent before RC1, even if that means broad source/ABI breaks across
examples, docs, tests, raw `ctypes`, and public C headers.


## Execution Authorization

Approved branch: `api/pre-rc-cleanup`, created from current `v0.4-dev`.

Approved workflow:

1. Make local checkpoint commits by coherent API wave.
2. Push the branch when checkpoint validation passes.
3. Do not push directly to `v0.4-dev`.
4. Do not add compatibility aliases by default. Add a temporary source alias only if it prevents a
   short-lived build dead end, does not preserve ABI, and is removed in the same campaign.
5. Leave existing dirty `data` submodule state and untracked `paper/paper.pdf` unstaged and
   untouched unless the maintainer explicitly approves those paths in the current turn.

Expected implementation shape: roughly 8-11 checkpoint commits. Prefer smaller commits when a wave
changes public headers, generated bindings, generated C reference, examples, and tests together.

Use `spec/api/status.yml` as the API tiering source of truth. Do not create a second stable,
advanced, experimental, or internal classification table in agent notes, docs, or binding policy.

Before and after each major wave, record an exported-header/symbol delta. Removed ABI is allowed,
but it must be deliberate and visible in the wave notes or commit message.


## Current Progress

Last updated: 2026-07-05 on `api/pre-rc-cleanup`.

Completed checkpoints:

1. `d24d1ce86` `agents: authorize pre-rc api cleanup branch`
   - Authorized `api/pre-rc-cleanup` as the execution branch and recorded branch guardrails.
2. `fe425ff78` `agents: prune completed pre-rc handoffs`
   - Removed stale completed handoff files from the active `agents/now/` dispatch set.
3. `ffda98d5d` `agents: record api cleanup baseline`
   - Recorded the API cleanup baseline before public-surface edits.
4. `c3caf968d` `api: demote object container internals`
   - Moved `DvzObject`/`DvzContainer` declarations out of installed public headers.
   - Removed object/container from generated C docs and raw `ctypes`.
   - Refreshed and validated bindings with `just ctypes` and `just ctypes-check`.
   - Deferred dynamic symbol hiding because split dylib linkage still needs internal
     `dvz_obj_*`/`dvz_container_*` exports across private library boundaries.
5. `7db8bf535` `scene: route visual family mapping through registry`
   - Fixed the scene visual-boundary guard by moving visual family/type mapping into the visual
     registry instead of maintaining root-facade enum switches.
   - Validation passed: scene visual boundary checker, `just spec-check`, `just build`,
     `just test scene/scene-graph`, and `git diff --check`.
6. `a703f01e1` `api: remove legacy scene clock aliases`
   - Removed `DVZ_CLOCK_REALTIME` and `DVZ_CLOCK_OFFLINE` from `DvzSceneClockMode`.
   - Updated legacy/lab examples, scene app tests, the portable scenario runner spec, generated
     raw `ctypes`, and generated C API docs to use `DVZ_SCENE_CLOCK_*` names.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just build`, `just test app`,
     `just test scene/animation`, `just spec-check`, and `git diff --check`.
7. `6ab5814aa` `api: remove public mock data helpers`
   - Removed `include/datoviz/math/mock.h`, `src/math/mock.c`, and the `dvzmath.h` umbrella include.
   - Regenerated raw `ctypes` and generated C API docs, removing 12 exported mock-data helpers from
     the public API surface.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just configure`, `just build`, `just test math`,
     `just spec-check`, stale-reference scan, and `git diff --check`.
8. `93c706d6a` `api: remove transitional texture wrappers`
   - Removed `dvz_visual_set_texture_rgba8()` and `dvz_visual_set_texture_r32f()` from the public C
     header, generated raw `ctypes`, and generated C API docs.
   - Kept the internal/wasm wrapper path private and migrated public C examples/docs to sampled
     fields plus `dvz_visual_set_field()`.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/fields`,
     `just test scene/scene-graph`, `just test app`, `just spec-check`, stale-reference scan, and
     `git diff --check`.
9. `0453620fd` `api: collapse panel view2d descriptor API`
   - Removed the compact `DvzPanelView2D` public struct, `dvz_panel_view2d()`, and
     `dvz_panel_set_view2d_desc()`.
   - Made `dvz_panel_set_view2d()` take `const DvzPanelView2DDesc*`, and migrated examples/tests,
     generated raw `ctypes`, and generated C API docs.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/axis`,
     `just test scene/scene-graph`, `just test app`, `just spec-check`, stale-reference scan, and
     `git diff --check`.
10. `11bbfd8b4` `api: normalize scale-bar API spelling`
   - Renamed the public `dvz_scalebar*` family to `dvz_scale_bar*`, matching the `DvzScaleBar`
     type spelling.
   - Renamed the raw FFI helper from `dvz_ffi_scalebar_desc()` to
     `dvz_ffi_scale_bar_desc()`.
   - Migrated examples, tests, binding policy, generated raw `ctypes`, and generated C API docs.
   - Exported symbol delta: removed `dvz_scalebar`, `dvz_scalebar_desc`,
     `dvz_scalebar_set_dimension`, `dvz_scalebar_set_anchor`, `dvz_scalebar_set_units`,
     `dvz_scalebar_set_duration_units`, and `dvz_ffi_scalebar_desc`; added the corresponding
     `dvz_scale_bar*` and `dvz_ffi_scale_bar_desc` symbols.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/interaction`,
     `just spec-check`, stale-reference scan, and `git diff --check`.
11. `256b4e018` `api: flatten scale descriptor styling`
   - Removed nested `DvzFormatDesc format` from `DvzScaleDesc`; callers now use
     `dvz_scale_set_format()` after scale creation.
   - Removed nested `DvzTextStyle label_style`, `DvzTextPlacement placement`, and
     `DvzFormatDesc format` from `DvzScaleBarDesc`.
   - Added retained scale-bar setters: `dvz_scale_bar_set_label_style()`,
     `dvz_scale_bar_set_placement()`, and `dvz_scale_bar_set_format()`.
   - Exported symbol delta: added the three retained `dvz_scale_bar_set_*` setters above; no
     exported functions were removed in this checkpoint, but `DvzScaleDesc` and
     `DvzScaleBarDesc` ABI layout changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/fields`,
     `just test scene/interaction`, `just spec-check`, stale-reference scan, and
     `git diff --check`.
12. `977881068` `api: flatten panel axes descriptor`
   - Removed nested `DvzAxisTickPolicy tick_policy`, `DvzAxisStyle x_style`, and
     `DvzAxisStyle y_style` from `DvzPanelAxes2DDesc`.
   - Kept `dvz_panel_set_axes_2d()` as the common X/Y axes helper: it now applies the default tick
     policy plus grid-enabled default styles, sets labels, and enables both axes.
   - Custom axis policy/style now stays on the existing explicit setters:
     `dvz_axis_set_tick_policy()` and `dvz_axis_set_style()`.
   - Exported symbol delta: no exported functions were added or removed; `DvzPanelAxes2DDesc` ABI
     layout changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test axis`,
     `just test scene/scene-graph`, `just spec-check`, stale-reference scan, and
     `git diff --check`.
13. `26adba228` `api: flatten graph edge tessellation style`
   - Replaced nested `DvzBezierTessellationDesc tessellation` in `DvzGraphEdgeStyle` with scalar
     `tessellation_segment_count` and `tessellation_tolerance` fields.
   - Kept graph internals on the existing `DvzBezierTessellationDesc` representation for path
     lowering.
   - Exported symbol delta: no exported functions were added or removed; `DvzGraphEdgeStyle` ABI
     layout changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/scene-graph`,
     `just example-c composites/graph --png`, `just spec-check`, stale-reference scan, and
     `git diff --check`.
14. `4afdfb53d` `api: flatten selection visual style`
   - Replaced nested `DvzItemStateVisualStyle selected` and `unselected` fields in
     `DvzSelectionVisualStyle` with prefixed scalar style fields.
   - Kept hover styling on `DvzItemStateVisualStyle`; selection internals convert the flattened
     public style back to item-state shader payloads.
   - Preserved the neutral no-active-selection render path while keeping
     `dvz_selection_visual_style()` as the public default that dims unselected items.
   - Exported symbol delta: no exported functions were added or removed; `DvzSelectionVisualStyle`
     ABI layout changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/interaction`,
     `just test app`, `just spec-check`, stale-reference scan, and `git diff --check`.
15. `343f32154` `api: split annotation text styling from descriptors`
   - Removed nested `DvzTextStyle style` and `DvzTextPlacement placement` from
     `DvzAnnotationDesc` and `DvzLabelDesc`.
   - Added retained annotation setters: `dvz_annotation_set_style()` and
     `dvz_annotation_set_placement()`.
   - Migrated examples, docs, guide internals, scale-bar construction, tests, generated raw
     `ctypes`, and generated C API docs.
   - Exported symbol delta: added `dvz_annotation_set_style` and
     `dvz_annotation_set_placement`; no exported functions were removed. `DvzAnnotationDesc` and
     `DvzLabelDesc` ABI layouts changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/interaction`,
     `just test app`, `just spec-check`, stale-reference scan, and `git diff --check`.
16. `787982697` `api: flatten font defaults`
   - Replaced nested `DvzFontDesc sans` and `mono` in `DvzFontDefaults` with prefixed scalar font
     default fields.
   - Kept `DvzFontDesc` unchanged for explicit font creation; scene text and text blocks now
     reconstruct an internal `DvzFontDesc` from flattened defaults.
   - Exported symbol delta: no exported functions were added or removed; `DvzFontDefaults` ABI
     layout changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test app`, `just test gui`,
     `just test scene/interaction`, `just spec-check`, stale-reference scan, and
     `git diff --check`.
17. `ebd35bb00` `api: split overlay card styling from descriptors`
   - Removed `const DvzOverlayCardStyle* style` from `DvzOverlayCardDesc`.
   - Kept card creation focused on text, placement, layout, and card flags; styling now uses the
     retained `dvz_overlay_card_set_style()` path after creation.
   - Migrated overlay examples, app/interaction tests, generated C API docs, and ABI validation.
   - Exported symbol delta: no exported functions were added or removed; `DvzOverlayCardDesc` ABI
     layout changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/interaction`,
     `just test app`, `just spec-check`, stale-reference scan, and `git diff --check`.
18. `8642a2efd` `api: flatten panel view3d descriptor`
   - Replaced nested `DvzCameraDesc camera` in `DvzPanelView3DDesc` with `DvzCameraView view` and
     `DvzCameraProjection projection`.
   - Kept `dvz_panel_set_view3d_desc()` internally reconstructing a local `DvzCameraDesc` so the
     existing panel-owned camera path remains unchanged.
   - Updated generated raw `ctypes`, generated C API docs, and scene-graph view state tests.
   - Exported symbol delta: no exported functions were added or removed; `DvzPanelView3DDesc` ABI
     layout changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test scene/scene-graph`,
     `just spec-check`, stale-reference scan, and `git diff --check`.
19. `cb58878f6` `api: flatten app font defaults config`
   - Replaced nested `DvzFontDefaults font_defaults` in `DvzAppConfig` with scalar `font_*`
     defaults fields.
   - App creation and GUI creation reconstruct internal `DvzFontDefaults` values at the existing
     scene/gui boundaries.
   - Updated generated raw `ctypes`, generated C API docs, app/gui tests, and the null-default
     invariant plan wording.
   - Exported symbol delta: no exported functions were added or removed; `DvzAppConfig` ABI layout
     changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test app`, `just test gui`,
     `just spec-check`, stale-reference scan, and `git diff --check`.
20. `a2d6863e5` `api: remove external surface from view descriptor`
   - Removed `const DvzWindowExternalSurfaceInfo* external_surface` from `DvzViewDesc`.
   - Kept hosted native-surface creation on the dedicated `dvz_view_external_surface()` API, which
     now calls the internal external-surface view path directly.
   - Updated generated raw `ctypes` and generated C API docs.
   - Exported symbol delta: no exported functions were added or removed; `DvzViewDesc` ABI layout
     changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test app`, `just spec-check`,
     stale-reference scan, and `git diff --check`.
21. `29b84d068` `api: flatten view size descriptor fields`
   - Replaced nested `DvzViewSizeDesc size` in `DvzViewDesc` with scalar `size_*` fields.
   - Kept `DvzViewSizeDesc` as the reusable value record for policy constructors and internal
     view-size resolution; app internals reconstruct it from the flattened view descriptor.
   - Left the older `logical_*` and `framebuffer_*` scalar fields in place pending a separate
     view-size API simplification decision.
   - Exported symbol delta: no exported functions were added or removed; `DvzViewDesc` ABI layout
     changed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test app`, `just spec-check`,
     stale-reference scan, and `git diff --check`.
22. `2eacad217` `api: make canvas flags atomic bits`
   - Changed `DVZ_CANVAS_FLAGS_FPS` and `DVZ_CANVAS_FLAGS_MONITOR` from compound values that
     implicitly included `DVZ_CANVAS_FLAGS_IMGUI` to independent bit flags.
   - Updated generated raw `ctypes` and generated C API docs.
   - Exported symbol delta: no exported functions were added or removed; `DvzCanvasFlags` enum
     values changed before RC1.
   - Validation passed: `just ctypes`, `just ctypes-check`, `python3 tools/build_api_c.py`,
     `python3 tools/build_api_c.py --check`, `just build`, `just test canvas`, `just spec-check`,
     stale-reference scan, and `git diff --check`.
23. `c437357b3` `bindings: treat read_gz return as owned pointer`
   - Added `dvz_read_gz()` to the raw `ctypes` owned-return policy with `dvz_memory_free()` as the
     destroy function.
   - Regenerated raw `ctypes` so `dvz_read_gz.restype` is `ctypes.c_void_p` instead of
     `ctypes.c_char_p`, preserving the allocator-owned pointer for explicit freeing.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just ctypes-smoke`,
     `just docs-api-check`, `python3 tools/build_api_c.py --check`, focused generated-binding
     assertion, and `git diff --check`.
24. `3a17d5b14` `api: rename glfw view API to window`
   - Renamed stable app-facing `DVZ_VIEW_GLFW` and `dvz_view_glfw()` to backend-neutral
     `DVZ_VIEW_WINDOW` and `dvz_view_window()`.
   - Migrated examples, tests, docs, generated raw `ctypes`, generated C API docs, and the Python
     facade `run()` helper.
   - Exported symbol delta: removed `dvz_view_glfw`; added `dvz_view_window`. `DvzViewKind` enum
     spelling changed before RC1 while preserving the underlying numeric value.
   - Validation passed: stale old-name scan, `just ctypes`, `just ctypes-check`,
     `python3 tools/build_api_c.py`, `python3 tools/build_api_c.py --check`, `just build`,
     `just spec-check`, `just ctypes-smoke`, `just docs-api-check`,
     `just test app/view_size_policy_resolve`, `just test gui/widget_wrapper_symbols`, and
     `git diff --check`.
   - Local validation caveat: full `just test app` and `just test gui` were started concurrently,
     then rerun serially for `app`; both attempts hung in the GLFW-backed
     `gui/config_inherits_app_font_defaults` process after earlier checks had passed, so the stuck
     sessions were terminated instead of used as pass/fail evidence.
25. `52e38e358` `api: normalize jpeg decode byte arguments`
   - Reordered `dvz_load_jpeg()` from `(size, bytes, width, height)` to
     `(bytes, size_bytes, width, height)`, matching `dvz_load_png()` and the byte-buffer API
     convention.
   - Updated internal callers, file I/O tests, generated raw `ctypes`, and generated C API docs.
   - Exported symbol delta: no exported functions were added or removed; `dvz_load_jpeg()` ABI
     signature changed before RC1.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just ctypes-smoke`,
     `python3 tools/build_api_c.py`, `python3 tools/build_api_c.py --check`, `just test fileio`,
     `just build`, `just spec-check`, and `git diff --check`.
26. `2a340b51c` `api: make DvzAlpha a typedef`
   - Replaced public `DvzAlpha` macro with a real `typedef uint8_t DvzAlpha`.
   - Updated generated C API type docs.
   - Exported symbol delta: no exported functions were added or removed; public type metadata
     changed from macro to typedef before RC1.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just ctypes-smoke`,
     `python3 tools/build_api_c.py`, `python3 tools/build_api_c.py --check`, `just build`,
     `just spec-check`, and `git diff --check`.
27. `2ac43481c` `api: prefix support math macros`
   - Renamed public support macros to `DVZ_*`: `DVZ_2PI`, `DVZ_INV_255`, `DVZ_EPSILON`,
     `DVZ_MIN`, `DVZ_MAX`, `DVZ_CLIP`, `DVZ_GB`, `DVZ_MB`, `DVZ_KB`, and
     `DVZ_PRETTY_SIZE_THRESHOLD`.
   - Removed the file-scope `_PRETTY_SIZE` buffer from the public header; `dvz_pretty_size()` now
     uses a function-local thread-local buffer where supported.
   - Migrated internal Datoviz call sites and legacy examples.
   - Exported symbol delta: no exported functions were added or removed; public macro spellings
     changed before RC1.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just ctypes-smoke`,
     `python3 tools/build_api_c.py`, `python3 tools/build_api_c.py --check`, `just docs-api-check`,
     `just build`, `just test math`, `just test geom`, `just spec-check`, and `git diff --check`.
28. `bff88625d` `api: export complete box helper surface`
   - Added `DVZ_EXPORT` to the public `dvz_box_normalize_2D()`,
     `dvz_box_normalize_polygon()`, `dvz_box_normalize_3D()`, and `dvz_box_inverse()` declarations.
   - Made `dvz_box_normalize_1D()` take `const double* pos`.
   - Removed the public/source `dvz_box_print()` debug helper instead of stabilizing stdout output.
   - Regenerated raw `ctypes` and generated C API docs.
   - Exported symbol delta: added four `dvz_box_*` symbols; no exported functions were removed.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just ctypes-smoke`,
     `python3 tools/build_api_c.py`, `python3 tools/build_api_c.py --check`, `just docs-api-check`,
     `just build`, `just test math`, `just spec-check`, and `git diff --check`.
29. `4fb4be416` `api: use unsigned dimensions for PPM reads`
   - Changed `dvz_read_ppm()` width/height outputs from `int*` to `uint32_t*`, matching the PNG and
     JPEG decode APIs.
   - Tightened the PPM parser to zero outputs on entry, reject invalid dimensions, close files on
     parse errors, and fail on short RGB payloads.
   - Regenerated raw `ctypes` and generated C API docs.
   - Exported symbol delta: no exported functions were added or removed; `dvz_read_ppm()` ABI
     signature changed before RC1.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just ctypes-smoke`,
     `python3 tools/build_api_c.py`, `python3 tools/build_api_c.py --check`, `just docs-api-check`,
     `just build`, `just test fileio`, `just spec-check`, and `git diff --check`.
30. `dc1617d8f` `api: normalize embedded resource sizes`
   - Changed embedded-resource accessor size outputs from `unsigned long*` to `DvzSize*`.
   - Updated the CMake resource generators and scene shader/text call sites.
   - Removed stale public `dvz_resource_texture()` and `dvz_resource_testdata()` declarations,
     which were documented/generated but not exported by the current library.
   - Regenerated raw `ctypes` and generated C API docs.
   - Exported symbol delta: removed the stale generated raw/docs surface for
     `dvz_resource_texture()` and `dvz_resource_testdata()`; no real exported library symbols were
     removed. The four real `dvz_resource_shader/glsl/wgsl/font` ABI signatures changed before RC1.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just ctypes-smoke`,
     `python3 tools/build_api_c.py`, `python3 tools/build_api_c.py --check`, `just docs-api-check`,
     regenerated build-tree `_shaders.c`, `_glsl_shaders.c`, `_wgsl_shaders.c`, and `_fonts.c`,
     `just build`, `just test fileio`, `just spec-check`, resource symbol-table check, and
     `git diff --check`.
   - Validation caveat: `just test scene/shaders` selected 0 tests in this tree.
31. `f7e412563` `api: make sampled field destroy void`
   - Changed `dvz_sampled_field_destroy()` from returning `bool` to returning `void`, matching the
     other destroy APIs.
   - Updated source callers, scene field tests, generated raw `ctypes`, and generated C API docs.
   - Exported symbol delta: no exported functions were added or removed; the
     `dvz_sampled_field_destroy()` ABI signature changed before RC1.
   - Validation passed: `just ctypes`, `just ctypes-check`, `just ctypes-smoke`,
     `python3 tools/build_api_c.py`, `python3 tools/build_api_c.py --check`,
     `just docs-api-check`, `just build`, `just test scene/fields`, `just spec-check`, and
     `git diff --check`.

Current working-tree noise to leave untouched unless explicitly approved in the current turn:

- dirty `data` submodule state;
- untracked `paper/paper.pdf`.

Next recommended checkpoint: choose whether to simplify `DvzViewDesc` size configuration further by
removing the older duplicate `logical_*`/`framebuffer_*` scalar fields or adding a public helper for
copying `DvzViewSizeDesc` values into the flattened descriptor.


## Maintainer Decisions

Use these defaults unless the code audit finds a stronger local reason:

1. Remove unused, legacy, transitional, or accidental public APIs aggressively.
2. Do not preserve v0.3 compatibility aliases when they weaken v0.4 naming, architecture, binding
   safety, ownership clarity, or consistency.
3. Migrate examples, tests, docs, generated bindings, and generated references in the same
   checkpoint commits as the API breaks.
4. Keep `datoviz.raw` exact to the exported ABI, but do not let raw `ctypes` expose known ownership
   traps such as owned `char*` returns bound as `c_char_p`.
5. Prefer backend-neutral stable app API names. GLFW-specific setup and conversion helpers belong
   in backend or interop surfaces unless deliberately classified as stable ABI.
6. Keep DRP2/vklite protocol and runtime escape hatches public only where useful, and classify them
   as advanced/unstable. Move fixture, JSON/base64 convenience, raw fallback, and development
   diagnostics out of the stable public surface where practical.
7. Use `DvzResult` for ordinary fallible Datoviz APIs, `bool` for predicates, raw integer returns
   for non-error status/count values, and Vulkan-native result codes only in Vulkan escape hatches.
8. Enforce one argument-ordering convention: object first; selector/config/range next; payload
   pointers before counts/sizes; byte-buffer APIs use `bytes, size_bytes`.
9. Make ownership and borrowing explicit with `const` resource arguments, copy-out descriptor/info
   APIs, and explicit free APIs or FFI policy for owned pointer returns.
10. Remove or prefix unprefixed public support macros. Prefer typedefs and inline/static helpers
    over public macros when a real type or helper is intended.


## Nested Descriptor Cleanup Audit

Maintainer concern: nested public descriptors such as `DvzScaleBarDesc` embedding
`DvzTextStyle`, `DvzTextPlacement`, and `DvzFormatDesc` are not ideal for a stable C/ABI/FFI
surface. They make designated initialization noisy, couple ABI layout across multiple public
records, complicate raw `ctypes`, and make defaulting ambiguous when both the outer and inner
records carry `struct_size` and `flags`.

Policy for the v0.4 cleanup:

1. Do not nest descriptor-like public structs by value inside other public descriptors.
2. Treat "descriptor-like" as any public struct with `uint32_t struct_size` and `uint32_t flags`,
   even when its name is `Style`, `Policy`, `Info`, or `Defaults` rather than `Desc`.
3. Small value records may remain nested by value: examples include `DvzColor`, `DvzRect`,
   `DvzPlacement`, `DvzCameraView`, `DvzCameraProjection`, `DvzPhongMaterial`,
   `DvzStandardMaterial`, and `DvzPolygonRing`.
4. Prefer minimal creation descriptors plus explicit setters for secondary style, layout,
   formatting, placement, tick-policy, or backend-specific configuration.
5. Pointer-to-descriptor fields are less severe than by-value nesting, but should still be reviewed
   for ownership, lifetime, backend leakage, and whether a setter or separate constructor argument
   is clearer.

Complete audit as of July 2026, using the criterion above:

| Owner | Nested field(s) | Location | Preferred disposition |
| --- | --- | --- | --- |
| `DvzAppConfig` | `DvzFontDefaults font_defaults` | `include/datoviz/app.h:133` | Done in `cb58878f6`: flattened to scalar `font_*` defaults fields. |
| `DvzFontDefaults` | `DvzFontDesc sans`, `DvzFontDesc mono` | `include/datoviz/font.h:42` | Done in `787982697`: flattened to prefixed scalar sans/mono font default fields. |
| `DvzViewDesc` | `DvzViewSizeDesc size` | `include/datoviz/app.h:225` | Done in `29b84d068`: flattened to scalar `size_*` fields while retaining `DvzViewSizeDesc` as the reusable policy value record. Duplicate legacy size scalar fields remain for a separate view-size API simplification decision. |
| `DvzViewDesc` | `const DvzWindowExternalSurfaceInfo* external_surface` | `include/datoviz/app.h:225`, `include/datoviz/window/backend.h:91` | Done in `a2d6863e5`: hosted surface creation uses `dvz_view_external_surface()` directly. |
| `DvzOverlayCardDesc` | `const DvzOverlayCardStyle* style` | `include/datoviz/scene/overlay.h:67` | Done in `ebd35bb00`: creation no longer carries style; use `dvz_overlay_card_set_style()` after creation. |
| `DvzPanelView3DDesc` | `DvzCameraDesc camera` | `include/datoviz/scene/types.h:704` | Done in `8642a2efd`: flattened to `DvzCameraView view` and `DvzCameraProjection projection`. |
| `DvzPanelAxes2DDesc` | `DvzAxisTickPolicy tick_policy`, `DvzAxisStyle x_style`, `DvzAxisStyle y_style` | `include/datoviz/scene/types.h:785` | Done in `977881068`: creation desc is labels/coarse flags only; tick policy and styles use setters. |
| `DvzGraphEdgeStyle` | `DvzBezierTessellationDesc tessellation` | `include/datoviz/scene/types.h:946` | Done in `26adba228`: flattened to scalar tessellation segment count and tolerance fields. |
| `DvzSelectionVisualStyle` | `DvzItemStateVisualStyle selected`, `DvzItemStateVisualStyle unselected` | `include/datoviz/scene/types.h:1253` | Done in `4afdfb53d`: flattened to prefixed selected/unselected scalar style fields. |
| `DvzScaleDesc` | `DvzFormatDesc format` | `include/datoviz/scene/types.h:1380` | Done in `256b4e018`: creation covers `kind`, `label`, and `unit`; format uses `dvz_scale_set_format()`. |
| `DvzAnnotationDesc` | `DvzTextStyle style`, `DvzTextPlacement placement` | `include/datoviz/scene/types.h:1582` | Done in `343f32154`: creation is minimal; style and placement use retained setters. |
| `DvzLabelDesc` | `DvzTextStyle style`, `DvzTextPlacement placement` | `include/datoviz/scene/types.h:1595` | Done in `343f32154`: creation is minimal; style and placement use retained setters. |
| `DvzScaleBarDesc` | `DvzTextStyle label_style`, `DvzTextPlacement placement`, `DvzFormatDesc format` | `include/datoviz/scene/types.h:1607` | Done in `256b4e018`: creation desc keeps identity, units, geometry, length bounds, offset, line/tick styling; label style, placement, and format use setters. |

Suggested execution order:

1. Nested descriptor cleanup is complete under the July 2026 audit criterion.
2. Clean any remaining app/view/backend API naming or duplication during the backend-neutral app
   API wave.
3. Regenerate raw `ctypes`, generated C docs, and examples in the same checkpoint as each public
   struct break.


## Non-Negotiable Pickup Rules

1. Read `AGENTS.md`, `agents/now/START.md`, `agents/now/STATUS.md`, and this handoff first.
2. Before public header, exported API, raw-binding policy, or generator changes, read
   `spec/bindings/ARRAY_FACADE.md` and `spec/bindings/CTYPES_POLICY.md`.
3. After changing public headers, exported API, binding policy, or binding generators, refresh and
   validate bindings with:

   ```sh
   just ctypes
   just ctypes-check
   ```

4. Always run `git diff --check` before finalizing.
5. Before committing, run:

   ```sh
   git status --short
   git diff --cached --stat
   ```

   Verify the staged set excludes `data`, generated/runtime binaries, vendored runtime libraries,
   and unrelated user changes.


## Highest Priority Findings

### 1. Remove Internal Object/Container API From Public Surface

Status: completed by `c3caf968d` except for deferred dynamic export-list tightening.

`DvzObject`, `DvzContainer`, object status/type enums, and generic container functions are installed,
exported, documented, and emitted in raw `ctypes`, but look like internal lifetime/runtime plumbing.

References:

- `include/datoviz/common/obj.h`
- `datoviz/_ctypes.py`

Preferred fix:

- Move object/container declarations to private/internal headers, or remove `DVZ_EXPORT`.
- If any pieces must remain installed, classify them explicitly as advanced/unstable and exclude
  them from ordinary C docs/raw public examples.


### 2. Remove Legacy And Transitional Scene Symbols

The public scene surface still exposes aliases and wrappers that are already documented as old or
transitional.

References:

- `include/datoviz/scene/animation.h`: `DVZ_CLOCK_REALTIME`, `DVZ_CLOCK_OFFLINE`

Preferred fix:

- Remove legacy clock aliases; keep only `DVZ_SCENE_CLOCK_REALTIME`,
  `DVZ_SCENE_CLOCK_FIXED_STEP`, and `DVZ_SCENE_CLOCK_EXTERNAL`.
- Collapse the 2D view API onto `DvzPanelView2DDesc` and the descriptor-taking
  `dvz_panel_set_view2d(DvzPanel*, const DvzPanelView2DDesc*)`.

Status:

- Clock alias cleanup completed by `a703f01e1`.
- Texture wrapper cleanup completed by `93c706d6a`.
- 2D view descriptor cleanup completed by `0453620fd`.


### 3. Fix Raw `ctypes` Ownership Traps

Status: completed for identified owned-return traps by `6ab5814aa` and `c437357b3`.

Owned pointer returns and fixed-array pointer types currently generate unsafe or awkward raw ctypes.

References:

- `include/datoviz/fileio/fileio.h`: `dvz_read_gz()` returns owned `char*`
- `include/datoviz/math/mock.h`: `dvz_mock_*()` helpers return owned arrays such as `vec3*`
- `datoviz/_ctypes.py`: owned `char*` is bound as `c_char_p`; `vec3*` returns are awkward

Preferred fix:

- Bind owned byte/string returns as `c_void_p` or explicit pointer types, not `c_char_p`.
- Add or update owned-return binding policy entries.
- Demote mock-data helpers from the public API, or provide explicit FFI helpers/policy for owned
  array returns.
- If mock helpers remain public, rename uppercase dimension suffixes to lowercase:
  `dvz_mock_pos_2d()`, `dvz_mock_pos_3d()`.


### 4. Make Canvas Flags Atomic Bit Flags

Status: completed by `2eacad217`.

`DVZ_CANVAS_FLAGS_FPS = 0x0003` and `DVZ_CANVAS_FLAGS_MONITOR = 0x0005` implicitly include
`DVZ_CANVAS_FLAGS_IMGUI`.

Reference:

- `include/datoviz/canvas/enums.h`

Preferred fix:

- Use one bit per flag, for example `IMGUI = 0x0001`, `FPS = 0x0002`, `MONITOR = 0x0004`.
- Add explicit named combinations only if needed.


### 5. Decide Whether GLFW Is Stable API Or Backend Detail

Status: stable app-facing view names renamed by `3a17d5b14`; remaining GLFW conversion helpers and
numeric compatibility comments still need a separate backend/interop classification decision.

The default public API exposes GLFW-specific names and compatibility assumptions.

References:

- `include/datoviz/app.h`: `DVZ_VIEW_GLFW`, `dvz_view_glfw()`, GLFW extension config
- `include/datoviz/input/pointer.h`: `dvz_pointer_button_from_glfw()`
- `include/datoviz/input/enums.h` and `include/datoviz/input/keycodes.h`: numeric compatibility
  comments

Preferred fix:

- Rename stable app-facing concepts to backend-neutral names, such as `DVZ_VIEW_WINDOW` and
  `dvz_view_window()`.
- Move GLFW conversion helpers and GLFW-specific setup to backend or interop headers.
- If numeric GLFW compatibility is intentional ABI, document it as such.


## Consistency Fixes Worth Doing Before RC

### 6. Standardize Result Types

Public fallible APIs mix `DvzResult`, `bool`, raw `int`, `int32_t`, and pointer-or-NULL.

References:

- `include/datoviz/common/types.h`: `DvzResult`
- `include/datoviz/app.h`: mostly `DvzResult`
- `include/datoviz/canvas.h`, `include/datoviz/stream.h`, `include/datoviz/window/backend.h`
- `include/datoviz/vklite/commands.h`, `include/datoviz/vklite/buffers.h`,
  `include/datoviz/vklite/sync.h`

Preferred fix:

- Use `DvzResult` for ordinary Datoviz success/error APIs.
- Keep raw `int` only where non-error status values are part of the contract.
- Keep Vulkan-native result codes only in explicitly Vulkan-native escape hatches.


### 7. Pick One Array/Count Argument Convention

Adjacent APIs mix `data, count` and `count, data`.

References:

- `include/datoviz/scene.h`: `dvz_visual_set_data()`, `dvz_visual_set_data_range()`
- `include/datoviz/scene/plot.h`: bars and band setters
- `include/datoviz/scene/text.h`: text batch setters

Preferred convention:

```text
object, selector, range-if-any, data pointer(s), count/size
```

Apply this especially to `dvz_visual_set_data_range()` and text batch setters.


### 8. Tighten Constness And Borrowed Ownership

Bind-only APIs accept mutable resources, and descriptor getters expose borrowed internals.

References:

- `include/datoviz/scene/field.h`: `dvz_visual_set_field()`,
  `dvz_sampled_field_get_desc()`
- `include/datoviz/scene/scale.h`: `dvz_visual_set_scale()`
- `include/datoviz/scene.h`: symbol-set bindings and `dvz_scene_buffer_get_desc()`
- `include/datoviz/scene/text.h`: `dvz_text_atlas_field()`

Preferred fix:

- Use `const DvzSampledField*`, `const DvzScale*`, and `const DvzSymbolSet*` for bind-only
  resource arguments when ownership is not transferred.
- Replace borrowed descriptor getters with copy-out APIs such as
  `bool dvz_sampled_field_info(const DvzSampledField*, DvzSampledFieldDesc* out)` and
  `bool dvz_scene_buffer_info(const DvzSceneBuffer*, DvzSceneBufferDesc* out)`.


### 9. Clean Or Classify Advanced DRP2

The DRP2 public headers mix protocol builders, fixture/JSON plumbing, base64 payloads, byte
payloads, and development replay fallback.

References:

- `include/datoviz/drp2/recording.h`: `DvzDrp2RawFallback`
- `include/datoviz/drp2/stream.h`: base64 and byte write APIs, JSON helpers
- `include/datoviz/drp2/stream.h`: stringly typed shader stage/format, index format, barriers

Preferred fix:

- Remove raw fallback from RC public API, or move it to private/test diagnostics.
- Make byte-oriented APIs primary.
- Rename base64 forms with `_base64` or move them to fixture/JSON headers excluded from stable docs.
- Replace stringly typed command arguments with enums where feasible; otherwise classify these
  commands as advanced/unstable.


### 10. Clean Support-Header Leakage

Status: `dvz_load_jpeg()` byte-buffer argument order normalized by `52e38e358`, `DvzAlpha`
replaced with a typedef by `2a340b51c`, and unprefixed math support macros plus the
`_PRETTY_SIZE` buffer cleaned by `2ac43481c`; public `dvz_box_*` declarations without export were
resolved by `bff88625d`; `dvz_read_ppm()` dimensions normalized by `4fb4be416`; embedded resource
sizes and stale texture/testdata accessors cleaned by `dc1617d8f`. Other support-header leakage
remains open.

Public support headers leak unprefixed macros, mutable TU-local buffers, test resources, and
inconsistent byte-buffer signatures.

References:

- `include/datoviz/math/types.h`: review remaining public constants/helpers such as `M_PI` and
  `dvz_pretty_size()` for intended stable support-header scope
- `include/datoviz/math/box.h`: resolved by `bff88625d`; the public helper set is now exported and
  generated in raw `ctypes`/C docs
- `include/datoviz/fileio/fileio.h`: `dvz_load_png(bytes, size)` versus
  `dvz_load_jpeg(size, bytes)` resolved by `52e38e358`; resource size outputs and stale
  texture/testdata declarations resolved by `dc1617d8f`

Preferred fix:

- Prefix public macros (`DVZ_MIN`, `DVZ_MAX`, etc.) or remove them from public headers.
- Replace `#define DvzAlpha uint8_t` with `typedef uint8_t DvzAlpha`.
- Keep only reentrant `dvz_pretty_size_r()` or make `dvz_pretty_size()` thread-safe.
- Export and document the full `dvz_box_*` group or make non-exported declarations private.
- Normalize byte-buffer APIs to `bytes, size_bytes`, use unsigned dimensions, use `DvzSize*` or
  `size_t*` consistently, make borrowed resources `const`, and demote `dvz_resource_testdata()`.


## Lower-Priority Polish

- Rename `dvz_scalebar_*` to `dvz_scale_bar_*` for consistency with `DvzScaleBar`.
- Normalize graph and polygon mutators to `*_set_*`, for example `dvz_graph_set_edges()` and
  `dvz_polygon_set_fill_color()`.
- Document public text batch setters or remove them in favor of `dvz_text_set_items()` plus
  single-item helpers.
- Decide whether geometry factories should be `dvz_geom_*` or `dvz_geometry_*`, then keep one
  public naming family.
- Split stable `window.h` from backend SPI if window APIs are meant to be user-facing; currently
  `window.h` includes backend registration and wrap-surface internals.
- Review `advanced.h` and `vk.h` umbrella consistency. Either make `vk.h` the full low-level Vulkan
  umbrella or split/rename it so omissions are intentional.


## Suggested Execution Plan

Use small checkpoint commits. Each commit should keep public headers, implementation, tests,
generated ctypes, generated docs if applicable, and examples in sync.

1. **API exposure cleanup**
   - Completed: remove or demote internal object/container API.
   - Completed: remove legacy clock aliases.
   - Completed: remove or demote mock-data helpers.
   - Next: remove transitional texture wrappers, or rename them as explicit sampled field helpers.
   - Validation: `just ctypes`, `just ctypes-check`, narrow compile/build check.

2. **Stable scene naming cleanup**
   - Collapse 2D view APIs.
   - Normalize scale-bar spelling.
   - Normalize graph/polygon mutator names.
   - Completed: change `dvz_sampled_field_destroy()` to `void`.
   - Validation: scene tests, examples that use affected APIs, `just ctypes-check`.

3. **Signature and ownership cleanup**
   - Standardize array/count order.
   - Tighten `const` on bind-only resources and read-only queries.
   - Replace borrowed descriptor getters with copy-out APIs.
   - Validation: affected C tests, raw ctypes regeneration/check, Python smoke if bindings changed.

4. **Runtime/app consistency cleanup**
   - Fix canvas flags.
   - Decide GLFW-neutral app names.
   - Standardize `DvzResult` versus raw `int`.
   - Split stable window API from backend SPI if scope permits.
   - Validation: app/canvas/window/input tests and hosted/offscreen examples where available.

5. **Advanced DRP2/vklite classification cleanup**
   - Remove DRP2 raw fallback from stable surface or clearly move it to diagnostics.
   - Rename or move base64/JSON fixture APIs.
   - Align draw argument ordering or classify one layer as protocol-specific.
   - Add enums for stringly typed command arguments where feasible.
   - Validation: DRP2 stream tests, frame-plan emission tests, WebGPU/WASM fixture tests if command
     schema changes.

6. **Support-header cleanup**
   - Prefix/remove public macros.
   - Normalize file I/O buffer signatures.
   - Fix `dvz_box_*` export/declaration mismatch.
   - Validation: fileio/math/geom tests, raw ctypes regeneration/check.


## Execution Log

### Baseline And API Inventory

Status: complete on branch `api/pre-rc-cleanup`.

Baseline commands:

```sh
git status --short --branch
git submodule status data
git diff --check
just build
python3 tools/check_api_status.py
python3 tools/build_api_c.py --check
just spec-check
```

Results:

1. `git status --short --branch` reported branch
   `api/pre-rc-cleanup...origin/api/pre-rc-cleanup`, unstaged `data`, and untracked
   `paper/paper.pdf`.
2. `git submodule status data` reported
   `c1506b18196e509a9d50f26faba41d6838620aa8 data (remotes/origin/v0.4-dev)`.
3. `git diff --check` passed.
4. `just build` passed with no rebuild work.
5. `python3 tools/build_api_c.py --check` passed with 1557 functions and 7 pages classified.
6. `python3 tools/check_api_status.py` failed on an existing metadata gap:
   `include/datoviz/video/types.h` is installed but unclassified in `spec/api/status.yml`.
7. `just spec-check` failed for the same API status gap and for the existing scene visual-boundary
   guard findings from `tools/check_scene_visual_boundaries.py`.

Inventory:

1. Installed public headers under `include/datoviz/`: 112.
2. `DVZ_EXPORT` occurrences in installed public headers: 1585.
3. `spec/api/status.yml`: 24 entries covering 57 headers; tiers are 9 stable, 14 advanced, and
   1 experimental.
4. Generated raw `datoviz/_ctypes.py`: 1568 bound functions and 189 generated structure/enum
   classes.

Next wave target: public exposure cleanup. First fix or deliberately classify the baseline
`video/types.h` manifest gap, then remove or demote accidental/internal public surfaces such as
object/container declarations, legacy scene aliases, and mock-data helpers.

### Public Exposure Cleanup: Object Container Slice

Status: complete.

Changes:

1. Classified `include/datoviz/video/types.h` in `spec/api/status.yml`.
2. Moved `include/datoviz/common/obj.h` to private `src/common/obj.h`.
3. Removed object/container lifecycle declarations from installed headers, generated C reference,
   and generated raw `ctypes`.
4. Removed unnecessary public `common/obj.h` includes from `vklite/commands.h` and
   `vklite/sampler.h`; internal users now include the private header.

Header/API delta:

1. Installed public headers: 112 -> 111.
2. `DVZ_EXPORT` occurrences in installed public headers: 1585 -> 1573.
3. Extracted public API: 1564 functions, 303 records, 187 enums, with no `dvz_obj_*`,
   `dvz_container_*`, `DvzObject`, or `DvzContainer` entries.

Validation:

```sh
python3 tools/check_api_status.py
just build
just ctypes
just ctypes-check
just docs-api
just docs-api-check
just test common
just ctypes-smoke
git diff --check
```

Result: all passed.

`just spec-check` now passes API status, DRP2 metadata, WASM metadata, fixture tests, WebGPU
preflight, scheduler tests, scene query source guard, and scene architecture source guard. It still
fails on the pre-existing scene visual-boundary guard findings recorded in the baseline.

Dynamic-symbol note: the macOS split-library build still needs the object lifecycle functions
visible from `libdatoviz_core.dylib` for `libdatoviz_vk.dylib` and vklite code. A true dynamic
export-list cleanup should be handled as a separate linkage/packaging wave; this slice removes the
object/container surface from installed headers, generated docs, raw `ctypes`, and public API
extraction.


## Validation Baseline For Future Agent

Minimum after any public API change:

```sh
just ctypes
just ctypes-check
git diff --check
```

Recommended broader checks by touched surface:

```sh
just build
just test scene
just test app
just test drp2
just spec-check
```

Before each commit:

```sh
git status --short
git diff --cached --stat
```

Do not stage or commit `data` submodule updates, generated/runtime binary payloads, vendored
runtime libraries, or unrelated user changes without explicit approval in the current turn.
