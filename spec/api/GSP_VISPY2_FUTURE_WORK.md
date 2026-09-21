# GSP and VisPy2 Future Work

Status: informative post-RC3 roadmap. This document records intended integration work; it is not an RC3 release gate and does not authorize implementation by itself.

## Current boundary

The qualified GSP and VisPy2 path already uses public Datoviz v0.4 APIs for retained visuals, View2D and View3D state, item queries, direct RGBA capture, offscreen rendering, and live navigation. Future work must preserve the Datoviz ownership boundary: GSP owns backend-neutral semantics, VisPy2 owns producer conveniences, and Datoviz owns retained rendering and native interaction primitives.

The following items are intentionally deferred:

| Item | Datoviz status | Release position |
| --- | --- | --- |
| Panel title | Generic annotations can render text, but there is no retained title role with automatic reserve, guide snapshot contribution, and guide hit behavior. | Post-RC3; not an RC3 gate. Promotion into RC4 requires an explicit maintainer decision. |
| Mesh-face query | Public `DVZ_SCENE_TARGET_FACE`, face identity, primitive identity, capability flags, bindings, and focused native tests exist for a single eligible mesh visual. | Datoviz fails closed when more than one eligible visual could answer FACE/TRIANGLE: the current per-visual query executor has no cross-visual depth comparison. A combined depth-aware query is required before scene-frontmost face identity can be advertised. |
| Image sample query | Public `DVZ_SCENE_TARGET_SAMPLE` and `DVZ_SCENE_TARGET_PIXEL` can return sampled/display RGBA. | Supported only as sampled color/value until the identity contract below is implemented. |
| Exact image texel identity | `DvzQueryResult` has `texel_id` and UVW fields, but the image query path does not currently populate a canonical texel coordinate or UVW. | Post-RC3 design and implementation. Do not infer texel identity from sampled RGBA. |
| Multi-panel GSP execution | The plural-view GSP schema and both reference adapters now execute multiple panels. The Datoviz adapter retains all panels in one native scene/figure and routes panel-local views, visuals, guides, queries, snapshots, and navigation through existing public APIs. | Downstream implementation complete at GSP `aa6bcb4`; no missing Datoviz engine primitive was identified. A local exact-wheel smoke proved mixed View2D/View3D native panels, partial snapshots, and capture. Keep live mixed-panel interaction as downstream evidence, not an RC3 API gate. |

## Panel title slice

The preferred direction is a title-specific retained API implemented on the existing annotation and panel-layout machinery. Do not overload ordinary labels with implicit title detection, and do not add a broad plotting or generic panel-text API for the first slice.

Proposed public shape:

```c
typedef struct DvzAnnotation DvzPanelTitle;

typedef struct DvzPanelTitleDesc
{
    uint32_t struct_size;
    uint32_t flags;
    const char* text;
    float reserve_px;
    float gap_px;
    uint32_t title_flags;
} DvzPanelTitleDesc;

DVZ_EXPORT DvzPanelTitleDesc dvz_panel_title_desc(void);
DVZ_EXPORT DvzPanelTitle* dvz_panel_title(DvzPanel* panel, const DvzPanelTitleDesc* desc);
DVZ_EXPORT DvzId dvz_panel_title_id(const DvzPanelTitle* title);
DVZ_EXPORT DvzResult dvz_panel_title_set_text(DvzPanelTitle* title, const char* text);
DVZ_EXPORT DvzResult dvz_panel_title_set_style(DvzPanelTitle* title, const DvzTextStyle* style);
DVZ_EXPORT DvzResult dvz_panel_title_set_visible(DvzPanelTitle* title, bool visible);
DVZ_EXPORT void dvz_panel_title_destroy(DvzPanelTitle* title);
```

The final API may use the existing annotation handle directly if that keeps ownership and bindings simpler, but the public contract must remain title-specific.

Required first-slice semantics:

- At most one retained title per panel.
- Text is copied before creation or mutation returns.
- `reserve_px == 0` selects a content-aware default; the title contributes an additive top reserve without overwriting axis, colorbar, legend, or caller reserve state.
- Placement is panel-local, top-centered, independent of controller transforms, and never depth-tested.
- Title identity is stable and distinct from any generated text visual identity.
- Text, style, visibility, reserve, resize, and destruction changes invalidate the relevant layout/frame snapshot.
- Frame snapshots expose dedicated panel-text/title guide kind and role values, a resolved title box, and a rendered contribution.
- Existing guide hit testing returns the stable title identity when the title is queryable.
- Destroying or hiding the title removes its rendered contribution; destruction also removes its reserve contribution.
- Subtitle, wrapping, collision solving, rich text, multiple titles, and strict cross-backend font metrics remain out of scope.

Implementation should reuse `DvzAnnotation`, generated text visuals, panel reserve aggregation, panel frame snapshots, and guide hit testing. Add a dedicated panel title reserve bucket rather than folding title state into the caller-owned base reserve.

Acceptance requires focused native tests for descriptor validation, copied text lifetime, stable identity, mutation, visibility, additive reserve, resize, snapshot invalidation, contribution enumeration, hit testing, and destruction. Because this is public API and binding work, regenerate and validate `ctypes` bindings before Python or GSP qualification. Initial GSP capability should be semantic or adapted, not layout-strict, until consumed title boxes and font/layout behavior are proven.

## Image sample and texel contracts

Keep two query meanings separate:

1. A sample query returns the color or value produced by the declared sampler at a coordinate. Linear filtering is valid, and there may be no single source texel identity.
2. A texel query returns an exact integer texel identity. It requires a specified coordinate origin, row/column ordering, addressing behavior, image orientation, and nearest-sampling rule.

Before promoting exact texel identity, define and test:

- whether `texel_id` is a flattened row-major identity and how it maps to integer `(x, y, z)` coordinates;
- top-left versus bottom-left origin at the public API boundary;
- behavior at the upper edge, outside the image, under clamp/repeat addressing, and for reversed image extents;
- whether UVW reports the requested coordinate, addressed coordinate, or texel center;
- behavior for nearest versus linear filtering and multisample or transformed images;
- consistency among `resolved_target`, `texel_id`, `has_uvw`, `uvw`, `value_kind`, sampled value, and displayed RGBA.

Until those rules land, `DVZ_SCENE_TARGET_PIXEL` and `DVZ_SCENE_TARGET_SAMPLE` may expose sampled value/color but must not return a default zero `texel_id` or UVW that downstream code could mistake for valid identity. Unsupported combinations must return an explicit status.

Acceptance requires CPU-level coordinate/addressing tests, native query plan/decode tests, transformed and reversed-extent cases, nearest and linear sampling cases, generated binding checks, and an exact-wheel GSP qualification that distinguishes sampled value from texel identity.

### Proposed exact-texel decisions and test vectors

The following table is a concrete post-RC3 proposal for review, not an active v0.4 contract. Implementation must not advertise exact texel identity until these choices are accepted in the authoritative query specification and all listed vectors pass.

| Concern | Proposed decision |
| --- | --- |
| Public coordinate | Define canonical integer `(x, y, z)` texel coordinates, with `z = 0` for a 2D image. `texel_id` is sufficient to derive them when dimensions are known; adding explicit coordinate fields requires a separate API decision. |
| Origin | Use lower-left public image origin: `x` increases rightward and `y` increases upward. Backend memory-row order remains an implementation detail. |
| Flattening | `texel_id = x + width * (y + height * z)` after addressing. The id is derived from the public integer coordinate, not an upload offset. |
| Addressing | Apply the declared clamp or repeat policy before choosing a texel. Border, mirror, and undefined addressing remain unsupported until separately specified. |
| Nearest rule | For an addressed normalized coordinate in `[0, 1)`, choose `floor(u * width)`, `floor(v * height)`, and `floor(w * depth)`. An exact upper edge under clamp selects the final texel; repeat maps it to zero. |
| UVW | For a successful texel query, return the selected texel center `((x + 0.5) / width, (y + 0.5) / height, (z + 0.5) / depth)`. Do not reuse the requested or pre-addressed coordinate. |
| Reversed extent | Invert the visual transform before addressing. Reversing a displayed extent changes which public texel lies under a panel point but does not change integer coordinates or flattening. |
| Linear sampling | A sample query may return the filtered value and displayed RGBA. An exact-texel query under linear sampling returns explicit unsupported because no unique source texel exists. |
| Result validity | Set `texel_id`, integer coordinates, and UVW together or omit all of them. A zero-filled field without a validity indicator is never a valid identity. |

Use a logical `3 x 2` image whose lower public row contains `10, 11, 12` and upper public row contains `20, 21, 22`. These CPU vectors should be shared by plan, decode, native, binding, and downstream exact-wheel tests:

| Case | Request and policy | Expected result |
| --- | --- | --- |
| Lower-middle center | nearest clamp at `(0.5, 0.25)` | coordinate `(1, 0, 0)`, `texel_id = 1`, UVW `(0.5, 0.25, 0.5)`, value `11` |
| Upper-right center | nearest clamp at `(5/6, 0.75)` | coordinate `(2, 1, 0)`, `texel_id = 5`, UVW `(5/6, 0.75, 0.5)`, value `22` |
| Exact upper edge | nearest clamp at `(1, 1)` | coordinate `(2, 1, 0)`, `texel_id = 5` |
| Negative edge | nearest clamp at `(-0.1, 0.25)` | coordinate `(0, 0, 0)`, `texel_id = 0`, value `10` |
| Repeated upper edge | nearest repeat at `(1, 1)` | coordinate `(0, 0, 0)`, `texel_id = 0`, value `10` |
| Reversed horizontal extent | panel point at the displayed left center after x reversal | coordinate `(2, 0, 0)`, `texel_id = 2`, value `12` |
| Linear center | exact-texel target with linear sampling | explicit unsupported with no identity fields; the separate sample target may return a filtered value |

The accepted specification must additionally cover zero-sized rejection, non-finite coordinates, 3D flattening, row padding, signed and unsigned integer formats, transformed panels, stale snapshots, and the distinction between a clamped hit and an out-of-domain policy rejection.

## Mesh-face and multi-panel responsibilities

Do not create new Datoviz mesh-face APIs for GSP unless a concrete gap is demonstrated. The public face target and result fields are already the intended primitive map. Preserve their bindings and focused tests while GSP adds target selection, semantic visual-ID mapping, topology validation, freshness checks, and any backend-neutral geometry reconstruction.

Likewise, do not redesign Datoviz panels around GSP's plural view collections. GSP now establishes explicit panel/view/attachment routing, and the adapter audit found that existing Datoviz scene, figure, panel, clipping, query, navigation, snapshot, resize, and teardown APIs are sufficient for the implemented slice. The adapter creates one native panel per GSP panel inside one retained scene/figure and keeps panel-local state explicit. Future Datoviz work should add an engine primitive only when a concrete downstream conformance failure demonstrates the gap.

### Proposed combined multi-visual FACE execution

The current fail-closed rule for more than one eligible mesh remains correct. A post-RC3 combined implementation should use one panel-scoped query plan and one depth comparison across every eligible opaque mesh, rather than running per-visual queries and attempting to compare backend-local results afterward. Each query fragment must encode both a plan-local visual slot and the canonical face or primitive identity; decode maps the slot to the retained public visual identity only after snapshot and topology freshness checks pass.

The combined path must reuse the rendered panel viewport, camera, projection, clipping, culling, depth convention, and visibility state. It must exclude unsupported transparent, instanced, stale, or non-triangle contributors explicitly. Equal-depth ties need an accepted deterministic semantic rule or an explicit unsupported result; draw order alone must not silently define public frontmost identity. Existing `DvzQueryResult` visual, face, and primitive fields should be used if they can carry the proven result, and no new public API should be added merely to expose an internal attachment encoding.

Required post-RC3 cases are:

1. Two overlapping opaque meshes at distinct depths return the scene-frontmost visual and canonical face, and reversing creation or draw order does not change the result.
2. Moving either mesh across the other updates the result and snapshot identity without returning stale data.
3. A miss across several eligible meshes remains a miss rather than unsupported.
4. Hidden, clipped, different-panel, and out-of-viewport meshes cannot contribute.
5. Unsupported transparent, instanced, non-triangle, or ambiguous equal-depth contributors return structured unsupported instead of a result from the remaining subset.
6. Resize, camera change, topology replacement, visual destruction, and query-resource recreation preserve freshness and lifetime rules.
7. A single eligible mesh remains byte-for-byte compatible with the already qualified FACE result shape.

Promotion requires focused native depth/order tests, validation-layer coverage, generated binding checks, and exact-wheel GSP tests that map the native visual slot to the semantic visual id. Until then, capability probing must retain the bounded single-eligible-mesh claim and all multi-visual FACE/TRIANGLE requests must fail closed.

The local exact-wheel pass now exercises native partial-layout snapshot aggregation and mixed 2D/3D capture: two panel and view identities were recovered and the installed wheel produced a 640×360 PNG. Unit and source-tree coverage additionally proves consumed full snapshots, attachment routing, targeted queries, independent Matplotlib live revisions, and VisPy2 mixed-panel lowering in both panel orders. Live mixed-panel interaction is the remaining evidence gap. Do not convert that downstream qualification gap into an RC3 public-API requirement.

## Recommended sequence

1. Keep RC3 focused on its existing artifact, platform, documentation, and maintainer-review gates.
2. Let GSP implement and qualify mesh-face picking against the unchanged exact Datoviz wheel.
3. Preserve the exact-wheel mixed-panel capture gate and add a live mixed-panel interaction case when the downstream harness can exercise native input; add Datoviz APIs only if that qualification exposes a concrete engine gap.
4. Implement the panel-title slice only after explicit release-scope approval.
5. Specify image sample versus texel identity, then implement exact texel fields without changing sample-query meaning.
6. Re-run native query/scene tests, generated binding checks, and exact-wheel GSP/VisPy2 qualification after every promoted Datoviz slice.

## Agent pickup

Before implementing a slice, read `AGENTS.md`, `agents/now/START.md`, the matching scene/build rules, [GSP backend readiness](GSP_BACKEND_READINESS.md), [annotation semantics](../scene/semantics/ANNOTATIONS.md), [panel query](../scene/interaction/PANEL_QUERY.md), and [panel frame snapshots](../../docs/architecture/panel-frame-snapshot.md). Confirm the item has been explicitly promoted into the active release scope; this roadmap alone is not authorization to expand a release candidate.

For public headers or bindings, run `just ctypes` and `just ctypes-check`. For scene/query implementation, run the focused native suite followed by `just build`, the relevant `just test` filters, `just spec-check`, and `git diff --check`. Requalify from exact built wheels rather than editable source imports before changing downstream capability claims.
