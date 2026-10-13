# Scientific 3D Profile Handoff

Status: approved queue, 2026-10-13. The required native non-volume target and RC4/RC5/final boundaries are in [the profile slice](../../spec/scene/slices/V04_SCIENTIFIC_3D_PROFILE.md). This record separates source inspection from delivered behavior; it does not close a release gate. Preserve [current release evidence](STATUS.md) and its recorded revisions.

## Source baseline and gaps

The technique audit inspected `b1ca5a195f921d531cdf729bd7e420ee732888d7`. Findings below are source evidence; their specific GPU symptoms were not reproduced during the audit. Existing successful scene tests do not prove every family/alpha combination.

| Native source path | Current boundary | Required disposition |
| --- | --- | --- |
| Ordinary mesh/primitive and splat WBOIT | Dedicated accumulation shaders exist; ordinary mesh delegates to primitive selection. | Preserve these supported paths and add mixed-profile regressions. |
| Point/pixel/marker WBOIT | Shared point shader resolver ignores the accumulation flag: `src/scene/visuals/point/shader.c:47`; pass caps still advertise WBOIT products: `registry/pass_caps.c:150`. | RC3 guard/diagnostic; required arbitrary-point WBOIT delivery in RC4/RC5. Marker/pixel expansion is optional unless used by an accepted scenario. |
| Textured mesh and image WBOIT | Resolvers reject accumulation: `mesh/shader.c:54`, `image/shader.c:45`; generic alpha state accepts it. | RC3 precise rejection; required conformant sampled-plane realization in RC4/RC5. |
| Image depth | `image/pipeline.c:60` uses no writes and `ALWAYS`; `registry/pass_caps.c:89` excludes image raster depth. | Preserve 2D overlay behavior; deliver the required 3D plane profile through an explicit conformant family/lowering path. |
| Sphere/path WBOIT | `sphere/shader.c:45` and `path/shader.c:48` reject accumulation. | Diagnose exclusions; preserve opaque spheres/paths mixed with required WBOIT geometry. Promote translucent variants only from concrete use cases. |
| Depth peeling | `registry/shader_desc.c:579` substitutes primitive fragments for every peel alpha mode; image/textured-mesh interfaces and sphere/point coverage differ. Init shaders write bounds without effective-alpha discard. | Guard unsupported combinations and test zero-alpha layers on supported mesh/primitive peeling; full-family peeling is deferred. |
| Alpha mask | Classification recognizes `DVZ_ALPHA_MASK`, but shader paths lack cutoff semantics and generally discard only alpha ≤ 0. | Honest RC3 capability status; shared shading/depth/query validity for required cutout planes before final. |
| AO surface capture | Primitive capture removes material layout (`registry/pipeline_desc.c:197`) and tests vertex alpha (`gbuffer_normal.frag:16`); shading also applies material opacity/base-color alpha (`scene_material.glsl:126`). | RC3 focused effective-alpha regression and correction; no broad AO-family expansion. |
| Scene occlusion | Generic depth fragment drops point/marker/image coverage; sphere/textured mesh reach the skip branch (`registry/shader_desc.c:400–431`). | Explicit eligible producers and diagnostics; coverage-conformant expansion only when required. Preserve existing optional volume behavior. |
| WebGPU advanced techniques | Basic lighting/visual routes exist; advanced WGSL/composition coverage is narrower. | Maintain honest route differences and missing-source diagnostics; full parity is deferred. |

Source paths abbreviated in the table are under `src/scene/visuals/` or `src/scene/shaders/glsl/`. Recheck line numbers and behavior against the implementation head before changing code. Capability and provider fixes belong behind normalized visual-family interfaces, not new concrete-family branches in generic emission.

## Ordered execution

1. Build an executable family/alpha/backend eligibility inventory from descriptors and shader contracts; reject unsupported combinations before planning/emission and correct contradictory status/spec claims. Separate temporary guards from completed final-profile delivery.
2. Reproduce and repair supported mesh/primitive peel effective-alpha validity and AO material-alpha capture. Verify final fragment discard agrees with bounds/surface products, including vertex alpha zero, material opacity zero and base-color alpha zero. Keep analytic-sphere AO and existing lighting regressions intact.
3. Deliver shared WBOIT for arbitrary points and sampled planes alongside meshes. Preserve family sampling, coverage, material response and opaque depth; test mixed spheres/paths and order reversal. Implement required plane depth/cutout without changing 2D overlays.
4. Prove query identity and retained updates together: face and supported item targets, signed link keys, mapping changes, explosion, visibility, smaller/larger/empty-restored payloads, and gesture/stationary-hover freshness. Reuse existing query and transactional resource paths.
5. Coordinate IBL package acceptance: multiple insertions/arbitrary point collections, independent region visibility, palette-alpha mode selection, volume-free linked navigator, scientific scalar/NaN semantics and linked tree/tables. These are package port dependencies; they do not impose old API compatibility on Datoviz.
6. Run focused engine/C/Python/WebGPU scenario validation, then real-data live interaction and matched performance checks. Refresh bindings after public API changes and shader ABI after shader/layout changes. Record unavailable GPU/browser/platform runs as pending or skips, then close the exact-candidate/artifact matrix.

## Acceptance evidence and package boundary

The inspected IBL package at `ae385fb45c68b5b4eeba48c16da9dcfe0f2ec5e0` uses mesh WBOIT, opaque analytic recording-site spheres, opaque round-cap/join paths and separate 2D image overlays. Its [real-data roadmap](https://github.com/int-brain-lab/ibl-datoviz/blob/ae385fb45c68b5b4eeba48c16da9dcfe0f2ec5e0/docs/REAL_DATA_EXAMPLE_ROADMAP.md) and [handoff](https://github.com/int-brain-lab/ibl-datoviz/blob/ae385fb45c68b5b4eeba48c16da9dcfe0f2ec5e0/docs/NEXT_STEPS.md) retain live-review and artifact gates. Palette RGBA is accepted while surface alpha-mode selection checks global opacity/selection; fix that package decision when porting independent region alpha. Linked table keys do not establish direct site clicking; distinguish site-item identity from mapped region identity explicitly.

Use the existing first six real-data scenarios as baseline acceptance, then add arbitrary points, multiple trajectories, region/hemisphere visibility and the volume-free linked navigator. Current linked slices do not demonstrate 3D image depth. Complete registered-section availability or an explicit absent-section policy belongs to anatomy/package acceptance, not a renderer workaround.

The [earlier controller snapshot](https://github.com/int-brain-lab/ibl-datoviz/tree/9901a35b5991b72ad79ec92d774aff32e05e5995/ibl_datoviz) provides the historical use-case inventory: independent regional color/opacity and hemisphere visibility, arbitrary scalar/RGBA point sets with size/alpha updates, and multiple insertion identities with independent color/width/visibility. Port these behaviors through the current package model; the old object-oriented API and notebook integration are not engine compatibility requirements.

RC3 closes supported-path correctness and diagnostic honesty; RC4 delivers the mandatory profile, with RC5 contingency if necessary. Final closes exact-candidate conformance without new semantics. Do not defer the required non-volume rows merely because advanced volume composition, full-family peeling or broad WebGPU parity remain later work. Do not remove already available optional volume/navigator behavior as a consequence of that deferral.
