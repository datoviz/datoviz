# Gallery Media Single-Resolution and Parallelism Plan

Status: approved planning record; implementation requires separate approval and no implementation changes are included in this checkpoint.

## Goal

Simplify animated gallery media to one `1280x720` spatial resolution, preserve smooth motion whenever the existing 1 MB MP4 card budget allows, make size fallback deterministic and explicit, and reduce end-to-end generation time through bounded parallel execution.

## Current State

- Canonical animation capture already defaults to `1280x720`, but gallery card generation resizes frames to `1024x576` with one ImageMagick invocation per frame.
- MP4 generation performs one configured encode and may perform one FPS fallback, but it has no bounded compression ladder and may leave an oversized result for later reporting.
- The manifest contains redundant card-size overrides and per-example `fallback_fps` fields.
- Fifteen MP4-card examples have an explicit 30 fps canonical capture rate; these are legitimate lower-rate exceptions and must not be upsampled.
- Gallery freshness validation detects stale frames, candidates, and deployed assets, but does not fully validate encoded resolution or the consistency of FPS, sampling, frame count, and duration metadata.

## Media Policy

- Use `1280x720` end to end for animated gallery capture, MP4 cards, animated WebP outputs, and posters.
- Encode directly from canonical frames and remove the per-frame ImageMagick resize phase.
- Produce each poster directly from a canonical `1280x720` frame.
- For canonical captures at 60 fps or higher, first encode MP4 at 60 fps using the configured base CRF.
- If the 60 fps MP4 exceeds 1 MB, encode at 30 fps by sampling frames consistently while preserving animation duration.
- If the initial 30 fps result exceeds 1 MB, increase CRF from the configured base in increments of four, capped at CRF 40, with duplicate rungs removed.
- Fail explicitly if the CRF 40 result remains over budget; never reduce spatial resolution silently.
- Do not upsample a canonical capture rate below 60 fps; begin at its explicit native rate and apply only the bounded CRF ladder if required.
- Keep FPS, sampling step, encoded frame count, reported duration, and actual duration internally consistent.
- Apply this policy to every MP4 gallery card without an example-specific implementation path.

## Checkpoint 1: Resolution, Encoding, and Freshness

1. Centralize the canonical `1280x720` animation size in `tools/gallery_media.py` and remove the separate `1024x576` card-size policy.
2. Refactor `tools/compare_gallery_media.py` so MP4, animated WebP, and poster generation consume canonical frames directly without requiring ImageMagick for card resizing.
3. Extract a pure, testable MP4 attempt planner and executor implementing the ordered FPS and CRF ladder.
4. Preserve the fifteen existing justified 30 fps examples as explicit manifest rates, remove redundant size and `fallback_fps` configuration, and reject inconsistent FPS or sampling metadata.
5. Share manifest-policy validation between generation and `tools/check_example_manifests.py` rather than duplicating rules.
6. Extend the gallery freshness gate to validate encoded dimensions and policy-consistent report metadata in addition to stale frame caches, card candidates, and site assets.
7. Update `agents/rules/BUILD_TEST.md` and relevant command help to document the single-resolution policy, fallback ladder, native-rate exceptions, and explicit failure behavior.
8. Add focused tests using mocks or small synthetic sequences; do not regenerate the complete gallery merely to test control flow.

Required focused cases are a 60 fps encode that fits, fallback from 60 to 30 fps, duration preservation, bounded CRF fallback at 30 fps, explicit impossible-budget failure, `1280x720` MP4 and poster dimensions, absence of the resize stage, rejection of inconsistent manifest metadata, and freshness validation under the new policy.

## Checkpoint 2: Bounded Parallel Generation

1. Add `--jobs N` for independent encoding and conversion work, with an automatic CPU-bounded default capped at four workers and an explicit serial mode.
2. Keep the fallback attempts for one animation sequential because each attempt depends on the preceding encoded size, while allowing separate animations to encode concurrently.
3. Add a separate `--capture-jobs N` option for independent animation capture, defaulting to one on macOS because concurrent Vulkan and GLFW processes may contend for GPU and window-system resources.
4. Give every example an isolated temporary workspace so parallel workers cannot collide on frames, candidates, posters, or intermediate files.
5. Preserve deterministic report and manifest ordering regardless of worker completion order.
6. Aggregate worker failures, stop scheduling new work after a fatal failure, clean up safely, and never accept an incomplete output set as successful.
7. Add focused tests for worker bounds, serial operation, deterministic output, failure propagation, cancellation behavior, and temporary-directory isolation.
8. Benchmark representative serial and parallel runs before finalizing defaults, and record the measured command, workload, worker count, wall time, and machine context.

## Validation

Run narrow tests first, followed by the complete gallery-media tooling suite, manifest validation, gallery pipeline and freshness checks, strict documentation validation when recipes or documentation change, and `git diff --check`.

The expected validation commands include `python3 -m pytest` for focused test files, `python3 -m pytest tools/tests`, `just check-gallery-media-pipeline`, `just check-gallery-media-freshness`, `just check-example-manifests`, `just docs-build-check`, and `just docs-status-check`; adapt only when repository recipes establish a more precise equivalent.

## Commit and Publication Boundaries

- Prepare one focused local implementation commit for Checkpoint 1 and a second focused local implementation commit for Checkpoint 2 after their respective checks pass.
- Before each commit, inspect `git status --short` and `git diff --cached --stat`, and stage only intended source, test, manifest, recipe, and documentation files.
- Never touch, stage, or commit `paper/paper.pdf`.
- Never modify, stage, commit, or update the `data` submodule without explicit approval in the current turn.
- Keep generated build-local media ignored and uncommitted.
- Do not push implementation commits, regenerate the published gallery, or deploy the website without later explicit approval of the exact commits and actions.

## Completion Criteria

The work is complete when all animated gallery outputs use canonical `1280x720` frames without a resize stage, MP4 generation follows the bounded and duration-preserving FPS/CRF policy, lower-rate exceptions remain explicit, freshness and manifest checks enforce the policy, serial and bounded-parallel modes are deterministic and tested, relevant validation is green, and the two implementation checkpoints are committed locally with no unrelated or prohibited files included.
