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

The four homepage videos use a named `media.profiles.homepage` capture profile. The default gallery preview remains independent. Select the profile with `python3 tools/compare_gallery_media.py --profile homepage --site-video-previews --write-site-assets`. Capture frames, caches, encodes, reports and site products live under `build/homepage-review/products/`; the homepage URLs use `assets/gallery/v0.4/profiles/homepage/`. Generated media stays untracked and requires manual approval before external publication.

Presentation motion is separate from scientific data playback. A bounded sinusoidal sway uses output playback time, preserves each example's authored axis or camera pivot, and samples the periodic path without duplicating its endpoint. A static presentation leaves scientific activity running at its established demonstration rate. The default example behavior is preserved when no presentation override is selected. A closed presentation path alone does not prove a seamless data loop: check the scientific animation separately, especially planetary trajectories.

Capture at the existing native 60 FPS; the existing encoding ladder may downsample to 30 FPS while preserving duration. Keep canonical 1280×720 dimensions. A named profile may explicitly set `card.mp4_crf`, `card.mp4_max_crf` and `card.video_budget_bytes` to protect detail; an oversized result must fail when it reaches that quality ceiling. The revised full-density homepage point cloud allows up to 20 MB at CRF 28 for its high-detail review candidate; a smaller separately labeled encode is also reviewed before selecting publication settings; other profiles retain the default 1 MB budget. `required_inputs` names relative files that must exist before capture, preventing a missing full-density cache from silently selecting a smaller fallback. Profile settings, capture inputs, renderer identity and encoding inputs participate in freshness checks. Use `python3 tools/check_gallery_media_freshness.py --profile homepage` for the selected products.

The standard `just docs-video-assets` and `just serve` workflows hydrate both gallery and homepage namespaces into one local availability inventory. The guarded publish recipe generates and validates the profile before staging. Explicit hydration stage and manifest paths are honored.

Supply the verified profile subtree with `tools/docs_asset_stage.py --homepage-products build/homepage-review/products/assets/profiles/homepage` alongside the normal gallery products. Publish stages require homepage videos and posters; local stages can show existing gallery posters until the profile is generated, while hermetic builds use generated stand-ins. Do not put experimental profile products into the normal watched gallery directory. Review generation and local preview do not approve canonical promotion or public deployment.
