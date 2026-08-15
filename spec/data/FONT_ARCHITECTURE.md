# Datoviz Font Architecture

> **Status:** implemented native consumer contract pending visual, installed-package, and legacy-cache proof
> **Scope:** scene text, rich text blocks, ImGui, embedded resources, custom fonts, glyph coverage, licensing, and deterministic generation

## Decision

Datoviz must provide useful, deterministic text without a network, an initialized `data` submodule, platform font discovery, or application configuration. The built-in family has five required face roles: sans regular, sans bold, sans italic, sans bold-italic, and mono regular. A sixth scientific fallback role supplies mathematical and technical codepoints absent from the selected primary face. Every required role remains available to ordinary library users and offline rich-text paths.

The approved long-term built-in family is the complete unmodified static Source Sans 3 regular, bold, italic, and bold-italic faces plus Source Code Pro regular, with unmodified Noto Sans Math regular as the scientific fallback. Exact files, upstream revisions, SHA-256 digests, sizes, OFL texts, font metadata, and coverage policy live under `assets/runtime/fonts/`. The native consumer switch, an isolated build with `data` absent, and source-bundle inclusion are validated. Scene/ImGui visual review, exact installed-package proof, and the legacy cached-atlas disposition remain required before release acceptance.

Do not subset the initial Source payload. Full upstream faces preserve their Reserved Font Names, coverage, provenance, and reproducibility. A later subset requires measured artifact or runtime benefit, replacement of every user-facing Reserved Font Name in the modified font, deterministic generation, exact license handling, and regression evidence.

## Implemented v0.4 Reality

The current implementation uses these mechanisms:

1. `DvzFontDefaults` selects Source Sans 3 and Source Code Pro without file paths;
2. one build-generated file-I/O resource embeds all six admitted parent-repository fonts when scene or GUI support is enabled;
3. ordinary retained scene text loads Source glyphs on demand and falls back per codepoint to Noto Sans Math in the MSDF, FreeType bitmap, and STB SDF atlas builders;
4. rich text resolves real Source regular, bold, italic, and bold-italic faces and uses Noto Sans Math for missing scientific codepoints;
5. ImGui uses Source Sans 3 regular and Source Code Pro regular, merges a bounded Noto Sans Math scientific range into its UI atlas, and still accepts configured sans and mono file paths;
6. callers may create a scene-owned `DvzFont` from `DvzFontDesc.path` and select it with `DvzTextStyle.font`;
7. `family`, `style`, and `font_flags` remain metadata and built-in-role selectors, not operating-system font discovery;
8. scene text and ImGui share immutable embedded bytes through file-I/O resources but retain independent atlas packing and GPU lifetime;
9. unsupported atlas codepoints still fall back visibly to `?`;
10. the tracked baked MSDF payload remains a legacy Roboto-only optimization and is not selected by the Source default; Source MSDF atlases are generated at runtime until that legacy product is removed or replaced through a separately approved generated-payload change.

Specifications and public documentation must distinguish this implemented behavior from later shaping, fallback-chain, and family-resolution targets.

## Ownership Model

The durable internal model is a scene- or application-owned font family with explicit roles:

```text
font family
├── sans regular
├── sans bold
├── sans italic
├── sans bold-italic
├── mono regular
└── scientific fallback
```

Each role resolves to one immutable font source:

```text
source kind: builtin | file | memory
identity: source digest + face index + load parameters
bytes: immutable for the source lifetime
metadata: diagnostic family and style names
```

`family` and `style` metadata describe and diagnose a selected source. They must not silently trigger operating-system discovery. No platform font discovery is required for v0.4.

Built-in and file sources should be resolved into immutable bytes before dependent resources are created. A file source must not change rendering merely because the original file is replaced later. A public memory-source API is optional before v0.4 freeze and requires an explicit copy-or-borrow ownership contract.

Scene text and ImGui share default-family policy and immutable source identity, but not backend objects, atlas textures, glyph packing, or GPU lifetime. Their atlases have different consumers and loading strategies.

## Public Contract

The current public escape hatch remains descriptor-oriented:

```c
DvzFontDesc desc = dvz_font_desc();
desc.path = "/absolute/or/application-owned/font.ttf";
desc.face_index = 0;
DvzFont* font = dvz_font(scene, &desc);

DvzTextStyle style = dvz_text_style();
style.font = font;
dvz_text_set_style(text, &style);
```

`dvz_font()` copies descriptor strings into a scene-owned font resource. `DvzTextStyle.font` is a borrowed reference and the selected font must remain alive while dependent text, annotations, atlases, or glyph visuals may use it. Cross-scene references are invalid.

Before API freeze, Datoviz must either make destruction invalidate or detach dependents safely, or document and enforce that referenced fonts are released only during scene teardown. Destroyed font slots must not silently accumulate into permanent capacity loss.

Public family/source APIs should expand only when source ownership, role resolution, dependent lifetime, cache identity, bindings, and diagnostics are mature enough to freeze. HarfBuzz, fallback chains, and platform discovery are not prerequisites for the built-in-family migration.

## Coverage Policy

One machine-readable coverage policy must drive validation and backend-specific products. It declares required and optional Unicode codepoints, visible fallback behavior, and canonical scientific spelling.

Derived consumers are:

1. cmap validation for every built-in face;
2. ImGui preload ranges or an explicit codepoint list;
3. the scene atlas seed set;
4. demand-loaded scene glyphs;
5. missing-glyph and regression tests.

Scene glyph loading remains demand-driven. ImGui may preload a bounded scientific set, but that policy must not force the scene atlas to preload the same range.

Use canonical text such as `Å` and `°C` where appropriate rather than compatibility characters. Do not promise U+FFFD unless the selected faces and fallback contract actually provide it. Required mathematical or scientific glyphs absent from Source Sans 3 are resolved from Noto Sans Math. Codepoints absent from both built-ins produce the documented visible `?` fallback.

## Generated Products

Every embedded font or generated atlas product records:

1. exact source filename and SHA-256;
2. upstream repository and immutable revision or release;
3. license file and redistribution basis;
4. generator command and generator/dependency versions;
5. face index, glyph set, renderer parameters, dimensions, and encoding;
6. output SHA-256;
7. a drift check proving two clean generations are byte-identical.

Cached downloads must be accepted only after their expected hash is verified. A generated MSDF atlas identity includes the exact font digest, glyph set, generator version, and all generation parameters.

## Migration Sequence

### Stage 1: correctness and ownership — complete

Fix retained text so an explicitly selected `DvzTextStyle.font` reaches the batched glyph realization path. Specify font destruction and dependent-resource behavior, remove permanent slot leakage, and test cross-scene rejection, font switching, teardown, and atlas identity.

### Stage 2: direct Source admission — complete

Present the exact five official Source files and one scientific fallback, including hashes, sizes, metadata, licenses, coverage report, compressed artifact impact, and proposed parent-repository paths for maintainer approval. After approval, admit the complete unmodified family directly into `assets/runtime/fonts/`; do not first copy the ten mixed-version legacy fonts into active parent Git.

The admission replaces the temporary legacy manifest atomically. Exact legacy bytes remain recoverable from the frozen `datoviz/data` snapshot and Git history, but they are not an intermediate runtime architecture.

### Stage 3: role resolution and consumer switch — complete for native consumers

Route scene defaults, per-codepoint scientific fallback, rich-text face selection, ImGui default policy, embedded-resource generation, source releases, packages, and tests through the six admitted roles while keeping backend atlases separate. Preserve file-path custom fonts and switch every active default consumer together so no partial Source/Roboto/Karla state ships. The legacy baked Roboto MSDF optimization is isolated from the Source default and requires a later explicit generated-payload disposition.

### Stage 4: independence and rendering proof — independence complete, visual/package proof pending

An isolated source copy with no `data` directory configures and builds the complete scene profile, including GUI, and its Source/Noto atlas, rich-text, and embedded-resource tests pass. The deterministic source bundle contains all six admitted fonts and no legacy `data/assets/fonts` entries. Remaining proof covers scene and ImGui captures, content-scale/DPI changes, exact wheels and installed consumers, and license packaging. Separately approve and validate removal or Source regeneration of the legacy baked Roboto atlas payload.

If the direct migration cannot be completed cleanly before v0.4 API freeze, retain the current default family from the legacy snapshot rather than committing an incomplete Source family or importing obsolete legacy files into active parent Git.

### Post-v0.4

Add HarfBuzz shaping, BiDi, script/language/features, explicit fallback chains, optional platform discovery, face-and-glyph-ID atlas keys, run segmentation, color emoji, persistent caches, and variable-font controls only through independent reviewed slices.

## Acceptance

The v0.4 font architecture is ready when:

1. ordinary build, install, package, scene text, and ImGui use require neither `data` nor network access;
2. all six built-in roles work offline and select real faces;
3. custom file fonts work on the main retained-text path and remain supported after the default switch;
4. ownership and destruction cannot leave dangling font references or exhaust slots silently;
5. source identity, licenses, coverage, generated products, and drift checks are complete;
6. scene and ImGui use the same default-family policy without sharing incompatible atlas state;
7. glyph fallback, DPI changes, native rendering, and WebGPU-relevant generated outputs have focused regressions;
8. no active font payload or generated atlas depends on Git LFS.
