# v0.4 Native Scientific 3D Profile

Status: approved implementation and release-acceptance plan, 2026-10-13. The non-volume profile below is required before final v0.4; it is not a claim of current implementation or validation. This slice applies the canonical [transparency](../semantics/TRANSPARENCY.md), [graph techniques](../implementation/GRAPH_TECHNIQUES.md), [occlusion](../implementation/OCCLUSION_EFFECTS.md), [lighting](../semantics/LIGHTING.md), and [query](../interaction/GPU_QUERY_SYSTEM.md) contracts without replacing their authority. Current execution belongs in [the handoff](../../../agents/now/SCIENTIFIC_3D_PROFILE_HANDOFF.md).

## Required native behavior

The profile supports atlas and other scientific scenes containing regional meshes, sampled planes, arbitrary point collections, analytic spheres, and opaque trajectories. It preserves the scientific use cases of earlier IBL viewers through v0.4 APIs; it imposes no v0.3 API or ABI compatibility requirement.

| Component | Required behavior before final v0.4 |
| --- | --- |
| Surface meshes | Indexed geometry, normals, categorical or scalar-derived RGBA, independent region visibility and opacity, opaque depth, and WBOIT; multiple regions and hemispheres may coexist. |
| Sampled planes | Data-space textured rectangles, texture-alpha cutout, opaque or masked depth testing/writes, and transparent WBOIT; intersecting planes and meshes share the same camera/depth convention. A conformant textured-mesh realization is sufficient; screen-space image overlays retain their overlay behavior. |
| Arbitrary points | Dense XYZ with scalar-derived or explicit RGBA, per-point size and alpha, opaque depth and WBOIT, dynamic positions/colors/sizes, visibility and replacement. This is independent of the atlas-specific recording-site object model. |
| Analytic spheres | Correct silhouette, hit normal and fragment depth; opaque spheres compose correctly with WBOIT meshes, points and sampled planes. Sphere WBOIT is optional unless a concrete translucent-sphere use case is promoted into RC4 or RC5 scope. |
| Trajectories | Multiple opaque paths with independent positions, widths, colors and visibility; stable caps/joins and depth behavior under camera movement. Translucent stroke OIT and general tube generation remain optional. |
| Queries and identity | Mesh-face and supported point/sphere item queries return stable scene identity; signed scientific region IDs survive 64-bit link-key encoding; selection and hover stay distinct. Query freshness follows geometry, camera, visibility and identity updates. |
| Linked 2D views | Ordered anatomy, annotation, hover, boundary and cursor layers; linear anatomy versus nearest categorical sampling; consistent slice orientation, coordinates and selection across panels. |
| Interaction and output | Arcball/navigation, gesture-aware hover, resizing, embedded viewport input, [HiDPI](../integration/HIGH_DPI.md), deterministic offscreen scenarios and documented raster color semantics. |

Meshes, points and transparent sampled planes must participate in one compatible WBOIT composition within a panel, including opaque-sphere and opaque-path occlusion. Family-specific surface evaluation happens before the shared accumulation equations; generic fragment replacement must not discard texture sampling, analytic coverage or corrected depth. Authored source-over overlay order remains separate from OIT phase order. Correct cutout discard must be shared by shading and every participating capture, occlusion and query path.

Current source accepts more alpha-mode combinations than its shaders implement. Unsupported family, material, technique or backend combinations must fail with precise diagnostics before an invalid pipeline or misleading product graph is emitted. An explicit temporary exclusion is valid during candidate development, but does not satisfy a required final-profile row.

## Engine and package ownership

Datoviz owns visual rendering, normalized capability declarations, retained updates, query identity/freshness, panel/input/GUI runtime behavior and capture. Changes must stay on the scene → FramePlan → DRP2 → runtime path and respect the [visual-family boundary](../visuals/BOUNDARY_CONTRACT.md); no IBL-specific renderer or parallel backend path is introduced.

The [IBL package](https://github.com/int-brain-lab/ibl-datoviz) owns controller conveniences, atlas mappings, hemisphere geometry and caps, scientific scalar reduction, missing-value presentation, legends/units, and linked application selection. Package port work includes multiple insertion identities, arbitrary point collections, independent region show/hide/remove, and selecting an appropriate alpha mode for per-region RGBA palettes. These dependencies require package acceptance, not restoration of an old engine ABI.

The package must offer a volume-free navigator path so linked slices, surface, points, trajectories and cursor behavior can be accepted independently of deferred volume work. Existing optional bounded volume rendering remains available under its documented limitations. Deferring advanced volume composition does not authorize removing the existing navigator or claiming that its current volume route is physically unified with surfaces.

## Release sequence

| Milestone | Scope and exit |
| --- | --- |
| RC3 | Repair capability/diagnostic honesty and supported-path correctness, including effective-alpha validity in peeling and AO capture; record exact unsupported combinations and focused regressions. Keep completed graph products and lighting intact. |
| RC4 | Deliver the required non-volume profile, prioritizing shared mesh/point/sampled-plane WBOIT, data-space plane depth/cutout, and scientific query/update composition. Prove package port dependencies and real-data interaction alongside engine examples. |
| RC5 contingency | If RC4 cannot validate the required profile, use another candidate for remaining implementation and affected conformance; do not turn unverified exclusions into final acceptance. RC5 is a contingency, not a claim that a candidate is already scheduled or published. |
| Final v0.4 | Close the profile matrix against the frozen candidate and actual artifacts, resolve candidate feedback, and record platform exceptions. Final validation adds no new rendering semantics or optional feature expansion. |

Advanced volume composition, volume OIT, full-family depth peeling, advanced WebGPU parity, full PBR, environment lighting, shadows, GPU-displaced surfaces and broad streaming/backend refactors remain later work. Point or spot lights are not required for readable scientific categorical/scalar colors; preserve the completed [lighting foundation](RC3_LIGHTING_FOUNDATION_SLICE.md).

## Acceptance scenarios

1. Mixed opaque/WBOIT scene: intersecting regional meshes, translucent scalar/RGBA points and sampled planes, with opaque spheres and paths in front and behind. Reverse transparent draw order and inspect overlap, zero alpha, opacity changes, silhouettes, texture sampling and camera/projection changes; use algorithm-appropriate tolerances rather than demanding exact WBOIT layering.
2. Plane/cutout scene: textured planes cross an opaque mesh and each other; transparent holes expose the correct geometry. Validate depth test/write policy and matching coverage for any supported capture, occlusion or query participation; screen-space overlays remain correctly ordered.
3. Identity/update scene: select signed left/right region IDs and point/sphere items; replace smaller/larger/empty-restored data, change mappings/colors/visibility, explode geometry and move the camera. Verify current draw counts, keys and queries, with no stale hover or lost committed selection during gestures.
4. Linked non-volume atlas: surface plus independent AP/ML/DV panels, anatomy/annotation sampling, boundaries, cursor and selection overlays; multiple trajectories and scalar sites; filtering, tooltips, docking, resize, HiDPI and clean shutdown. Package-owned missing values, units and reduced mappings are explicit.
5. Performance evidence: compare matched real-data baseline, moving hover, drag, stationary pointer, recoloring/replacement and slice-update workloads. Record source/native/artifact identity, GPU/backend, extent and scaling, latency distributions, query counts and memory/resource growth. Preserve comparable prior measurements; no portable FPS threshold is inferred from one host.

Each newly published portable engine example is one C/Python/WebGPU deliverable under [the example policy](../../../docs/contributors/adding-examples.md): canonical C, runnable direct-engine Python, live WebGPU route, packet checks and browser proof. Existing missing WGSL implementations are work gaps, not intrinsic platform exceptions. An intrinsic browser limitation must be identified precisely in the manifest; any reduced route must name the changed semantics and prove its supported behavior. The native profile remains the mandatory target; neither a reduced browser route nor an unavailable browser run establishes native/browser parity.

Acceptance records must distinguish declared capability, successful frame planning/emission, shader compilation, GPU execution, inspected pixels, interaction review and exact-package/platform proof. Use a family × alpha mode × technique × backend matrix with explicit supported, excluded, pending and failed states; record the head and environment for every validation result.
