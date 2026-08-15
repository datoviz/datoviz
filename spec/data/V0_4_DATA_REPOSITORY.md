# Datoviz v0.4 Data Repository Strategy

> **Status:** implemented transitional policy; superseded as a target architecture by [ASSET_ARCHITECTURE.md](ASSET_ARCHITECTURE.md)

This document describes the currently implemented v0.4 data-submodule model and remains relevant while that model exists. New architectural work must follow the no-LFS ownership and migration target in `ASSET_ARCHITECTURE.md`; do not expand the submodule or treat its Git LFS policy as the final Datoviz boundary.

## Summary

Datoviz v0.4 should keep using the existing `datoviz/data` repository, but the v0.4
line should be a clean `v0.4-dev` branch with a new layout and policy. The v0.4 data branch
should not carry old v0.3 payloads forward just because they already exist on the historical
branch.

The parent repository owns the API, examples, preparation scripts, manifest schema, and submodule
pointer. The data repository owns redistributable, render-ready assets and their provenance.

## Branch Model

- Keep the GitHub repository URL as `https://github.com/datoviz/data`.
- Create a `v0.4-dev` branch in that repository for Datoviz v0.4 assets.
- Make the parent v0.4 branch point its `data` submodule at a specific commit on the data
  `v0.4-dev` branch.
- Do not migrate v0.3 assets wholesale. Promote only assets that are useful for v0.4 examples,
  fixtures, gallery validation, or reusable runtime resources.
- Keep the historical v0.3 data branch available for old releases, but do not treat it as the
  v0.4 source of truth.

## Repository Layout

The v0.4 data branch should use a small number of top-level directories with clear ownership:

```text
data/
  README.md
  LICENSES/
  examples/
    <example_id>/
      manifest.json
      PROVENANCE.md
      prepared/
      BLOCKERS.md
  fixtures/
    <fixture_id>/
      manifest.json
      PROVENANCE.md
      prepared/
  assets/
    fonts/
    textures/
    colormaps/
  gallery/
    baselines/
```

Directory roles:

- `examples/` contains user-facing datasets that are ready to load by examples or showcase programs.
- `fixtures/` contains small deterministic data used by tests, specs, and validation fixtures.
- `assets/` contains reusable runtime assets such as fonts, textures, and colormap atlases.
- `gallery/baselines/` contains expected images only when they are actively used for validation.
- `LICENSES/` contains copied license texts or attribution records when a source requires them.

There should be no `legacy_v03/` directory on the v0.4 data branch. If a legacy asset is still useful,
copy or regenerate it into the v0.4 layout with fresh manifest and provenance records.

## Data States

Use these states consistently:

- Raw source: original public, local, or manually downloaded files.
- Cache: local downloads, intermediates, and expensive generated files that are not committed.
- Prepared data: render-ready arrays, meshes, images, or binary blobs committed in the data submodule.

Local cache paths belong outside committed data:

```text
.cache/datoviz/examples/<example_id>/
  source/
  prepared/
  work/
```

Use `source/` for manually supplied or downloaded raw files, `work/` for scratch intermediates, and
`prepared/` for local render-ready outputs. If the dataset is later promoted to a redistributable v0.4
bundle, the local `prepared/` files become the input to `data/examples/<example_id>/prepared/` after
manifest and provenance review.

## Render-Ready Bundles

Each committed example or fixture bundle should include:

- `manifest.json` using the current `datoviz.example-data.v1` schema.
- `PROVENANCE.md` with source, processing, license, and attribution notes.
- `prepared/` containing the files consumed by Datoviz examples or tests.
- `BLOCKERS.md` only when the real source cannot be redistributed or regenerated automatically.

Manifest artifact entries should record at least:

- relative path
- byte size
- SHA-256 hash
- format
- dtype and shape for arrays
- coordinate system, units, color space, or semantic role when known

## Large Data Rules

- Large prepared binaries must live in the `data` submodule, not in the parent repository.
- Prefer Git LFS for large `.npy`, `.npz`, compressed arrays, images, meshes, and binary payloads.
- Do not commit raw downloads, temporary extraction directories, or scratch conversion outputs unless
  a preparation script intentionally promotes them and provenance explains why.
- Do not commit generated runtime payloads into the parent repo.
- Provide compact default bundles for normal examples. Keep full-scale benchmark or showcase datasets
  optional unless the example specifically exists to stress large rendering paths.
- If redistribution rights are unclear, commit a blocker/provenance record, not guessed data.

## Public Dataset Showcase Policy

Real scientific datasets are preferred for flagship gallery examples when they can be handled
legally and reproducibly. A dataset may become a v0.4 showcase only after these checks are recorded:

- stable source URL, dataset or paper citation, and release date;
- license, redistribution rights, citation requirements, and derived-media permissions;
- scientist, group, or project contact route when individual feedback will be requested;
- raw data size, prepared data size, expected cache size, and optional full-scale benchmark size;
- preprocessing script or manual preparation notes sufficient to rebuild the render-ready bundle;
- clear statement of whether prepared files may be committed to `data`, must remain local cache, or
  require a `BLOCKERS.md` note.

When the example is prepared for outreach, keep the data record tied to the gallery record:

1. the gallery page links to `PROVENANCE.md`, source data, license, citation, and code;
2. generated screenshots, videos, or GIFs are traceable to the example id and data manifest;
3. scientist feedback that changes attribution, interpretation, or allowed use is reflected before
   public launch;
4. no endorsement, quote, or prominent naming is used without explicit permission.

## Regeneration Workflow

Data preparation should be script-first and reproducible:

```text
source data
  -> tools/data/prepare_<example_id>.py
  -> .cache/datoviz/examples/<example_id>/source/ for raw local sources
  -> .cache/datoviz/examples/<example_id>/work/ for downloads and intermediates
  -> .cache/datoviz/examples/<example_id>/prepared/ for local render-ready artifacts
  -> data/examples/<example_id>/prepared/ for committed render-ready artifacts
  -> manifest/provenance validation
```

Run regeneration and validation from the parent repository root:

```bash
python tools/data/prepare_<example_id>.py
python tools/data/normalize_manifests.py data/examples/<example_id>
python tools/data/validate_manifests.py data/examples/<example_id>
```

For developer-only local showcase data, examples may consume
`.cache/datoviz/examples/<example_id>/prepared/` directly. Such scripts should not touch the data
submodule unless the dataset is being promoted to a redistributable v0.4 bundle.

## Commit Workflow

When a change includes data-submodule content and parent-repo references, commit in this order:

1. In `data/`, commit prepared artifacts, manifests, provenance files, blocker notes, and LFS pointer
   updates.
2. In the parent repository, commit the updated `data` submodule pointer and any scripts, examples, or
   documentation that depend on that data commit.
3. Push only when explicitly requested. Push the data repository first, then the parent repository.

Before committing parent-repo changes, verify that unapproved data-submodule changes and large binaries
are not staged:

```bash
git -C data status --short
git status --short
git diff --submodule
```

## Migration Plan

1. Create the `v0.4-dev` branch in `datoviz/data` from an empty or minimal root.
2. Add the v0.4 layout, README, LFS rules, and license/attribution conventions.
3. Promote only the assets needed by active v0.4 examples, tests, and gallery validation.
4. Regenerate those assets through `tools/data/*` wherever practical.
5. Validate manifests and provenance before updating the parent submodule pointer.
6. Keep any blocked external datasets represented by `BLOCKERS.md` and clear manifest statuses.

This keeps useful data available to users without letting v0.3 repository history dictate the v0.4
data model.
