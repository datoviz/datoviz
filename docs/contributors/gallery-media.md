# Gallery Media Pipeline

Run commands from the repository root. For the contribution workflow and promotion approval requirements, read [Adding a public example](adding-examples.md). Canonical screenshot origin and comparison policy lives in the repository at `spec/release/GALLERY_REFERENCE_SCREENSHOTS.md`.

## Capture And Encoding

Static gallery screenshots are captured as PNGs under `data/gallery/v0.4/` and converted to build-local WebP derivatives under `build/gallery-webp/v0.4/`. Do not stage or commit `data` submodule media or generated WebP files unless the user explicitly approves those exact payloads in the current turn.

Use `just gallery-refresh` for a full local media/docs refresh. It must capture PNG screenshots, convert static WebPs, regenerate animated WebPs, rebuild generated gallery docs/manifests, run the gallery media checks, and run `git diff --check`. Use `just check-gallery-media-pipeline` after changing gallery media tooling or manifest preview metadata.

When canonical PNGs exist but the local screenshot cache is absent, use `python3 tools/capture_gallery.py --all-screenshot --cache --verify-existing --jobs auto`. Verification captures into an isolated temporary tree, leaves `data` untouched, and writes a local cache record only when the normalized recapture is byte-identical or differs by no more than eight channel levels across at most 0.1% of RGBA components. Larger differences remain failures requiring explicit review and approval before any `data` change.

Canonical v0.4 native gallery PNGs use the designated physical Linux reference host defined in `spec/release/GALLERY_REFERENCE_SCREENSHOTS.md`. Use `just gallery-reference-candidates` to produce two isolated build-local capture sets, provenance, enhanced differences, and a review index. The two Linux runs must be byte-identical before promotion. Cross-platform or cross-driver pixel equivalence is comparison evidence, not canonical-origin proof.

Animated gallery captures, animated WebP previews, MP4 cards, and posters use canonical `1280x720` frames end to end. The media comparison pipeline consumes the canonical frame cache directly and must not add a per-frame resize stage.

Animated gallery previews use manifest metadata:

```yaml
media:
  preview:
    kind: animated-webp
    frames: 60
    fps: 30
```

MP4 gallery cards preserve their explicit native capture rate and are never upsampled. A capture at 60 fps first encodes at 60 fps with the configured base CRF; if it exceeds the 1 MB budget, the pipeline samples consistently to 30 fps while preserving duration. At 30 fps or a lower native rate, an oversized result advances through a bounded CRF ladder in increments of four through CRF 40. The command fails if the CRF 40 result remains oversized; it never silently reduces spatial resolution. Keep `preview.fps` equal to `card.fps * card.sample_step` so encoding never changes the preview duration.

`tools/build_gallery_animations.py` and `tools/compare_gallery_media.py` accept `--jobs N` for independent encoding work and `--capture-jobs N` for independent captures. Automatic worker counts are CPU-bounded and capped at four; capture defaults to one worker on macOS. Use `--jobs 1 --capture-jobs 1` for explicit serial execution. Attempts for one MP4 remain sequential, each example uses an isolated temporary workspace where needed, and reports retain manifest order regardless of worker completion order.

Gallery card and poster encodes use content-addressed cache records under `build/gallery-cache/cards/`. Cache keys cover canonical frame content, the encoding profile, generated variants, implementation inputs, and encoder identities; cache hits also verify every output hash. Use `--force` only when intentionally rebuilding verified current outputs.

Animated-frame cache invalidation is entry-scoped: only capture-relevant metadata from the selected manifest entry participates in its fingerprint. Editorial or unrelated-example manifest changes must not invalidate every cached frame sequence.

Generate selected previews with:

```sh
python3 tools/build_gallery_animations.py --id <example_id> --force
```

The static WebP converter must not overwrite examples with `kind: animated-webp`; those WebP paths are owned by `tools/build_gallery_animations.py`. The animation tool captures a deterministic PNG sequence by launching the example with `--preview`, `--preview-sequence`, `--preview-frames M`, and `--png`, then encodes the PNG sequence with `img2webp`. For animated examples whose motion depends on elapsed simulation state rather than a controller preview descriptor, add an explicit preview-mode path that derives deterministic state from `ctx->preview_frame_index` and `ctx->preview_frame_count`; otherwise every captured frame may start from the same initial state and produce a static animated WebP.

The standalone animation generator builds only WebP-owned public previews by default. Entries whose preferred card is `video-mp4` are generated by the comparison/card pipeline and must not be regenerated immediately before that pipeline removes their site WebP. An explicit `--id` selection or `--include-video-previews` retains animated-WebP candidate generation for comparison and debugging.

## Homepage Presentation Profile

The four homepage examples declare a named `media.profiles.homepage` candidate capture profile, independent of the normal gallery profile. Presentation motion uses output playback time without rescaling scientific data time. Native capture remains at 60 FPS; encoding can sample at 30 FPS while preserving duration. `required_inputs` must exist before capture, preventing the full-density point cloud from silently loading a smaller fallback.

Generate new candidates explicitly with `python3 tools/compare_gallery_media.py --profile homepage --site-video-previews --write-site-assets`. Configure `SVT_AV1_ENC` or supply `SvtAv1EncApp` on PATH for AV1 candidates. The point-cloud AV1 recipe pins SVT-AV1 4.3.0, preset 3, tune 4, CRF 56 and 30 FPS, with film-grain synthesis and temporal filtering disabled. Its compatibility stream uses two-pass H.264 High/3.1 with four references and three B-frames. The planet uses the same AV1 encoder/tuning with CRF 46.5 and a 2 MB budget; quarter-step CRF values are supported. Its H.264 fallback uses fixed CRF 24, the veryslow preset and a 4 MB budget, consuming the same uncompressed 10-bit video input as AV1. Fixed-quality fallback encoding fails rather than increasing compression to fit. Budgets, sampling, encoder identity and variant hashes are recorded independently. CRF numbers across codecs are not comparable.

The normal documentation workflow consumes the hash-locked snapshot declared by `homepage_media.selection` in the example manifest. Materialize its exact files under `build/homepage-review/selected-homepage-media/`; `python3 tools/homepage_media.py` verifies them. `just docs-video-assets` and `just serve` hydrate both normal-gallery and selected-homepage namespaces into one availability manifest. Homepage hydration copies accepted bytes without recapturing or re-encoding. Missing or substituted selected media must not become approved implicitly; restore the reviewed bundle or retain the last good preview/posters.

`tools/docs_asset_stage.py --homepage-products build/homepage-review/selected-homepage-media` stages both AV1 primaries and their H.264 compatibility streams. Publish stages require the complete verified snapshot; hermetic builds use generated placeholders without requiring media or encoder binaries. `python3 tools/check_gallery_media_freshness.py --profile homepage` validates selected hashes and video timing. Use the explicit comparison tools for candidate freshness and encoding experiments.

## Homepage Release Delivery

The selected homepage media are prepared as GitHub Release attachments in the existing website repository. They are served through versioned release URLs rather than committed as website Git blobs. This delivery change does not deploy the website, create a repository, migrate datasets or remove the data submodule.

Verify the selected local files and prepare the exact flat attachment set from a committed producer revision:

```sh
python3 tools/homepage_media.py
python3 tools/homepage_media.py --producer-commit "$(git rev-parse HEAD)" --release-dir build/homepage-review/release-attachments
```

The output contains ten media files, `homepage-media.json` and `SHA256SUMS`. The manifest records the producer source commit, capture-frame identities, video dimensions/timing, exact file hashes and versioned public URLs. Preparation verifies the entire source set before writing, never encodes or uploads anything, and refuses to replace a different existing draft. Use a new output directory after changing the producer revision. Review receipts and publication decisions remain separate from technical media identity.

`python3 tools/homepage_media.py --package build/homepage-review/publication/homepage-media.tar.gz` also creates a deterministic local backup archive. Its technical descriptor excludes hosting and approval state, so changing those does not change the approved media bundle. The archive is not required as another public attachment.

Create, upload and publish a media release only after explicit approval of its repository, tag, notes and exact attachment list. Use a draft for upload/re-download verification, publish as a prerelease with latest disabled, then verify every public file anonymously:

```sh
python3 tools/homepage_media.py --verify-public
```

The public verifier checks bounded downloads against every selected byte count and SHA-256. The guarded website publisher requires both final website approval and successful public verification before preparing deployment. Publishing media attachments and publishing the documentation website are separate operations.

`DATOVIZ_HOMEPAGE_ASSET_BASE_URL` selects the versioned delivery origin. In that mode homepage URLs are rewritten and their binary subtree is excluded from website Git output. A private WIP preview can use a verified private mirror while the proposed public release awaits approval; this demonstrates the same URL layout without uploading unpublished files. Ordinary local and hermetic builds retain their explicit local staging behavior. Mirror checks do not establish availability of a future public release.
