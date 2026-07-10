# Release Validation

Use this page as the maintainer index for v0.4 release-candidate checks. Durable policy lives in
`spec/release/`; this page links the runnable contributor procedures.


## Core Checks

Run the smallest relevant loop while iterating, then record the broader checks used for each RC:

```sh
git diff --check
just build
just test
just spec-check
```

For a local RC1 preflight driver over the standard sequence, use:

```sh
just release-preflight rc1
```

For the v0.4 release automation front door, use:

```sh
just release-plan 0.4.0rc1
just release-dry-run 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl
just release-candidate 0.4.0rc1 --dry-run
just release-candidate 0.4.0rc1
just release-report 0.4.0rc1
```

`release-candidate` writes local state under `build/release/<version>/`. The first automation slice
does not tag, upload, publish, push, or mutate GitHub. Publication remains a separate approval-gated
phase.

For installed-artifact validation on a physical machine, use:

```sh
just release-validation-pack 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl
```

Copy `build/release/0.4.0rc1/validation-pack/datoviz-0.4.0rc1-validation.tar.gz` to the target
machine, extract it, then run:

```sh
./validate-rc.sh
```

On Windows PowerShell:

```powershell
./validate.ps1 -Profile rc
```

If the target machine has the repository checkout, it can run the validator directly:

```sh
just release-machine-validate 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl --profile rc
just release-report 0.4.0rc1
```

The `quick` profile runs wheel inventory plus installed import/CLI smoke. The `rc` profile adds the
CMake consumer smoke. The `full` profile adds shaderc and render smoke. Evidence is written under
`build/release/<version>/evidence/<machine-id>/`.

To return evidence from a validation pack, archive the generated evidence directory:

```sh
tar -czf evidence-<machine-id>.tar.gz evidence/<machine-id>
```

Back in the release checkout, ingest it and refresh the matrix report:

```sh
just release-ingest-evidence 0.4.0rc1 path/to/evidence-<machine-id>.tar.gz
just release-report 0.4.0rc1 --strict-matrix
just release-gates 0.4.0rc1 --write-artifacts --strict-matrix
```

Publication rehearsals stay approval-gated:

```sh
just release-dry-run 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl --dist-dir dist --write-report
just release-testpypi 0.4.0rc1 --dry-run --dist-dir dist
just release-github-draft 0.4.0rc1 --dry-run
just release-create-tag 0.4.0 --dry-run
just release-pypi 0.4.0 --dry-run
just release-github-publish 0.4.0 --dry-run
just release-docs-publish 0.4.0 --dry-run
```


## Packaging

Wheel build, inspection, installed smoke tests, platform matrices, and the non-live GitHub Actions
draft are documented in [Release wheels](release-wheels.md).


## Maintainer Flow

Use [Release process](release-process.md) for the operational RC sequence and
[Release flight checklist](release-flight-checklist.md) for the step-by-step maintainer checklist.
Agents assisting release work should follow [Agent release checklist](agent-release-checklist.md).


## Release Records

Each RC note should include:

1. exact commit and tag;
2. feature/status table;
3. known issues;
4. platform validation matrix;
5. test and static-analysis summary;
6. docs and gallery build links;
7. wheel and source artifacts;
8. migration/status notes from v0.3 and development snapshots;
9. targeted feedback request.
