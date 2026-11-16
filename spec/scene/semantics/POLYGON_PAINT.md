# Polygon Paint Contract And RC3 Plan

Status: accepted target semantics and implementation roadmap, approved 2026-11-17 after an independent architecture audit. The correction is implemented and locally validated; headed-browser proof, canonical-media approval, and exact-candidate acceptance remain pending. This is a blocking requirement before freezing or releasing v0.4.0rc3; it is not deferred to RC4. Existing headers describe the currently installed API, not proof that this target is implemented.

## Decision And Rationale

Semantic polygons represent planar 2D paint, with F64 XY rings, holes, region identity, fill, and stroke. Their colors must behave like chart or map colors. Triangulating a polygon into a mesh is an implementation detail and must not turn it into a lit physical surface.

Before this correction, the fill inherited Phong lighting and opaque alpha mode from `dvz_mesh()`. Merely choosing unlit/blended does not fix composition: phase separation can move translucent fills after opaque strokes, role `z + 1` interleaves separate composites incorrectly, and an aggregated polygon set cannot reproduce per-item ordering with just one fill draw and one stroke draw. The [gallery review](../../../agents/now/GALLERY_REFERENCE_REVIEW.md) preserves the diagnostic evidence.

## Required Behavior

| Concern | Accepted contract |
| --- | --- |
| Color and lighting | Unlit fill and stroke preserve authored data colors. Follow [color management](COLOR_MANAGEMENT.md): sRGB-authored RGB, linear-light blending, linear alpha. Camera, coordinate space, triangulation normals, and panel lights do not implicitly select lighting. Ordinary mesh defaults stay unchanged. |
| Alpha | Both roles use stable source-over composition for alpha 0, fractional alpha, and alpha 255. Recoloring or animating alpha must not change phase, depth policy, or ordering. Do not infer an AUTO mode by scanning colors. |
| Depth and effects | Paint does not test or write scene depth, supply surface normals/depth, or accidentally participate as an AO/GTAO producer or scene occluder. |
| Default item order | For items A then B, draw fill A, stroke A, fill B, stroke B. A polygon set in append order must match the equivalent sequence of single polygons. |
| Explicit collection order | An opt-in collection policy draws all fills, then all strokes. This supports maps where every boundary should stay visible; document its different overlap result. It is not the default item-order contract. |
| Composition scope | Use a reusable ordered-paint composition policy after world/volume scene composition and before UI overlays. Parent attachment order and internal item/role order are distinct keys. Images can interleave only when explicitly admitted to the same paint policy; `z_layer` does not reorder arbitrary rendering phases. |
| Visibility | Hidden or zero-effective-alpha contributions are consistently excluded from visible rendering and visible-hit queries. Holes remain transparent except for their stroke coverage. |
| Picking | Select the topmost covered paint contribution with positive effective alpha and return its semantic region ID. Do not substitute a mesh face ID or a largest-contribution heuristic. Unsupported query routes require a clear diagnostic. |

This is an explicit paint composition contract alongside [ordinary transparency](TRANSPARENCY.md), not a change to the global opaque default or physical 3D depth semantics. MASK, WBOIT, and depth-peeling combinations unsupported by either role must diagnose rather than silently substitute another technique.

For a physical lit surface, use `dvz_triangulate_polygon()` followed by `dvz_mesh_set_geometry()` and explicit mesh material, transparency, and depth choices. Do not add an ambiguous surface mode to the semantic polygon just because its fill uses triangles.

## Ownership And API

The semantic object owns topology, region IDs, RGBA, visibility, and stroke style. The view owns persistent presentation and ordering. Establish defaults at construction or explicit reset; geometry preparation must not repeatedly overwrite user-owned presentation choices. Account for material descriptors that also carry alpha mode so setters do not accidentally reset composition.

Generated role access must document ownership per field. `dvz_polygon_composite_set_order()` selects persistent `DVZ_POLYGON_ORDER_ITEMS` (default) or explicit `DVZ_POLYGON_ORDER_COLLECTION`; `dvz_visual_set_paint(visual, true)` explicitly admits an image or compatible visual to ordered paint composition. Mutable advanced access may remain if semantic-owned fields are explicitly documented as regenerated and user-owned presentation fields persist through updates. The semantic owner fixes paint composition on generated roles: `dvz_visual_set_paint(role, false)` must fail. Paint validation rejects lit material models where the visual consumes a material, enabled depth cueing, volume proxies, additive blending, and mask/OIT alpha modes; unused material-shaped stroke storage does not imply lighting. Const inspection and typed view setters are an option when justified, not a prerequisite or an excuse to introduce duplicate owners. API compatibility may break before RC3 when needed for this contract; exact signatures remain an implementation decision.

Do not introduce a generic composite opacity setter in this fix. Fill/stroke RGBA already expresses per-role transparency. Multiplying both roles independently is not isolated whole-object opacity; isolated group opacity would require a separate contract.

## Current Implementation Boundary

The correction introduces normalized paint composition and shared-buffer item draw spans while retaining one generated fill visual and one stroke visual per polygon set. Public constructors remain unchanged; the order setter controls view overlap policy. Unlit source-over paint is independent of panel lights, generated normals, and per-frame alpha values.

Rendered semantic region queries are not implemented by this correction. Until a combined GPU-authoritative paint query path can preserve coverage and order, the executor conservatively returns `DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY` for a panel containing visible drawable paint, before trying background visuals. This applies regardless of role query capabilities or fixed-controller attachment. Hidden/empty roles do not block queries. The diagnostic is panel-wide rather than a CPU geometry test; holes or pixels outside a visible polygon do not permit a background fallback. Stored region IDs must not be advertised as working rendered identity.

This diagnosed unsupported query route satisfies the plan's explicit unsupported-route boundary; it does not claim region picking support. The full target above remains the required behavior for any future supported paint query path. The [implementation evidence and remaining release gates](../../../agents/now/GALLERY_REFERENCE_REVIEW.md#polygon-paint-implementation-2026-11-17) distinguish completed local acceptance from pending browser and canonical-media approval.

## Implementation Sequence

1. **Persistent defaults and ownership:** define the public view/order contract, initialize unlit paint defaults for both roles, unify hidden/zero-alpha preparation, and document mutation behavior. Audit bars/bands and other similar composites for the same policy mistakes without assuming they share the polygon lighting bug or broadening into an unrelated renderer refactor.
2. **Ordered lowering and batching:** carry normalized paint policy and parent/item/role ordering through existing attachments, FramePlan, DRP2, and vklite. Replace role `z + 1` ordering. Emit ordered spans/draw ranges from shared collection buffers rather than retaining two visuals per region. Support the explicit collection mode and an image/paint interleave fixture. Generic planning consumes normalized facts rather than switching on polygon family names; do not create a parallel renderer.
3. **Queries and mutation:** map fill/stroke coverage back to semantic region IDs, preserve IDs through recoloring, visibility, and topology updates, and diagnose unsupported query combinations. Keep color-only updates from needlessly rebuilding polygon topology; measure large-collection resource and draw costs.
4. **Validation and media:** complete the checks below, update public documentation and C/Python/WebGPU routes together, regenerate the affected polygon and choropleth captures, and record implementation heads and results in release evidence. Canonical media promotion remains a separate exact-asset approval step.

All four steps are required before RC3 freeze and release. A two-flag visual fix, a written plan alone, or an approved-looking screenshot does not close this gate. Broader PBR, new OIT techniques, isolated group opacity, and a lit-polygon convenience API are outside this fix.

## Acceptance And Closure

- Prove opaque palette fidelity under changing panel lights and triangulation normals; check known linear source-over pixels and alpha transitions 0/fractional/255 after attachment.
- Compare overlapping singles against the same ordered polygon set, including opaque/translucent strokes, mixed region alpha, holes, and visibility. Verify the intentional difference of collection order and the declared boundaries with world, volume, and UI phases.
- Verify image/polygon interleaving within paint composition and persistence of user-owned presentation through geometry/style refreshes.
- Check visible-hit picking for overlaps, holes, stroke edges, hidden/zero-alpha regions, and stable semantic IDs after updates; unsupported routes must fail explicitly rather than report incorrect identity.
- Exercise a large choropleth without two retained visuals per region; record draw counts, retained resources, and color-only update costs. Preserve shared-buffer batching where order allows it.
- Build and run focused native/Vulkan checks and the existing C, Python, and live WebGPU polygon/choropleth routes, including GLSL/WGSL output, packet checks, and browser validation. Record unavailable validation as a limitation, never a pass.
- For public API/binding changes run `just ctypes` and `just ctypes-check` before Python validation; apply relevant shader ABI, specification/DRP2, documentation, and example-manifest checks. Run `git diff --check`.

Close the RC3 blocker only when implementation, focused regressions, cross-language/backend evidence, and regenerated media review are recorded. The earlier gallery and lighting test passes do not establish this new contract.

## Related Composite Audit

Bars and bands use primitives without normal attributes, so they already take the unlit primitive shader path and do not share the polygon fill-lighting bug. Their alpha-dependent defaults and fill/outline attachment ordering remain a separate bounded migration candidate for the reusable paint policy; this polygon correction does not claim to change their overlap semantics.
