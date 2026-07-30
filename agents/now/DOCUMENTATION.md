# Datoviz v0.4 Documentation Plan

Status: active RC3 documentation, gallery, and tutorial-pilot gate followed by RC4 tutorial completion. Updated: 2026-07-30.

Use [RELEASE.md](RELEASE.md) for sequencing, [STATUS.md](STATUS.md) for current blockers, [../../spec/docs/](../../spec/docs/) for durable information architecture and documentation decisions, and [../../spec/release/](../../spec/release/) for readiness, communication, and attribution policy.

The checkpoint-by-checkpoint completion record and PR #132 disposition are in [RC3_DOCUMENTATION_INVENTORY.md](RC3_DOCUMENTATION_INVENTORY.md).

## Current Position

The public v0.4 site is deployed with RC2 installation guidance, release notes, generated examples, a broad native and WebGPU gallery, generated C-reference infrastructure, Python binding guidance, and the preserved v0.3 site under `/v0.3/`.

Build-local animated media is current under the implemented policy: all 38 animations regenerated at canonical `1280x720`, the comparison report selects 29 MP4 cards and nine animated WebPs, all poster and card variants remain within budget, and the full freshness gate passes. The native screenshot policy now designates the physical Linux host `fractal`; source commit `c5fcaeb3a` captured all 104 reviewed PNGs twice in serial order on its NVIDIA GeForce RTX 5090 with driver 595.84, and both runs are byte-identical for every screenshot. Compared with the committed set, 50 candidates are byte-identical, 46 are tightly pixel-equivalent, and eight differ materially under the strict tolerance. The build-local report and provenance are ready for review; no canonical PNG, `data` commit, gitlink, or published asset has changed.

The dataset audit now enforces name, source link, license, citation guidance, preprocessing, and provenance for every real or prepared manifest entry. All 12 dataset-bearing showcases pass, and the RESEPI point-cloud page now exposes its previously missing citation and preparation record. Cross-cutting v0.4 constraints are consolidated in the public Known limitations page while exact published-candidate issues remain in their release notes.

Do not modify or stage the `data` submodule without explicit approval for the exact change. Generated build-local comparison media remains ignored and uncommitted.

## Required for RC3

1. Keep the documentation structure and navigation stable enough that only blocker-level corrections remain after RC3.
2. Complete the generated C reference or its complete generated outline from parsed public headers, using `build/bindings/datoviz_api.json` as the first candidate source rather than maintaining symbol catalogs manually.
3. Provide usable Python binding guidance for top-level NumPy-adapted calls and exact `datoviz.raw` calls, including ownership, callback lifetime, array adaptation, and the GSP/VisPy2 boundary.
4. Ensure release examples have current captured artifacts and link representative render-conformance, fixture, native, or browser proof.
5. Reconcile known issues and feature status with RC1/RC2 feedback and the actual supported, experimental, advanced/unstable, deferred, and external/GSP surfaces.
6. Review real-dataset showcases for source links, license and citation terms, prepared-data provenance, scientific context, and usefulness to the dataset authors.
7. Document and validate the gallery generation path for screenshots, animated WebP, MP4 cards, posters, and freshness checks using [GALLERY_MEDIA_SINGLE_RESOLUTION.md](GALLERY_MEDIA_SINGLE_RESOLUTION.md).
8. Review outreach drafts before contacting dataset authors or publishing external messages.
9. Publish the first three chapters of the release-required modern GPU graphics tutorial with complete standalone CMake examples, external GLSL, live GLFW and deterministic offscreen paths, focused C guidance, ownership explanations, experiments, exercises, screenshots, validation commands, and advanced/unstable compatibility labels.
10. Document every RC3 tutorial-facing public API change and keep generated C reference, bindings, ownership guidance, diagnostics, installed-package instructions, and tutorial source snippets synchronized with the compiled examples.

## Active Documentation Work

- The gallery resolution, encoding, freshness, and bounded-parallelism implementation specified in [GALLERY_MEDIA_SINGLE_RESOLUTION.md](GALLERY_MEDIA_SINGLE_RESOLUTION.md) is complete. Build-local regeneration and comparison pass for all 38 animations with zero budget violations; publishing or changing canonical media still requires exact approval.
- The visual-system pilot specified in [HANDOFF_VISUAL_DOCUMENTATION_PASS.md](HANDOFF_VISUAL_DOCUMENTATION_PASS.md) is implemented on the four approved pages and passes strict build plus software-rendered desktop/mobile inspection. Obtain maintainer review before broader rollout.
- The designated Linux reference workflow produced two byte-identical 104-image capture sets plus provenance and an old/new/diff review index. Review the complete build-local candidate set and obtain exact approval before replacing the 54 byte-different canonical PNGs, adding committed provenance, creating a `data` commit, or staging the parent gitlink.
- Dataset attribution/provenance and the branch-wide feature/known-limitations audit are complete and mechanically validated; keep exact release-note issues synchronized when an RC3 artifact is drafted.
- PR #132 has been triaged read-only against current `v0.4-dev`; most bundled changes are superseded, and the remaining embedding/build cleanups should return only as focused successors after exact maintainer approval.
- Branch-specific public links and clone instructions must change atomically with [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md), not before the branch transition.
- The modern GPU graphics tutorial execution and validation plan is [VKLITE_GRAPHICS_TUTORIAL.md](VKLITE_GRAPHICS_TUTORIAL.md). RC3 owns its enabling API and three-chapter pilot; do not publish uncompiled prose sketches or duplicate the advanced raw-triangle example as a beginner chapter.

## Required for RC4

1. Complete the tutorial through textured, lit, mouse-rotatable Suzanne while preserving the one-concept-at-a-time learning contract.
2. Ship the Suzanne OBJ, deterministic texture, source or generation recipe, provenance, license, hashes, install rules, and exact binary-asset approval outside the `data` submodule.
3. Freeze the tutorial-facing API profile and publish exact Datoviz version compatibility with honest `advanced/unstable` vklite status.
4. Validate every chapter against exact installed packages, external shaders and assets, deterministic captures, bounded GLFW interaction, Vulkan validation, and supported hosted platforms.
5. Collect and resolve or record RC4 tutorial feedback before final v0.4.0.

## Optional Unless Promoted

- `wind_globe.c` showcase and a new hero screenshot.
- Static prompt widget and external assistant links.
- Pyodide live Python playground over the existing WASM scene module.
- Four-panel hero composition and new marketing-oriented screenshots.
- Broad visual-family screenshot expansion beyond the examples required to document supported release surfaces.

These projects may improve the final site, but they do not block RC3 unless the maintainer explicitly promotes one into the required gate.

## Final Documentation Gate

1. Publish the final feature table, known issues, platform limitations, install/build guidance, positioning notes, Python and WebGPU/WASM scope, and release notes.
2. Ensure public dataset examples include source links, licenses, citation guidance, provenance, and required permissions.
3. Generate final README, website, gallery, short-video, and announcement assets from current canonical examples.
4. Add the exact Zenodo version DOI, concept DOI, and release date to citation and announcement material after the final archive exists.
5. Submit the JOSS draft or record its explicit deferral.
6. Publish the release-pinned modern GPU graphics tutorial, final Suzanne captures, assets, provenance, compatibility statement, and known limitations after resolving or recording RC4 feedback.

## Validation

Documentation-only changes require `git diff --check`, `just docs-build-check`, `just docs-status-check`, and inspection of `git status --short` unless a narrower repository recipe is explicitly sufficient.

Generated references, example pages, screenshots, animations, or inventories require their focused generator and checker plus the strict documentation gates. Gallery work additionally uses `just check-example-manifests`, `just check-gallery-media-pipeline`, and `just check-gallery-media-freshness` as applicable.

Never hard-wrap Markdown prose; keep each paragraph and list item on one source line.
