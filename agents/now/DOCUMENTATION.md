# Datoviz v0.4 Documentation Status

Status: complete 16-chapter course implementation, the 2026-11-17 explanatory pass and approved homepage media publication reconciled on 2026-11-18; maintainer course acceptance, revised-course validation, exact release artifacts and website publication decisions remain. Earlier automated reviews retain their recorded scope. Scientific technique scope updated: 2026-10-13; earlier prose, PR, and gallery evidence retains its recorded identities.

Use [RELEASE.md](RELEASE.md) for sequencing, [STATUS.md](STATUS.md) for repo-wide blockers, and `spec/docs/` plus `spec/release/` for durable policy. This file is the sole active RC3 documentation inventory.

## Private Review Entry Points

The Datoviz category on the private gateway index is the front door for three distinct workflows; the older `/datoviz-workspace/` URL remains available for compatibility. WIP documentation (`/datoviz-docs/`) is rebuilt automatically from the working tree and keeps the last successful build on failure. The campaign dashboard (`/datoviz/`) retains its independent application workflow. Temporary visual reviews (`/datoviz-review/`) hold dated screenshot/video comparisons and exact proposed asset hashes. No ad hoc preview port or separate certificate is needed. Agents should edit the source, run required checks, and let the managed preview update; viewing a candidate does not approve publication.

## Completed RC3 Documentation Work

- The public v0.4 structure, navigation, status vocabulary, Known limitations page, and preserved `/v0.3/` route are implemented.
- Generated C reference covers 1,643 classified functions across 13 pages with drift checking at `4f39d031e`; the input-router page was regenerated from current headers.
- Python guidance covers all 18 NumPy-adapted calls, three generated helpers, exact `datoviz.raw` use, ownership, callbacks, and the GSP/VisPy2 boundary.
- All 12 real or prepared dataset showcases pass attribution and provenance checks.
- Gallery tooling enforces canonical `1280x720` animation media, bounded FPS/CRF fallback, encoded-output validation, isolated deterministic parallelism, and capture-worker limits.
- All 38 animation frame caches were regenerated under the prepared-data-aware key, all 38 animation candidates are current, and all 29 MP4 card candidates plus posters pass their budgets; publication remains an exact-approval action.
- The nine invalidated still-cache records were regenerated and verified against the canonical images: eight are byte-identical and `features_panel_mixed_2d_3d` is pixel-equivalent with maximum channel delta 4 across 0.0911% of components.
- The designated Linux host produced two byte-identical 104-image screenshot runs; the approved 54 changed images were promoted through `data` commit `d72c72c` and parent gitlink commit `264517633` with machine-readable evidence.
- The canonical histogram is published through `data` commit `6a9091c10e08e477af5b07496cac602aefc7c999` and parent commit `9b0884f507f76f5a982db85cd2f6e59f094512e1`; all 107 reviewed stills pass full-pixel validation, and the example has C, Python, and live WebGPU routes.
- The four-page visual-system pilot passes strict build and software-rendered desktop/mobile inspection.
- PR #157 merged the refreshed embedding and platform-portability work at `fccf83061b05b7373f5caa1ba40bdecfc3578306` with green Linux, macOS, Windows, agent-instruction, and submodule-reachability checks. PR #136 is merged and superseded by #157; original PR #132 is closed as superseded.
- All 16 substantive course chapters and epilogue 17 are implemented, including uniform buffers/descriptors and the generated textured, lit mesh. Current source synchronization verifies 178 excerpts, in-tree smoke passes all 16 steps with reproducible captures, and the documentation build generates 18 media outputs. Earlier installed-source and independent-reader evidence retains its original revision/chapter scope.
- Rewritten course previews are generated from the canonical programs with real stdout, exact flat-color validation, deterministic fixed-time animation, and no `data` dependency.
- The 2026-09-08 automated course prose review covers the overview and chapters 1-3. Setup now gives corrected Windows Developer PowerShell, Release-configuration source-install, package DLL `PATH`, and configuration-specific executable guidance; maintainer review of voice and pacing remains required.

The two bounded Fractal prose batches (`c4adddfd8` and `6d4affab6`) clarify twelve authored guide/reference pages, preserving code blocks, link targets, and technical qualifications. Strict documentation builds and status checks pass; [campaign evidence](FRACTAL_CODE_HARDENING.md) records their scope. The later course explanatory and consistency passes are recorded in [the course queue](VKLITE_GRAPHICS_TUTORIAL.md); further automated review does not replace maintainer voice and pacing acceptance. Broad visual rollout remains pending.

The scientific-profile correction at `d0e904705` updates native technique coverage, known limitations, feature status, and point/mesh specifications. Unsupported family/alpha/producer combinations, mask cutoff, item-state/instancing exclusions, and native/WGSL differences are explicit; the [profile handoff](SCIENTIFIC_3D_PROFILE_HANDOFF.md) retains implementation and GPU regression evidence.

## Current Gallery Reference Review, 2026-11-16

[The complete review](GALLERY_REFERENCE_REVIEW.md) classifies all 69 changed screenshots against the protected references: 64 are consistent with intentional font, lighting, rendering, GUI or showcase changes; two polygon cases were corrected and revalidated in the [2026-11-17 implementation](GALLERY_REFERENCE_REVIEW.md#polygon-paint-implementation-2026-11-17); three small raster residuals retain a matched-provider/explicit-disposition gate. Current generation produced 107 nonblank, repeatable PNGs, 38 changing motion previews and 29 budget-compliant MP4 cards/posters. No reference image or `data` gitlink was promoted by this review.

## Remaining RC3 Documentation Work

1. Confirm the four-page visual pilot scope before any broad rollout; this is distinct from the accepted homepage videos and completed gallery comparison. Do not repeat accepted reviews merely because their historical handoff still says pending.
2. Proofread the 2026-11-18 corrected course after the completed Claude Opus 5.5 medium review, independent source check and automated validation (see [the course queue](VKLITE_GRAPHICS_TUTORIAL.md)); the complete course ships in RC3. Maintainer acceptance of voice, pacing, API profile, ownership explanations, package-first instructions and previews remains separate.
3. Preserve the accepted polygon rendering direction and completed comparison; canonical polygon/choropleth images have not been promoted. Prepare the exact destination/hash and publication action for any remaining canonical replacements. For the three small raster residuals, tie any existing maintainer acceptance to the exact candidate provenance or obtain a scoped disposition; do not reopen the accepted benign general review. Keep RC3 unsupported paint queries and browser validation scope explicit; composed picking and device lifecycle are RC4 work.
4. Review the local RC3 release-notes draft; finalize its exact identity, validation evidence, and release-specific known issues only when artifact scope is fixed.
5. Review exact outreach drafts before any dataset-author or public GitHub communication.

The 2026-10-11 Qt/PyQt documentation draft corrects the completed upstream publication state, documents the indexed local-channel install and managed Vulkan ownership, and records the Windows trust limitation. [Provider promotion gates](QT_MACOS_VULKAN_HANDOFF.md#proposed-rc3-provider-scope-2026-10-11) keep RC4 as the default allocation and require an explicit decision plus exact-platform proof for any RC3 promotion; provider promotion must not block base RC3. If promoted, review final channel, version, platform claims, and release-note wording after candidate proof.

## PR #132 Disposition

Per-image present semaphore, Kvazaar/PThreads4W, and `DVZ_LOG_LEVEL` work were integrated independently. The current vendored Vulkan-header fallback does not have a reproduced defect. The broad developer-preset overhaul remains deferred rather than entering RC3.

The contributor reviewed PR #136 and requested namespaced shader-tool settings and accurate no-compiler behavior. PR #157 incorporated those follow-ups together with parent-owned GLFW reuse, the guarded Windows lean-header policy, opt-in macOS Vulkan-environment sanitization, removal of the obsolete Canvas swizzle-shader pipeline, deterministic compiler-discovery controls, FetchContent and no-compiler regressions, and Windows CI hardening. It merged on 2026-09-17 at `fccf83061b05b7373f5caa1ba40bdecfc3578306` after all hosted checks passed. PR #136 is merged and its final comment records #157 as the refreshed successor. PR #132 was closed as superseded on 2026-09-17 after the approved final comment was published.

## RC3 Paint And Browser Scope

RC3 ships the implemented unlit ordered polygon rendering. Public query guidance must retain the conservative panel-wide unsupported result while drawable paint is present, including its zero-coverage exceptions. The bounded browser investigation obtained strict headless and headed/Xvfb rendering proof for polygon and choropleth with the explicit Vulkan profile; any persistent provider-only failure must be an exact, narrowly scoped browser-validation exclusion, not a pass or a waiver of native/Python and exact-artifact validation. Keep release notes and compatibility wording aligned with [the evidence](GALLERY_REFERENCE_REVIEW.md) and the [RC4 allocation](../../spec/scene/slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md).

## RC4 Documentation Gate

Resolve RC3 course feedback and repeat the isolated-reader audit after the RC3 revision pass, regenerate previews from exact candidate code, freeze the tutorial-facing API and the compatibility profile for advanced low-level interfaces that may evolve during the release candidates, validate every chapter against exact installed artifacts on supported hosted platforms, and resolve or record reader feedback. Development implementation and preview generation do not close these acceptance gates.

The required course uses generated geometry and a procedural texture. Suzanne and committed binary course assets are optional polish and must not become release blockers.

## Final Documentation Gate

Publish final feature status, known issues, platform limitations, installation/build guidance, Python and WebGPU scope, release notes, release-pinned course, final media, and announcement assets. Add exact Zenodo DOI/date metadata after archiving and submit or explicitly defer JOSS.

## Validation

Follow [documentation validation](../rules/DOCUMENTATION.md) for public documentation changes. The release and maintainer-review gates above apply when work affects release scope or readiness.

## Homepage Motion Review

The maintainer selected all four primary homepage animations: gentle protein sway, a stronger full-density six-million-point cloud path, gentle cortex rotation with unchanged activity timing, and the moving planet with its accepted loop discontinuity. Default gallery profiles remain independent. The first cloud proposal accidentally used a smaller fallback dataset; the homepage profile now requires the full input and fails if it is missing.

The selected point-cloud AV1 is 4,713,781 bytes, 1280×720, 30 FPS and 9.6 seconds, SHA-256 `5e7e15bfd2b7f274becaea5fcf49545576566ab5f4814f814fb017087f6db61a`. Its H.264 High/3.1 compatibility fallback is 3,537,225 bytes. Both preserve the approved six-million-point scene and camera path.

The maintainer reopened planet compression review because the old 1 MB budget had forced H.264 to CRF 38. The approved replacement is 1,960,895-byte AV1, 1280×720, 30 FPS and 14.4 seconds, SHA-256 `3b0dfac76379861611ade742eb59fe38795a1fee895710037f3ed46056bb505c`. The fixed recipe uses SVT-AV1 4.3.0, preset 3, tune 4, CRF 46.5, with film-grain synthesis and temporal filtering disabled. Its 3,562,063-byte H.264 High/3.1 fallback uses fixed CRF 24, the veryslow preset and the same uncompressed 10-bit video input. The production encoder functions reproduce both selected planet files byte-for-byte. Protein, point-cloud assets, cortex and all posters remain unchanged. The maintainer accepted the planet H.264 fallback after the real public-release pilot. The maintainer subsequently accepted the point-cloud H.264 fallback, the complete private visual review and the videos in the WIP documentation. All ten selected media files are approved.

[The selected snapshot](../../spec/docs/homepage-media.json) pins all ten video/poster files. Standard staging and hydration verify and copy accepted bytes without recapture or re-encoding. The homepage offers AV1 first for cloud and planet, H.264 when unsupported, viewport-aware playback and static reduced-motion/data-saving behavior. The current deterministic local package has SHA-256 `79bd239426ea54e45e49e8db3b7ae81709105d453aadd2e5478f122272cded7a`; it supersedes the earlier nine-file package.

RC3 preparation now uses versioned GitHub Release attachments in the existing website repository, outside Git binary history. The two-video pilot is published and maintainer-approved, with all four exact pilot attachments verified after draft and public downloads. Its 18 Chromium/Firefox checks and six byte-range checks pass, including the full homepage, AV1, forced/explicit H.264, seeking, looping, viewport controls, reduced motion and data saving. The live homepage bytes, website main and Pages configuration remained unchanged. Firefox sometimes chose H.264 automatically despite successful explicit AV1 playback; retain that observation without inventing a cause. Safari/iPhone remain unvalidated.

[The complete selected snapshot](../../spec/docs/homepage-media.json) is published as the [homepage media release](https://github.com/datoviz/datoviz.github.io/releases/tag/homepage-media-ff1b026c8256), a prerelease that is not marked latest. Its twelve flat attachments contain ten exact media files, a producer/source/capture manifest and checksums. Authenticated draft and anonymous public downloads verify all twelve attachments. The published manifest records producer source `932190e034dcc691d30c9c31465b18aa2723e47a`; subsequent source preparation is integrated at `32bc89efc5e2f6de7a1b5971cc744b44c847e32a`. Review and publication bookkeeping changes do not change the technical snapshot or binary hashes.

The private WIP documentation now references the real public release URLs, rather than the private mirror. The homepage binary subtree is omitted from that preview. The earlier proposal to commit these binaries into website Git history is superseded. No new repository, data submodule update or dependency upgrade is required. The RC4 dependency assessment remains in [the dependency notes](../../spec/architecture/EXTERNAL_DEPENDENCIES.md#rc4-refresh-assessment-2026-11-17).

### Complete-set delivery evidence

The published set passes 19 Chromium/Firefox browser cases covering all six video streams, four public WebP posters, the compiled WIP homepage, automatic/forced H.264 selection, seeking, looping, mobile viewport, reduced motion and data saving. Eighteen HTTP byte-range responses cover the start, middle and end of each video and match the approved bytes. Browser fixtures supplied private HTML/CSS; all media/poster requests used real public GitHub/CDN responses without interception. Safari/iPhone and physical mobile devices remain untested. Earlier 184 focused tests, strict hermetic/mirror/public-URL documentation builds and 18 complete-set private-mirror cases remain preparation evidence, separate from the public delivery checks.

The deterministic local binary backup remains SHA-256 `79bd239426ea54e45e49e8db3b7ae81709105d453aadd2e5478f122272cded7a`; it was not uploaded. The deferred website draft uses the actual public v0.4 preview baseline and preserves the existing v0.3 archive. It references all ten public media URLs and excludes the homepage binary subtree; other gallery/WebGPU assets retain their current pipeline. Recheck the current website baseline before any eventual deployment.

Website deployment remains unapproved: `publication_authorized` is deliberately false and guards the website action, independently of the completed media-release approval. The live homepage bytes, website branch and Pages configuration were unchanged by media publication. Do not update datoviz.org during release preparation. RC3 is not frozen; obtain separate final website approval when the release is ready.

The hosted-media preparation also corrected localhost preview handling: the local availability inventory gates local files only, while external URLs retain their exact identity without a local cache-busting query. Focused browser policy checks cover hosted sources with an empty local inventory, missing-local poster fallback, playback controls, reduced motion/data saving, mobile layout and actual AV1 decode.
