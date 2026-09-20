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
| Multi-panel GSP execution | Datoviz already supports multiple retained panels; the GSP scene schema and adapters still need explicit multi-view routing. | GSP-owned first. Add Datoviz APIs only if the implemented GSP contract demonstrates a missing engine primitive. |

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

## Mesh-face and multi-panel responsibilities

Do not create new Datoviz mesh-face APIs for GSP unless a concrete gap is demonstrated. The public face target and result fields are already the intended primitive map. Preserve their bindings and focused tests while GSP adds target selection, semantic visual-ID mapping, topology validation, freshness checks, and any backend-neutral geometry reconstruction.

Likewise, do not redesign Datoviz panels around GSP's proposed plural view collections. GSP should first establish explicit panel/view/attachment routing and keep adapters fail-closed for unsupported multi-panel execution. When that contract is accepted, audit existing Datoviz grid, panel, clipping, query, navigation, snapshot, resize, and teardown APIs against it. Add only missing primitives revealed by that audit.

## Recommended sequence

1. Keep RC3 focused on its existing artifact, platform, documentation, and maintainer-review gates.
2. Let GSP implement and qualify mesh-face picking against the unchanged exact Datoviz wheel.
3. Let GSP settle its plural-view and attachment contract, then audit Datoviz multi-panel execution against concrete conformance cases.
4. Implement the panel-title slice only after explicit release-scope approval.
5. Specify image sample versus texel identity, then implement exact texel fields without changing sample-query meaning.
6. Re-run native query/scene tests, generated binding checks, and exact-wheel GSP/VisPy2 qualification after every promoted Datoviz slice.

## Agent pickup

Before implementing a slice, read `AGENTS.md`, `agents/now/START.md`, the matching scene/build rules, [GSP backend readiness](GSP_BACKEND_READINESS.md), [annotation semantics](../scene/semantics/ANNOTATIONS.md), [panel query](../scene/interaction/PANEL_QUERY.md), and [panel frame snapshots](../../docs/architecture/panel-frame-snapshot.md). Confirm the item has been explicitly promoted into the active release scope; this roadmap alone is not authorization to expand a release candidate.

For public headers or bindings, run `just ctypes` and `just ctypes-check`. For scene/query implementation, run the focused native suite followed by `just build`, the relevant `just test` filters, `just spec-check`, and `git diff --check`. Requalify from exact built wheels rather than editable source imports before changing downstream capability claims.
