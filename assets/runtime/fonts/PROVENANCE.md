# Runtime Font Provenance

## Purpose

This directory is the ordinary-Git home for the exact font files required to preserve v0.4 build, scene-text, rich-text, source-release, and fallback behavior while the active library becomes independent from `datoviz/data` and Git LFS.

The machine-readable inventory is [manifest.json](manifest.json). Font binaries are admitted only after explicit maintainer approval of the exact paths, sizes, hashes, and licenses in that manifest.

## Exact Source Boundary

The preserved bytes come from `datoviz/data` commit `a9542d20f2d29aecb9518738f6b7ba1914b63997` under `assets/fonts/`. The manifest SHA-256 values identify the exact legacy bytes independently of that repository or its LFS service. The migration copies those bytes without modification and does not update the submodule or its gitlink.

The legacy repository did not retain a complete original-download ledger. Provenance is therefore classified as legacy-exact: Datoviz can prove the exact bytes and legacy source commit, and the fonts identify their family project, version, copyright, and license in their OpenType name tables, but an exact upstream commit is not claimed where it has not been recovered.

## Licensing

The exact fonts declare one of two licenses in their OpenType name tables:

- `Apache-2.0`: Droid Sans, Roboto Bold Italic, Roboto Italic, Roboto Medium, and Roboto Mono Medium.
- `OFL-1.1`: Inconsolata Regular, Roboto Black, Roboto Bold, Roboto Light, and Roboto Regular.

The admitted directory must include exact `LICENSES/Apache-2.0.txt` and `LICENSES/OFL-1.1.txt` copies. Primary family references are the archived [Roboto 2 repository](https://github.com/googlefonts/roboto-2), the active [Roboto classic repository](https://github.com/googlefonts/roboto-3-classic), [Roboto Mono](https://github.com/googlefonts/RobotoMono), [Inconsolata](https://github.com/googlefonts/Inconsolata), and the Android-origin Droid Sans attribution embedded in the font.

No current file is modified or subsetted. The later Source-family migration has its own admission record and must not reuse this legacy provenance.

## Verification

Before staging or packaging the fonts, run:

```sh
sha256sum assets/runtime/fonts/*.ttf
```

The output must match `manifest.json`. Build, runtime lookup, rich-text face selection, source-bundle construction, and no-data validation must consume this parent-owned directory before the legacy paths are removed.
