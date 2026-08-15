# Runtime Font Provenance

## Purpose

This directory contains the complete built-in v0.4 font family and its scientific Unicode fallback. Ordinary build, installed-library use, scene text, rich text, and ImGui must use these parent-owned resources without a network, Git LFS, platform font discovery, or an initialized `datoviz/data` submodule.

The machine-readable identities are in [manifest.json](manifest.json), and the required coverage and visible fallback contract are in [coverage.json](coverage.json). Every font is an exact unmodified upstream static TTF. Datoviz does not subset or rename these admitted files.

## Built-In Roles

Source Sans 3 supplies regular, bold, italic, and bold-italic sans roles. Source Code Pro supplies the regular monospaced role. Noto Sans Math is a per-codepoint scientific fallback for glyphs absent from the selected primary face; it is not the ordinary label face and does not define a future equation aesthetic.

Scene text and ImGui share this role policy but retain separate backend atlases and GPU resources. Custom application fonts remain supported through `DvzFontDesc.path` and `DvzTextStyle.font`; the built-in fallback must not silently replace a valid glyph from an explicitly selected primary font.

## Exact Upstream Boundary

The Source Sans files come from Adobe's `source-sans` tag `3.052R` at commit `ed1808970eb3c7301c9a523bee26473ba0bb62fa`. The Source Code Pro file comes from Adobe's `source-code-pro` tag `2.042R-u/1.062R-i/1.026R-vf` at commit `d3f1a5962cde503f9409c21e58527611d4a19ef1`.

Noto Sans Math comes from the official `notofonts/math` release `NotoSansMath-v3.000`, target commit `00e9941d95b2a355399a66f3990ffde6e4985676`. The exact release archive and member path are recorded in the manifest so the admitted TTF can be reproduced without relying on an unpinned font CDN.

The mixed-version Roboto, Roboto Mono, Droid Sans, Inconsolata, Karla, and Cousine payloads remain recoverable from frozen legacy history but are not admitted into the active runtime architecture.

## Licensing

All six fonts are distributed under the SIL Open Font License 1.1. The upstream license texts are stored separately under `LICENSES/` because their copyright headers differ; repository copies use canonical LF line endings with trailing whitespace removed, and the manifest records their canonical hashes. Source Sans 3 and Source Code Pro reserve the font name `Source`; the admitted files are unmodified originals, so the Reserved Font Name restriction does not require renaming.

## Missing Glyph Policy

The primary and scientific fallback together cover the audited standard mathematical operators, supplemental mathematical operators, arrows, geometric shapes, mathematical alphanumeric symbols, Greek, common units, and ordinary scientific annotation characters. They do not promise emoji, broad writing-system fallback, miscellaneous pictograms, or every Unicode compatibility character.

If the complete internal fallback chain lacks a codepoint, Datoviz renders the visible question mark U+003F. U+FFFD is not present in the admitted Source or Noto files and is not promised as the visible replacement glyph. Canonical spellings such as `°C`, `°F`, and `Å` are preferred over compatibility characters `℃`, `℉`, and `Å`.

Future mini-LaTeX equation rendering is a separate parser and OpenType MATH layout subsystem. It may use a dedicated equation font and share lower-level glyph rendering without changing the ordinary Unicode fallback policy.

## Verification

The admission checker verifies file sizes, SHA-256 identities, license identities, OpenType family/style metadata, and the coverage policy. Build, runtime lookup, rich-text face selection, ImGui initialization, source bundles, packages, and no-data validation must consume this directory before legacy lookup paths are removed.
