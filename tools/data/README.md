# Datoviz Example Data Tools

Scripts in this directory prepare deterministic, render-ready data bundles for examples.
See [DATA_POLICY.md](DATA_POLICY.md) for commit order, storage policy, blocked external sources, and
how to handle existing untracked data.

The v0.4 data repository branch strategy and target layout are specified in
[`../../spec/data/V0_4_DATA_REPOSITORY.md`](../../spec/data/V0_4_DATA_REPOSITORY.md). The short version:
the parent repo keeps preparation scripts and schema policy, while the `data` submodule stores only
redistributable v0.4 render-ready bundles and reusable assets.

Prepared artifacts live in the `data` submodule under:

```text
data/examples/<example_id>/
  manifest.json
  PROVENANCE.md
  prepared/
```

Raw downloads and large intermediates should stay outside committed data unless a script
intentionally records them as source artifacts:

```text
.cache/datoviz/examples/<example_id>/
  source/
  prepared/
  work/
```

Every committed bundle should include:

- `manifest.json` using schema `datoviz.example-data.v1`
- `PROVENANCE.md` with source, processing, license, and notes
- file hashes and sizes for every prepared artifact

## Regenerate

Run commands from the parent repository root:

```bash
python tools/data/prepare_all.py
python tools/data/prepare_all.py existing
python tools/data/prepare_all.py napari generated
python tools/data/prepare_all.py --keep-going external
```

The available groups are:

- `existing`: bundles prepared from checked-in or already mirrored source assets.
- `napari`: public napari-adjacent or synthetic microscopy examples.
- `generated`: deterministic synthetic/generated examples.
- `external`: source-gated examples that may produce blocker notes instead of artifacts.

Individual preparation scripts may also be run directly when iterating on one bundle:

```bash
python tools/data/prepare_cells3d.py
python tools/data/normalize_manifests.py data/examples/napari/cells3d
python tools/data/validate_manifests.py data/examples/napari/cells3d
```

`prepare_all.py` runs `normalize_manifests.py` and `validate_manifests.py` after the selected groups.
When running scripts individually, run normalization before validation if metadata or artifact layout may
have changed.

## Validate

Use the manifest validator before staging data changes:

```bash
python tools/data/validate_manifests.py
python tools/data/validate_manifests.py data/examples/<example_id>
python tools/data/validate_manifests.py data/examples/<example_id>/manifest.json
```

The validator checks the schema id, required manifest fields, artifact existence, byte counts, and SHA-256
hashes. It does not prove license acceptability or source redistribution rights; record those decisions in
`PROVENANCE.md` and apply the policy in [DATA_POLICY.md](DATA_POLICY.md).

Before finalizing documentation-only changes in this directory, also run:

```bash
git diff --check -- tools/data/README.md tools/data/MANIFEST_SCHEMA.md tools/data/DATA_POLICY.md
```
