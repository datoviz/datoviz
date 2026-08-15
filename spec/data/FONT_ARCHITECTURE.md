# Datoviz Font Architecture

> **Status:** implemented native consumer and deterministic atlas contract pending visual and installed-package proof
> **Scope:** scene text, rich text blocks, ImGui, embedded resources, custom fonts, glyph coverage, licensing, and deterministic generation

## Decision

Datoviz must provide useful, deterministic text without a network, an initialized `data` submodule, platform font discovery, or application configuration. The built-in family has five required face roles: sans regular, sans bold, sans italic, sans bold-italic, and mono regular. A sixth scientific fallback role supplies mathematical and technical codepoints absent from the selected primary face. Every required role remains available to ordinary library users and offline rich-text paths.

The approved long-term built-in family is the complete unmodified static Source Sans 3 regular, bold, italic, and bold-italic faces plus Source Code Pro regular, with unmodified Noto Sans Math regular as the scientific fallback. Exact files, upstream revisions, SHA-256 digests, sizes, OFL texts, font metadata, and coverage policy live under `assets/runtime/fonts/`. The native consumer switch, deterministic Source atlas admission, an isolated build with `data` absent, and source-bundle inclusion are validated. Scene/ImGui visual review and exact installed-package proof remain required before release acceptance.

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
10. printable ASCII for the standard 32/4, 64/8, and 128/16 Source Sans 3 Regular MSDF recipes uses approved deterministic embedded products; non-ASCII requests rebuild the requested slot transactionally at runtime with Source and the scientific fallback.

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

## RC3 MSDF Atlas Architecture

The RC3 atlas refactor separates a pure CPU atlas-product builder from scene and GPU realization. The builder consumes a resolved immutable font source, a canonical glyph set, and a generation recipe, and returns pixels, metrics, glyph records, coverage, and product identity. It must not create a scene, sampled field, GPU resource, or cache entry. The developer serializer and runtime generator consume this same product layer so generated and runtime products cannot diverge by construction.

The realization layer owns persistent request slots. Each slot retains a stable `DvzTextAtlas` and a stable resizable `DvzSampledField` for the resource lifetime; rebuilding or extending a product updates those objects without invalidating documented pointers. A completed CPU product is installed transactionally. Failed generation or extension leaves the previous realization untouched, and a failed partial MSDF result must never shadow a complete SDF fallback.

Atlas cache identity uses a private resolved source identity, not a family-name string. The built-in Source Sans 3 source has a stable identity such as `SOURCE_SANS_3_REGULAR`; custom paths, memory sources, face indices, and load parameters remain distinct identities and must not collide with the built-in cache. Generated product identity records the source digest and face/load parameters needed to reproduce the product; runtime cache selection uses the already resolved internal source identity without repeatedly hashing embedded bytes.

Coverage is strict at the product boundary: `ensure()` succeeds only when every requested codepoint is represented or is explicitly mapped to the documented visible `?` fallback. Fallback selection is transactional across backends and must resolve a complete product before replacing the active one. Atlas dimensions, allocation sizes, and extension growth are bounded by explicit budgets. The implementation must not use repeated whole-atlas doubling as an unbounded extension policy.

The default scene seed is printable ASCII. Scientific and technical glyphs remain demand-driven for scene text, while ImGui may use its separately declared bounded preload policy. RC3 retains the existing 32/4, 64/8, and 128/16 atlas products; changing that size policy requires measurements and a separate post-RC3 decision.

The developer-only generator is excluded from ordinary library builds and writes the recipe and generated include at `assets/runtime/text/default_msdf_atlas.json` and `src/scene/text/generated/text_default_msdf_atlas.inc`. The generated include remains textual for RC3. Generation commands report source digests, backend and dependency versions, dimensions, byte sizes, timings, coverage, and output hashes. The admitted manifest and include were generated twice with byte-identical output and approved as exact payloads; later replacement repeats that explicit review checkpoint.

Reproducibility has two levels. Canonical byte identity is required in a pinned Linux generation environment whose toolchain and dependencies are recorded in the recipe. Other supported platforms must provide portable structural and rendering equivalence, including matching coverage, metrics within the declared tolerance, bounds, and focused visual regressions; universal cross-platform byte identity is not promised.

## Migration Sequence

### Stage 1: correctness and ownership — complete

Fix retained text so an explicitly selected `DvzTextStyle.font` reaches the batched glyph realization path. Specify font destruction and dependent-resource behavior, remove permanent slot leakage, and test cross-scene rejection, font switching, teardown, and atlas identity.

### Stage 2: direct Source admission — complete

Present the exact five official Source files and one scientific fallback, including hashes, sizes, metadata, licenses, coverage report, compressed artifact impact, and proposed parent-repository paths for maintainer approval. After approval, admit the complete unmodified family directly into `assets/runtime/fonts/`; do not first copy the ten mixed-version legacy fonts into active parent Git.

The admission replaces the temporary legacy manifest atomically. Exact legacy bytes remain recoverable from the frozen `datoviz/data` snapshot and Git history, but they are not an intermediate runtime architecture.

### Stage 3: role resolution and consumer switch — complete for native consumers

Route scene defaults, per-codepoint scientific fallback, rich-text face selection, ImGui default policy, embedded-resource generation, source releases, packages, and tests through the six admitted roles while keeping backend atlases separate. Preserve file-path custom fonts and switch every active default consumer together so no partial Source/Roboto/Karla state ships. The approved deterministic Source Sans 3 ASCII products replace the legacy baked Roboto optimization.

### Stage 4: independence and rendering proof — independence complete, visual/package proof pending

An isolated source copy with no `data` directory configures and builds the complete scene profile, including GUI, and its Source/Noto atlas, rich-text, and embedded-resource tests pass. The deterministic source bundle contains all six admitted fonts, the approved Source ASCII atlas products, and no legacy `data/assets/fonts` or baked Roboto atlas entries. Remaining proof covers scene and ImGui captures, content-scale/DPI changes, exact wheels and installed consumers, and license packaging.

If the direct migration cannot be completed cleanly before v0.4 API freeze, retain the current default family from the legacy snapshot rather than committing an incomplete Source family or importing obsolete legacy files into active parent Git.

### Post-v0.4

Add HarfBuzz shaping, BiDi, script/language/features, explicit fallback chains, optional platform discovery, face-and-glyph-ID atlas keys, run segmentation, color emoji, persistent disk caches, binary atlas containers, font subsetting, multi-page stable-UV allocation, and variable-font controls only through independent reviewed slices. Full scientific-range preloading and a change to the 32/64/128 size policy also remain deferred until measurements justify them.

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
