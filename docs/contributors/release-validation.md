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
just release-plan 0.4.0rc2
just release-dry-run 0.4.0rc2 --wheel path/to/datoviz-0.4.0rc2-...whl
just release-candidate 0.4.0rc2 --dry-run
just release-candidate 0.4.0rc2
just release-notes 0.4.0rc2
just release-docs-validate 0.4.0rc2
just release-report 0.4.0rc2
```

`release-candidate` writes local state under `build/release/<version>/`. The first automation slice
does not tag, upload, publish, push, or mutate GitHub. Publication remains a separate approval-gated
phase.

`release-notes` writes a generated draft to `build/release/<version>/release-notes.md` and records
it as the GitHub draft notes artifact. Review and edit the wording before using it for publication.
`release-docs-validate` records `docs-api-check` plus selected fenced code-block doctests in
`build/release/<version>/docs-validation.json`.

For installed-artifact validation on a physical machine, use:

```sh
just release-validation-pack 0.4.0rc2 --wheel path/to/datoviz-0.4.0rc2-...whl
just release-machine-plan 0.4.0rc2 --wheel path/to/datoviz-0.4.0rc2-...whl
```

Copy `build/release/0.4.0rc2/validation-pack/datoviz-0.4.0rc2-validation.tar.gz` to the target
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
just release-machine-validate 0.4.0rc2 --wheel path/to/datoviz-0.4.0rc2-...whl --profile rc
just release-report 0.4.0rc2
```

The `quick` profile runs wheel inventory plus installed import/CLI smoke. The `rc` profile adds the
CMake consumer smoke and installed Python/C no-window example smokes. The `full` profile adds
shaderc plus installed Python/C offscreen render example smokes. Evidence is written under
`build/release/<version>/evidence/<machine-id>/`.
Machine validation scans command logs for Datoviz errors, Vulkan validation diagnostics, sanitizer
output, crashes, and Python tracebacks. Error diagnostics are blocking even when the process exits
with status 0; warning diagnostics are recorded for review.
Use `release-machine-plan` after each evidence ingest to see which machine classes are still
missing, failed, or ready.

Automated installed-artifact profiles do not replace the required human interaction pass. On each
available required machine, continue with [Physical release validation](release-physical-validation.md):
run the Quickstart from the isolated candidate wheel, then let the local agent launch the fixed live
example set and record the maintainer's observations.

To return evidence from a validation pack, archive the generated evidence directory:

```sh
./archive-evidence.sh <machine-id>
```

On Windows PowerShell:

```powershell
./archive-evidence.ps1 -MachineId <machine-id>
```

Back in the release checkout, ingest it and refresh the matrix report:

```sh
just release-ingest-evidence 0.4.0rc2 path/to/evidence-<machine-id>.tar.gz
just release-machine-plan 0.4.0rc2
just release-report 0.4.0rc2 --strict-matrix
just release-gates 0.4.0rc2 --write-artifacts --strict-matrix
```

Publication rehearsals stay approval-gated:

```sh
just release-dry-run 0.4.0rc2 --wheel path/to/datoviz-0.4.0rc2-...whl --dist-dir dist --write-report
just release-testpypi 0.4.0rc2 --dry-run --dist-dir dist
just release-github-draft 0.4.0rc2 --dry-run
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
