# Datoviz Gallery Visual Identity Style Guide (Draft)

> Status: draft for v0.4-dev examples/gallery work.

## 1) Purpose

This guide defines a coherent visual identity for Datoviz examples and gallery pages so that:

- examples feel like one product (not disconnected demos),
- screenshots communicate Datoviz strengths (speed, interactivity, scale),
- contributors have clear defaults for color, typography, layout, and interaction cues.

## 2) Design principles

Use these principles as tie-breakers when aesthetics choices conflict:

1. **Data-first clarity**: visual styling should improve interpretation, not decoration.
2. **Performance confidence**: visuals should look responsive and technically robust.
3. **Scientific neutrality**: avoid branding that distorts quantitative reading.
4. **Consistency across families**: point/mesh/image/volume examples should feel related.
5. **Dual-context readability**: gallery assets should hold up on both dark and light UIs.

## 3) Gallery structure and visual role

Map style effort to existing gallery categories:

- **Showcase**: strongest cinematic polish and narrative framing.
- **Visuals**: controlled baseline style for direct visual-family comparison.
- **Features**: emphasize interaction affordances and state transitions.

Public website and MkDocs gallery implementation notes live in
[`../../docs/GALLERY_SITE.md`](../../docs/GALLERY_SITE.md). This file owns visual style and capture
defaults for examples and gallery media.

## 4) Recommended visual direction

Recommended identity direction: **Scientific Cinematic Minimalism**.

- restrained composition and annotation,
- high local contrast on data marks,
- subtle depth cues in 3D,
- quick, smooth interactions,
- minimal ornamental styling.

## 5) Color system proposals

Choose one of the options below as the default. Other options can remain as alternates.

### Option A (Recommended): Graphite + Cyan Accent

Best for interactive/gallery hero screenshots and GPU-forward look.

- Background (dark): `#0E1117`
- Background elevated panel: `#161B22`
- Grid/axes muted: `#30363D`
- Text/annotation light: `#C9D1D9`
- Primary accent (interactive): `#4CC9F0`
- Secondary accent (highlight): `#80FFDB`
- Warning/accent warm: `#FFB703`
- Error/selection conflict: `#EF476F`

Use case:

- default for 3D mesh/volume and high-density point scenes,
- hover glow or probe marker in cyan,
- selected entity ring in warm accent.

### Option B: Warm Light Scientific

Best for publication-adjacent examples and educational screenshots.

- Background (light): `#F8FAFC`
- Panel surface: `#FFFFFF`
- Grid/axes muted: `#CBD5E1`
- Text/annotation dark: `#0F172A`
- Primary accent: `#2563EB`
- Secondary accent: `#0EA5A4`
- Warning: `#D97706`
- Error: `#DC2626`

Use case:

- examples meant to mirror "paper figure" expectations,
- axis-heavy 2D visuals and annotation-heavy demos.

### Option C: Night Neon (Experimental showcase only)

Use sparingly for showcase cards, not baseline visuals.

- Background: `#080A0F`
- Primary accent: `#7C3AED`
- Secondary accent: `#06B6D4`
- Emphasis: `#F43F5E`

Rule: do not use for calibration/reference visuals where neutral reading matters.

## 6) Colormap policy

Datoviz has many colormaps; gallery identity should still be curated.

- Default continuous scalar map: perceptually uniform (`viridis`-class) for quantitative meaning.
- Diverging map only when data has meaningful center/zero crossing.
- Categorical data: small fixed palette (max ~8 hues before shape/texture reinforcement).
- Avoid rainbow defaults in baseline examples unless teaching colormap pitfalls explicitly.

## 7) Typography and labeling recommendations

### Font stack

For docs/gallery web UI:

- Primary UI/font: `Inter, system-ui, -apple-system, Segoe UI, Roboto, sans-serif`
- Monospace code/data overlays: `JetBrains Mono, SFMono-Regular, Consolas, monospace`

For in-render text (when text visuals are used), target neutral sans rendering with medium weight.

### Type scale (gallery page)

- Card title: 16-18 px semibold
- Card subtitle/description: 13-14 px regular
- Inline metadata/tags: 12 px medium
- Figure annotations: minimum equivalent of 11 px at capture resolution

### Label style

- Keep labels short and noun-first.
- Prefer sentence case over all caps.
- Limit punctuation and decorative glyphs in-canvas.

## 8) Interaction aesthetics

To reinforce Datoviz's interactive identity, define states explicitly:

- **Default**: stable, low visual noise.
- **Hover**: +10-20% brightness or outline, <=120 ms transition.
- **Selected**: persistent contrast shift + shape/halo cue.
- **Probe/Pick feedback**: fixed semantic color and marker shape across examples.
- **Animated transitions**: short and purposeful; avoid easing that hides data motion.

## 9) Composition rules for screenshots

Baseline rules for captures:

1. Keep a consistent aspect ratio per category (e.g., 16:10 for showcase, 4:3 for visuals).
2. Ensure subject occupies 60-85% of frame area.
3. Keep margin breathing room around axes and annotations.
4. Avoid clipped legends/labels.
5. Prefer deterministic random seeds for reproducibility.

Extended screenshot rules:

1. Treat every gallery screenshot as a documentation artifact, not an incidental frame.
2. Use a consistent capture shape:
   - showcase: prefer 16:10 or 16:9, for example `1600x1000` or `1600x900`;
   - visual-family baselines: prefer `1280x960` or another fixed comparison size;
   - thumbnails: crop from the canonical image rather than rendering a separate composition unless
     the crop fails.
3. Use dark neutral backgrounds by default, with light-background variants only for
   publication-adjacent 2D examples.
4. Use real or realistic scientific data for showcases. Tiny synthetic data is fine for fixtures,
   but it should not define the public gallery.
5. Use deterministic camera poses, data ranges, and style defaults.
6. Prefer a small recurring colormap set. Avoid rainbow defaults except in explicit colormap
   comparison or legacy-parity examples.
7. Capture one clear interaction state when relevant: default, hover, selected, probe, or animated
   frame.
8. Avoid visible GUI chrome unless the example is specifically about GUI controls or hosted
   integration.
9. Verify every selected gallery image is nonblank and readable at thumbnail size.


## 9.1) Video rules

Short videos should demonstrate interaction, scale, or rendering quality without becoming demos of
manual camera operation.

1. Keep videos short: 5-20 seconds.
2. Make them loopable when possible.
3. Show one visual idea per video.
4. Use smooth camera motion and deterministic animation paths.
5. Avoid frantic mouse interaction; scripted movement is preferred for documentation assets.
6. Keep captions minimal and factual, for example `10M points`, `image probe`, `EDL on/off`,
   `volume slice`, `textured mesh`, or `WebGPU subset`.
7. Good v0.4 video archetypes:
   - orbit around protein, brain, sphere cloud, or textured terrain;
   - fly through LiDAR with EDL/depth cueing;
   - pan/zoom a scalar field and update a probe readout;
   - sweep a volume slice or transfer range;
   - animate a weather/vector field if the vector example lands;
   - show a bounded high-density signal or CPU fluid/particle update loop.
8. If video export is backend-dependent, the gallery should show the result but the docs should
   state the backend requirement clearly.

## 10) Category-specific styling presets

### Showcase preset

- Use Option A or C palette,
- more depth/postprocess allowed,
- stronger composition narrative,
- include one signature interaction frame if available.

### Visuals preset

- Use Option A or B only,
- fixed camera, fixed lighting, fixed neutral background,
- remove decorative extras for comparability.

### Features preset

- Keep style neutral,
- emphasize before/after or state A/state B differences,
- add concise callout for feature under test.

## 11) Accessibility and robustness guardrails

- Target WCAG-like contrast for text overlays on backgrounds.
- Do not encode critical distinctions with color alone; add size/shape/line-style differences.
- Check readability in color-vision-deficiency simulation when feasible.
- Keep thin lines >=1.5 px in gallery captures to survive compression.

## 12) Suggested near-term decisions (recommended)

To unblock contributions immediately, decide the following now:

1. **Default palette**: adopt Option A (Graphite + Cyan Accent).
2. **Alternate palette**: Option B for publication-adjacent examples.
3. **Typography**: Inter + JetBrains Mono stack on gallery/docs pages.
4. **Interaction semantics**:
   - hover = cyan,
   - selected = amber,
   - error/conflict = rose.
5. **Screenshot format**:
   - baseline visuals: 1280x960,
   - showcase: 1600x1000,
   - PNG with deterministic seed and fixed camera when possible.

## 13) Contributor checklist (pass/fail)

Before adding or updating an example screenshot:

- [ ] Category preset selected (showcase/visuals/features).
- [ ] Palette selected and documented in example metadata/notes.
- [ ] Contrast and legibility checked at card-thumbnail size.
- [ ] Interaction state (if relevant) intentionally captured.
- [ ] Seed/camera reproducibility notes recorded.
- [ ] Screenshot file present and non-blank.

## 14) Optional metadata extensions (future)

If we extend `tools/build_gallery.py` metadata later, add optional fields:

- `style_preset`: `showcase|visuals|features`
- `palette`: `graphite_cyan|warm_light|night_neon`
- `capture`: resolution/aspect tuple
- `seed`: deterministic seed used for capture
- `interaction_state`: `default|hover|selected|probe`

These fields would enable filtering and consistent labeling in generated gallery pages.
