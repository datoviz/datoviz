# Example Data Policy

This policy covers example-data bundles prepared by `tools/data/*` and stored in the `data` submodule.
Do not edit scripts or data files when the task is documentation-only.

The canonical v0.4 data-branch strategy is in
[`../../spec/data/V0_4_DATA_REPOSITORY.md`](../../spec/data/V0_4_DATA_REPOSITORY.md). In particular,
the v0.4 data branch should be clean and should not carry old v0.3 payloads forward unless they are
promoted into the v0.4 layout with fresh manifest and provenance records.

## Commit Order

Example data is stored in a separate Git repository mounted as the parent repo's `data` submodule. When a
change includes both data and parent-repo references, keep the order explicit:

1. In `data/`, stage and commit the changed bundles, manifests, provenance files, blocker notes, or LFS
   pointer updates.
2. Return to the parent repository and stage the updated `data` submodule pointer plus any parent-repo
   code or documentation that depends on it.
3. Commit the parent repository only after the submodule commit exists locally.
4. Do not push either repository unless the user explicitly asks for a push. If a push is requested later,
   push the data submodule commit first, then the parent repository commit that references it.

Useful status checks:

```bash
git -C data status --short
git status --short
git diff --submodule
```

## No-Push Reminder

Regeneration and validation tasks should leave commits and pushes under human control unless the user
explicitly asks otherwise. In particular, do not run:

```bash
git -C data push
git push
```

## LFS Policy

Keep large binary artifacts in the `data` submodule, not in the parent repository. Prefer Git LFS for
large prepared binaries such as `.npy`, `.npz`, compressed arrays, images, meshes, and other files that are
expensive to diff or likely to grow.

Before pushing a data commit, confirm that large files are LFS pointers in Git rather than ordinary blobs:

```bash
git -C data lfs ls-files
git -C data ls-tree -r -l HEAD examples | sort -k4 -nr | head
git -C data lfs status
```

The `ls-tree` check should show only small pointer blobs for large artifacts. If a large prepared file
appears as a multi-megabyte Git blob, fix `.gitattributes` and re-add that file before pushing.

Do not commit raw downloads, scratch intermediates, caches, or temporary extraction directories unless a
preparation script intentionally promotes a source artifact and the provenance explains why it must be
versioned. Use `.cache/datoviz/examples/<example_id>/source/` for local raw sources,
`.cache/datoviz/examples/<example_id>/work/` for downloads and intermediates, and
`.cache/datoviz/examples/<example_id>/prepared/` for developer-only render-ready payloads that are not
being promoted into the data submodule.

Manifests and provenance files are normal text files and should not be stored through LFS.

When a push is eventually requested, upload LFS objects before pushing the parent repository pointer:

```bash
cd data
git lfs status
git lfs push origin main
git push origin main
cd ..
git push
```

Do not run those commands unless the user explicitly asks for a push.

## Size Review

Prepared bundles can be technically safe in Git because they are LFS-managed while still being too heavy
for a default example. Review both working-tree size and duplicated source/prepared payloads:

```bash
du -sh data/examples data/examples/* data/examples/*/*
find data/examples -type f -printf '%s %p\n' | sort -nr | head
```

Current policy:

- Full-scale benchmark/showcase data may be committed when the example's purpose is to stress large
  rendering paths and provenance is clear.
- Regular tutorial/default examples should prefer compact subsets or deterministic generated fixtures.
- If a source archive is already committed, avoid also committing a large prepared duplicate unless the
  runtime example needs the prepared format to run without Python preprocessing.
- For LIDAR specifically, the full prepared split arrays are acceptable only as an intentional full-scale
  point-cloud/EDL showcase. For lightweight examples, add a smaller subset bundle and keep the full bundle
  optional.

## External-Source Blockers

Some datasets cannot be redistributed automatically because the source requires credentials, manual
acceptance of terms, unclear licensing, a fragile API, or a citation/license review. In those cases:

- Do not commit guessed, scraped, or manually downloaded artifacts without clear redistribution approval.
- Add or keep a `BLOCKERS.md` file next to the bundle explaining what prevents preparation.
- Use manifest status `external-required` or another explicit blocked status.
- Keep `PROVENANCE.md` clear that no redistributable prepared artifact is present yet, or that only a
  documented placeholder/blocker bundle exists.
- Prefer deterministic generated or synthetic examples when the external source is not necessary for the
  example's rendering behavior.

## Synthetic and Fallback Data

Use `generated` for deterministic examples whose source of truth is the preparation script and seed. These
bundles are first-class example data when they are intentionally synthetic.

Use `fallback` only when synthetic data stands in for a desired external/public dataset that is blocked,
temporarily unavailable, too large, or not redistributable. The manifest and provenance should make the
distinction explicit so loaders, tests, and documentation do not confuse fallback data with the real source.

Runtime examples that declare prepared, generated, or external data must not silently synthesize an
in-memory stand-in when the expected bundle is missing. They should fail hard and print the exact
preparation command. Keep simulation/synthetic examples explicit by making generated data the declared
source, either embedded in the example or produced by a preparation script.

Generated or fallback data must still include stable parameters, seeds when applicable, artifact hashes,
sizes, validation facts, and provenance notes.

## Pre-Existing Untracked Data

Before regenerating, check for untracked or modified files inside the data submodule:

```bash
git -C data status --short
```

If untracked data already exists, do not delete, move, overwrite, stage, or normalize it unless the user
confirms it belongs to the current task. Treat it as someone else's local work. Regenerate only the
requested bundle when possible, and record in your final note that unrelated pre-existing untracked data
was left untouched.

Known local scratch paths can be ignored in the data submodule when they duplicate committed prepared
bundles or represent cache/source extraction directories. For example, `allen_ibl_assets/` is a local
scratch/source-prep path; the committed prepared copy lives under `data/examples/allen_ibl/`.

If a script would overwrite pre-existing untracked files, stop and ask for direction. A safe follow-up is
to rerun the script against a clean checkout or after the owner stages, removes, or relocates those files.

## Review Checklist

Before handing off a data workflow change:

- Run the narrow regeneration command needed for the touched bundle or group.
- Run `python tools/data/normalize_manifests.py ...` if manifest metadata may be stale.
- Run `python tools/data/validate_manifests.py ...`.
- Check `git -C data status --short` and `git status --short`.
- Confirm data changes are committed in the submodule before committing any parent-repo pointer update.
- Leave pushes to the user unless explicitly requested.
