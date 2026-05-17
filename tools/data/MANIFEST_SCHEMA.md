# Example Data Manifest Schema

Current schema id: `datoviz.example-data.v1`

Required top-level fields:

- `schema`: schema identifier.
- `id`: stable example-data id.
- `title`: human-readable title.
- `status`: `committed`, `generated`, `fallback`, `external-required`, or another explicit status.
- `producer`: script, script version, and command used to build the bundle.
- `source`: source name, URLs, citation/license notes, and retrieval details when applicable.
- `processing`: timestamp, Python version, platform, and optional dependency/parameter details.
- `artifacts`: prepared files with role, relative path, format, byte size, and SHA-256 hash.
- `validation`: compact facts that loaders/tests can use to sanity-check the bundle.

Recommended status values:

- `committed`: redistributable prepared artifacts are committed in the `data` submodule.
- `generated`: artifacts are deterministic synthetic/generated data and are committed as primary example
  data.
- `fallback`: artifacts are deterministic synthetic/generated data standing in for a blocked or unavailable
  external source.
- `external-required`: the real source needs credentials, manual download, license review, accepted terms,
  or another blocker before artifacts can be committed.
- `external-public`: artifacts or metadata come from a public external source that is believed to be
  redistributable; provenance must carry source and license notes.

Artifact entries should include shape, dtype, coordinate system, units, or semantic roles whenever
that information is known.

Human-readable provenance belongs in `PROVENANCE.md` next to the manifest. The manifest should be
stable enough for scripts and tests; provenance can contain narrative notes, source caveats, and
license discussion.

Manifests should describe prepared, render-ready artifacts only. Raw downloads, caches, and large
intermediates normally stay out of committed data unless the preparation script intentionally records them
as source artifacts and provenance explains why they are versioned.

Use `BLOCKERS.md` next to a manifest when external-source requirements prevent redistribution or automated
preparation. Blocked bundles should use an explicit blocked status such as `external-required` and should
not silently substitute synthetic data. If a synthetic stand-in is committed, use `fallback` and explain the
distinction in both `source` fields and `PROVENANCE.md`.

After regenerating or editing a manifest, validate it from the parent repository root:

```bash
python tools/data/validate_manifests.py data/examples/<example_id>
```

For the complete workflow, storage, LFS, and commit-order policy, see
[DATA_POLICY.md](DATA_POLICY.md).
