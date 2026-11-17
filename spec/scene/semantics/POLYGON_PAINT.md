# Polygon Paint Contract

Status: normative target semantics with an explicit implementation boundary. Updated 2026-11-17: the maintainer-approved RC3 scope ships the existing rendering correction and diagnosed unsupported rendered paint queries; composed GPU paint picking and WebGPU device-lifecycle architecture are allocated to RC4. This allocation does not weaken the target semantics below or establish browser validation.

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

Generated role access must document ownership per field. `dvz_polygon_composite_set_order()` selects persistent `DVZ_POLYGON_ORDER_ITEMS` (default) or explicit `DVZ_POLYGON_ORDER_COLLECTION`; `dvz_visual_set_paint(visual, true)` explicitly admits an image or compatible visual to ordered paint composition. Mutable advanced access may remain if semantic-owned fields are explicitly documented as regenerated and user-owned presentation fields persist through updates. The semantic owner fixes paint composition on generated roles: `dvz_visual_set_paint(role, false)` must fail. Paint validation rejects lit material models where the visual consumes a material, enabled depth cueing, volume proxies, additive blending, and mask/OIT alpha modes; unused material-shaped stroke storage does not imply lighting. Const inspection and typed view setters are an option when justified, not a prerequisite or an excuse to introduce duplicate owners. Pre-release API compatibility may break when needed for this contract; exact future signatures remain an implementation decision.

Do not introduce a generic composite opacity setter in this fix. Fill/stroke RGBA already expresses per-role transparency. Multiplying both roles independently is not isolated whole-object opacity; isolated group opacity would require a separate contract.

## Current Implementation Boundary

The correction introduces normalized paint composition and shared-buffer item draw spans while retaining one generated fill visual and one stroke visual per polygon set. Public constructors remain unchanged; the order setter controls view overlap policy. Unlit source-over paint is independent of panel lights, generated normals, and per-frame alpha values.

Rendered semantic region queries are not implemented by this correction. Until a combined GPU-authoritative paint query path can preserve coverage and order, the executor conservatively returns `DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY` for a panel containing visible drawable paint, before trying background visuals. This applies regardless of role query capabilities or fixed-controller attachment. Hidden/empty roles, roles without enabled draw spans, and roles with provably zero effective material alpha do not block queries. For material-shaded geometry, zero opacity or zero base-color alpha proves zero contribution. Mere lack of coverage at the requested pixel does not prove the panel free of drawable paint. The diagnostic is panel-wide rather than a CPU geometry test; holes or pixels outside a visible polygon do not permit a background fallback. Stored region IDs must not be advertised as working rendered identity.

This diagnosed unsupported query route is the RC3 query boundary; it does not claim region picking support. The full picking contract above applies when that route becomes supported. The [implementation evidence](../../../agents/now/GALLERY_REFERENCE_REVIEW.md#polygon-paint-implementation-2026-11-17) retains the completed local checks and their exact scope. The [RC4 implementation slice](../slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md) owns composed picking and device replacement acceptance.

## Rendering Correction And Release Allocation

1. **Persistent defaults and ownership:** define the public view/order contract, initialize unlit paint defaults for both roles, unify hidden/zero-alpha preparation, and document mutation behavior. Audit bars/bands and other similar composites for the same policy mistakes without assuming they share the polygon lighting bug or broadening into an unrelated renderer refactor.
2. **Ordered lowering and batching:** carry normalized paint policy and parent/item/role ordering through existing attachments, FramePlan, DRP2, and vklite. Replace role `z + 1` ordering. Emit ordered spans/draw ranges from shared collection buffers rather than retaining two visuals per region. Support the explicit collection mode and an image/paint interleave fixture. Generic planning consumes normalized facts rather than switching on polygon family names; do not create a parallel renderer.
3. **Queries and mutation:** RC3 preserves the conservative unsupported diagnostic and semantic IDs through updates. RC4 maps composed fill/stroke coverage to those IDs. Keep color-only updates from needlessly rebuilding polygon topology; measure large-collection resource and draw costs.
4. **Validation and media:** complete the checks below, update public documentation and C/Python/WebGPU routes together, regenerate the affected polygon and choropleth captures, and record implementation heads and results in release evidence. Canonical media promotion remains a separate exact-asset approval step.

The bounded RC3 investigation obtained polygon and choropleth browser rendering proof with the explicit ANGLE/Vulkan profile in headless and headed/Xvfb modes. The default canvas-presentation failure and passing configuration are recorded in [release evidence](../../../agents/now/GALLERY_REFERENCE_REVIEW.md#rc3-browser-validation-and-scope-2026-11-17); this does not establish universal browser support or device-loss recovery. A later persistent provider-only failure may use only the approved narrowly scoped browser-validation exclusion, with exact evidence and no native/Python or exact-artifact waiver.

## Acceptance And Closure

- Prove opaque palette fidelity under changing panel lights and triangulation normals; check known linear source-over pixels and alpha transitions 0/fractional/255 after attachment.
- Compare overlapping singles against the same ordered polygon set, including opaque/translucent strokes, mixed region alpha, holes, and visibility. Verify the intentional difference of collection order and the declared boundaries with world, volume, and UI phases.
- Verify image/polygon interleaving within paint composition and persistence of user-owned presentation through geometry/style refreshes.
- For RC3, check the panel-wide unsupported diagnostic, including paint without query capabilities or on fixed-controller attachments, and the hidden/empty/zero-contribution exceptions. RC4 visible-hit picking acceptance is defined in the [implementation slice](../slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md); unsupported routes must continue to fail explicitly.
- Exercise a large choropleth without two retained visuals per region; record draw counts, retained resources, and color-only update costs. Preserve shared-buffer batching where order allows it.
- Build and run focused native/Vulkan checks and the existing C, Python, and live WebGPU polygon/choropleth routes, including GLSL/WGSL output, packet checks, and browser validation. Record unavailable validation as a limitation, never a pass.
- For public API/binding changes run `just ctypes` and `just ctypes-check` before Python validation; apply relevant shader ABI, specification/DRP2, documentation, and example-manifest checks. Run `git diff --check`.

Record RC3 rendering regressions, native/Python/packet evidence, browser proof or the exact approved browser-validation exclusion, and regenerated media review separately. Earlier gallery and lighting passes do not establish this contract; the RC4 work does not reopen completed RC3 rendering evidence.

## Related Composite Audit

Bars and bands use primitives without normal attributes, so they already take the unlit primitive shader path and do not share the polygon fill-lighting bug. Their alpha-dependent defaults and fill/outline attachment ordering remain a separate bounded migration candidate for the reusable paint policy; this polygon correction does not claim to change their overlap semantics.
