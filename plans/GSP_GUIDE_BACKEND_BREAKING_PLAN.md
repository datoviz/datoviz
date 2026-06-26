# GSP Guide Backend Breaking Implementation Plan

## Purpose

Use the remaining Datoviz v0.4-dev API/ABI break window to make Datoviz a strict, mechanical
backend target for GSP guide, View2D, explicit tick, colorbar, text, mesh, and query contracts.

This plan supersedes `plans/GSP_GUIDE_BACKEND_PLAN.md` for implementation sequencing. The older
plan remains useful context, but this document explicitly authorizes breaking C API, ABI, generated
`ctypes`, and the top-level Python facade when that produces a cleaner v0.4 contract.


## Breaking-Change Policy

1. Prefer clean v0.4 semantics over compatibility with current ambiguous behavior.
2. It is acceptable to break C struct layouts, enum values, function signatures, generated
   `datoviz.raw`, and top-level `import datoviz as dvz` facade details.
3. Do not preserve sorted-domain, implicit-auto-tick, query-default, or binding-generator behavior
   if it conflicts with the GSP backend contract.
4. Keep the normal v0.4 guardrails:
   - no Matplotlib, GSP, or VisPy2 implementation inside Datoviz;
   - no high-level plotting aliases such as `plot()`, `imshow()`, `scatter()`, `Figure`, or
     prefixless wrappers;
   - no parallel renderer, frame-stream, presentation path, or Vulkan wrapper;
   - no guide picking for RC unless the maintainer explicitly broadens scope;
   - no nonlinear axes, categorical axes, geospatial axes, equal-aspect layout solving, label
     collision solving, live navigation event semantics, or full query parity unless explicitly
     approved.


## Target GSP Contract

Datoviz must expose these behaviors through public C symbols and the generated top-level Python
facade:

1. A panel View2D/domain API where axes, grid lines, colorbars, and data visuals share the same
   visible domain state.
2. Ordered finite domains, including reversed domains such as `x=(10, 0)` and `y=(1, -1)`.
3. Data-to-visual mapping, rendered axis marks, rendered grid lines, and public visible-domain
   readback derived from one visible-domain snapshot.
4. Exact explicit axis tick positions and optional exact tick labels supplied by upstream GSP.
5. Grid lines aligned with the same tick values rendered as axis ticks.
6. Axis labels and panel text/title behavior stable enough for GSP guides.
7. Complete data query payload fields for supported data picking/probing.
8. Structured unsupported results for guide query and all-rendered query scopes when guide picking
   is absent.
9. Colorbar, text, and mesh support documented and exposed through Python facade calls.


## Phase 0 - Baseline Inventory

Before editing implementation code:

1. Run `git status --short --branch`.
2. Treat staged `data` gitlink updates as stop signs. Do not touch, stage, or commit `data` unless
   the maintainer explicitly approves it in the current turn.
3. Inspect public API and ABI surfaces:
   - `include/datoviz/scene.h`
   - `include/datoviz/scene/types.h`
   - `include/datoviz/scene/enums.h`
   - `include/datoviz/scene/interaction.h`
   - `include/datoviz/scene/scale.h`
   - `include/datoviz/scene/text.h`
4. Inspect implementation surfaces:
   - `src/scene/core/`
   - `src/scene/annotation/`
   - `src/scene/interaction/`
   - `src/scene/query/`
   - `src/scene/visuals/*/query.c`
   - `datoviz/_array_facade.py`
   - `spec/bindings/ctypes.yml`
5. Identify the narrowest existing tests for axes, query, colorbar, text, mesh, and Python binding
   exposure. Prefer extending those over broad new harnesses.

Exit criteria:

- The first implementation commit or PR body records which APIs already existed, which were
  changed incompatibly, and which were added.


## Phase 1 - Ordered Domain and View2D Contract

Goal: make ordered domains first-class instead of treating reversed domains as accidental input.

Public contract:

1. `DvzDataDomain { min, max }` stores ordered endpoints, not sorted bounds.
2. Reversed finite domains are legal.
3. `dvz_panel_visible_domain(panel, dim, out_a, out_b)` returns ordered visible endpoints.
4. For `x=(10, 0)`, data `10` maps to the left visual endpoint and data `0` maps to the right
   visual endpoint.
5. For `y=(1, -1)`, document and test the visual-coordinate convention explicitly. Do not leave
   the orientation implicit in assertions.
6. Internal helpers that need numeric low/high bounds must use clearly named sorted-bound helpers,
   for example `_domain_sorted_bounds()` or `_axis_visible_sorted_interval()`.
7. Auto tick generation may compute over sorted numeric bounds, but public readback and rendering
   transforms must preserve the ordered domain orientation.

Implementation guidance:

1. Refactor `_axis_visible_domain()` and related panel helpers so public readback is oriented.
2. Add a separate internal sorted interval for auto ticks and density/cache decisions.
3. Ensure panzoom/controller extent does not cause axes and data mapping to diverge.
4. Ensure `dvz_panel_data_to_visual_positions()` and axis/grid emission use the same resolved
   View2D snapshot.
5. Update docs only where behavior was ambiguous or changed.

Tests:

1. Increasing domain: `x=(0, 10)`, `y=(-1, 1)`.
2. Reversed domain: `x=(10, 0)`, `y=(1, -1)`.
3. Public visible-domain readback preserves endpoint order.
4. Data normalization and rendered axis/grid positions agree.
5. Panzoom-visible extent, if bound, preserves agreement between readback and mapping.

Acceptance criteria:

- Reversed finite domains are legal and deterministic.
- Public visible-domain readback preserves endpoint order.
- Auto ticks remain stable because sorted interval logic is explicit and internal.
- Data visuals, axes, and grid lines use one coherent visible-domain model.


## Phase 2 - Axis Tick Model Refactor

Goal: make auto ticks and explicit GSP-supplied ticks separate, understandable sources.

Preferred public API:

```c
typedef struct DvzAxisTicks
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t count;
    const double* values;
    const char* const* labels; /* optional; NULL means format values */
} DvzAxisTicks;

DVZ_EXPORT bool dvz_axis_set_ticks(DvzAxis* axis, const DvzAxisTicks* ticks);
DVZ_EXPORT bool dvz_axis_clear_ticks(DvzAxis* axis);
```

The final shape may change if a better v0.4 type-system fit emerges, but it must preserve these
semantics:

1. Explicit tick values are in panel data coordinates.
2. Explicit tick order is preserved exactly as supplied.
3. Explicit ticks are not sorted for rendering, grid emission, or labels.
4. Labels are copied before the setter returns.
5. `labels == NULL` means format numeric values through the current units/datetime/format policy.
6. `count == 0` is valid and renders no ticks, labels, or grid lines for that axis.
7. Counts above fixed engine capacity are rejected with no partial mutation.
8. Explicit tick state overrides auto tick policy until cleared.
9. `dvz_axis_clear_ticks()` returns to auto tick generation and marks the axis dirty.

Internal model:

1. Split axis state into:
   - auto tick policy;
   - explicit tick input snapshot;
   - computed render tick snapshot;
   - copied render label snapshot.
2. Avoid overloading cache fields such as `tick_lmin`, `tick_lmax`, and `tick_lstep` to mean both
   auto tick cache and explicit input.
3. Keep grid, tick marks, and tick labels driven by the same render tick snapshot.
4. If labels are explicitly supplied, label layout must use the copied labels, not a regenerated
   numeric formatter.

Tests:

1. Explicit values `[0, 5, 10]`.
2. Explicit reversed values `[10, 5, 0]`.
3. Explicit labels `["ten", "five", "zero"]`.
4. Empty explicit ticks render no ticks/grid.
5. Clearing explicit ticks returns to auto policy.
6. Caller-owned label storage can be freed or mutated after the setter without corrupting rendered
   labels.
7. Over-capacity explicit ticks fail without partial mutation.

Acceptance criteria:

- GSP can set exact tick positions and labels.
- Reversed explicit ticks render in requested order and align with reversed domains.
- Grid lines align with explicit ticks.
- Auto tick policy remains available and distinct from explicit tick state.


## Phase 3 - Python Facade and Binding Regeneration

Goal: expose the GSP-facing C surface through `import datoviz as dvz` without private modules or raw
`ctypes` boilerplate for required GSP probes.

Breaking binding policy:

1. Update the public C API first.
2. Regenerate or adapt `datoviz.raw` and the top-level facade after the C contract is clean.
3. It is acceptable to change generated structure layouts and skipped-function lists.
4. Do not preserve old Python quirks that conflict with the v0.4 C-shaped `dvz_*` facade.

Required top-level symbols:

- `dvz_panel_set_domain`
- `dvz_panel_set_view2d`
- `dvz_panel_visible_domain`
- `dvz_panel_data_to_visual_positions`
- `dvz_panel_axis`
- `dvz_axis_set_grid`
- `dvz_axis_set_label`
- `dvz_axis_set_tick_policy`
- `dvz_axis_set_ticks`
- `dvz_axis_clear_ticks`
- `dvz_colorbar`
- `dvz_colorbar_set_title`
- `dvz_colorbar_set_format`
- `dvz_text`
- `dvz_text_set_string`
- `dvz_text_set_style`
- `dvz_text_set_placement`
- `dvz_mesh`
- query APIs and `DvzQueryResult`

Explicit tick facade policy:

1. Numeric tick values should be accepted as a NumPy array or compatible buffer when feasible.
2. String labels must either be deliberately supported by the generator/facade or explicitly
   documented as raw/advanced for the current packet.
3. Do not half-support labels: either top-level Python labels work and are tested, or the limitation
   is explicit in the GSP matrix.

Tests:

1. Python smoke test imports `datoviz as dvz`.
2. `hasattr()` covers all required probe symbols.
3. Numeric explicit tick values work through the top-level facade if implemented.
4. Label behavior is tested if supported through the top-level facade.

Acceptance criteria:

- GSP can mechanically probe support with `hasattr(dvz, "...")`.
- Required direct-engine calls use top-level `datoviz` unless a limitation is explicitly marked
  advanced/raw.
- The facade remains C-shaped and does not become a plotting API.


## Phase 4 - Query Contract and Payload Matrix

Goal: make supported data queries complete and unsupported guide/all-rendered queries explicit.

If the current request model cannot express guide or all-rendered scopes cleanly, break it now.
Prefer a clear v0.4 request/target/scope model over documenting around an awkward gap.

Payload matrix to fill before implementation:

| Query scope | Required status | Required fields on hit | Unsupported behavior |
| --- | --- | --- | --- |
| point item | supported if visual capability enabled | request id, status, hit, panel id, panel position, framebuffer position, visual id, visual family, item id, optional link key, displayed RGBA when available, data position when available | explicit unsupported or no-capable-visual status |
| marker item | supported if visual capability enabled | same as point item | explicit unsupported or no-capable-visual status |
| image pixel/sample | supported for promoted image/sample paths | visual id, family, texel/sample id or coordinate, displayed RGBA, scalar/vector value when represented, uvw when available | explicit unsupported status |
| sampled field | supported for promoted sample profile paths | visual id, family, sample value, displayed RGBA when available, uvw/data position when available, scale when valid | explicit unsupported status |
| mesh item/face | support only what runtime can prove | visual id, family, item/face/primitive id as applicable, data/visual position when available | deferred cases return unsupported |
| text/labels item | support only what runtime can prove | visual id, family, item or label id, text value when available | deferred cases return unsupported |
| guide query | deferred for RC | none | explicit unsupported target/scope status |
| all-rendered including guides | deferred unless guide picking is implemented | none | explicit unsupported target/scope status, not data-only degradation |

Implementation steps:

1. Inspect `DvzQueryRequest`, `DvzQueryResult`, target enums, profile enums, and family query ops.
2. Update enums/records if guide/all-rendered scopes need a cleaner public shape.
3. For supported data families, ensure live runtime query results fill required identity,
   coordinate, value, and displayed-color fields when available.
4. If a field cannot be produced for a supported target, set an explicit unsupported status or
   document the field as unavailable in the matrix. Do not leave zero/default fields that look
   valid.
5. Add deterministic `dvz_panel_query_now()` tests where possible.
6. Add one runtime smoke only if the touched subsystem already has a narrow pattern for it.

Acceptance criteria:

- GSP can build a semantic result for supported data picking/probing.
- Unsupported guide and all-rendered query requests are distinguishable from data misses.
- Guide query is still deferred unless explicitly approved.


## Phase 5 - Colorbar Contract Proof

Goal: verify and expose the colorbar behavior GSP needs without broadening into a plotting layer.

Required proof:

1. Vertical and horizontal colorbars render.
2. Title is stable.
3. Numeric range comes from the bound color scale.
4. Tick labels follow the scale/format policy.
5. Layout does not unexpectedly overlap the plot area.
6. Python facade exposes creation, title, orientation, layout/anchor, and format APIs needed by GSP.
7. Colorbar query/readback is either supported and tested or explicitly deferred.
8. Explicit colorbar ticks are either implemented deliberately or documented as auto/formatted-only
   for RC.

Acceptance criteria:

- GSP can render colorbars through Datoviz or receive a precise limitation.
- Colorbar rendering does not require private modules.


## Phase 6 - Text Contract Proof

Goal: stop downstream adapters from treating text as symbol-present but semantically unknown.

Document and test:

1. Placement coordinate system.
2. Anchor behavior.
3. Font size units.
4. Rotation angle convention.
5. Color format.
6. Renderer fallback rules.
7. String lifetime/copy behavior.
8. Python facade exposure for `dvz_text()`, `dvz_text_set_string()`,
   `dvz_text_set_style()`, `dvz_text_set_placement()`, and relevant records/enums.
9. Text query behavior: supported fields if implemented, otherwise explicit unsupported status.

Acceptance criteria:

- GSP can classify Datoviz text support by documented semantics, not just symbol presence.


## Phase 7 - Mesh Contract Proof

Goal: stop downstream adapters from treating mesh as symbol-present but semantically unknown.

Document and test:

1. Indexed triangle topology.
2. Flat uniform RGBA behavior.
3. Per-vertex and per-face color behavior if supported.
4. 2D `z=0` usage for plotting overlays.
5. Depth and culling defaults.
6. Canonical mesh upload path.
7. Python facade exposure for `dvz_mesh()` and required upload helpers.
8. Mesh query behavior: face/item support if implemented, otherwise explicit unsupported status.

Acceptance criteria:

- GSP can classify mesh support according to documented semantics and tested behavior.


## Phase 8 - Documentation and Compatibility Matrix

Goal: make Datoviz support status mechanically consumable by GSP, release notes, and future agents.

Update:

1. `spec/api/GSP_BACKEND_READINESS.md` or a sibling spec with a GSP compatibility matrix.
2. `docs/reference/queries.md` with payload fields and guide-query deferral.
3. `docs/how-to/axes.md` with ordered/reversed domains and explicit tick behavior.
4. `docs/reference/feature-status.md` if status changes.
5. `spec/bindings/ARRAY_FACADE.md` if facade policy changes materially.

Required compatibility rows:

| Capability | Status | Public C API | Top-level Python | Tests/proof | Notes |
| --- | --- | --- | --- | --- | --- |
| View2D/domain readback | TBD | TBD | TBD | TBD | ordered endpoints |
| reversed finite domains | TBD | TBD | TBD | TBD | no silent sorting in public readback |
| explicit axis ticks | TBD | TBD | TBD | TBD | exact order |
| explicit tick labels | TBD | TBD | TBD | TBD | copied labels |
| grid alignment | TBD | TBD | TBD | TBD | same tick snapshot |
| axis labels | TBD | TBD | TBD | TBD | text pipeline |
| guide query | deferred | TBD | TBD | TBD | explicit unsupported |
| all-rendered with guides | deferred | TBD | TBD | TBD | no data-only fallback |
| data query payload completeness | TBD | TBD | TBD | TBD | matrix-backed |
| colorbar render | TBD | TBD | TBD | TBD | range/title/format |
| colorbar query | deferred or supported | TBD | TBD | TBD | no CPU fallback |
| text render | TBD | TBD | TBD | TBD | placement/style semantics |
| text query | deferred or supported | TBD | TBD | TBD | explicit status |
| mesh render | TBD | TBD | TBD | TBD | topology/upload semantics |
| mesh query | deferred or supported | TBD | TBD | TBD | explicit status |

Acceptance criteria:

- A downstream GSP agent can read one status section and know exactly what Datoviz supports.
- Deferred items are intentional RC boundaries, not accidental omissions.


## Suggested Commit Boundaries

Commit by contract, not by incidental file grouping:

1. `scene: make View2D domains ordered endpoints`
2. `axes: refactor tick sources and explicit labels`
3. `python: expose GSP axis and guide-facing symbols`
4. `query: define GSP payload and unsupported guide scopes`
5. `scene: prove colorbar text mesh GSP contracts`
6. `docs: record GSP backend compatibility matrix`

Split further if a commit crosses too many files or validation takes too long.


## Validation

Use the narrowest relevant loop while iterating. Before finalizing any code or documentation change,
run:

```sh
git diff --check
```

For scene/runtime code:

```sh
just build
just test scene
just spec-check
```

For Python binding/facade changes:

```sh
python3 tools/bindings/validate_ctypes_policy.py
python3 tools/bindings/validate_array_facade.py
python3 tools/bindings/ctypes_smoke.py
python3 tools/bindings/array_facade_smoke.py
```

For runtime query or offscreen rendering changes, also run the narrow existing runtime/offscreen
smoke used by the touched subsystem. Record skipped graphics tests honestly if the local
environment lacks the required Vulkan, GLFW, or headless support.


## Stop Conditions

Stop and ask the maintainer before:

1. adding a high-level plotting API;
2. implementing guide picking instead of explicit deferral;
3. broadening into nonlinear axes, categorical axes, geospatial axes, layout collision solving, or
   Matplotlib compatibility;
4. touching, staging, or committing the `data` submodule;
5. staging generated/runtime binary payloads or unrelated user changes;
6. creating a parallel runtime, renderer, frame-stream, presentation, or Vulkan-wrapper path.
