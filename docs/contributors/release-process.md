# Release Process

This is the operational maintainer process for Datoviz v0.4 release candidates. Durable policy and
exit criteria live in `spec/release/`; this page is the route to follow when preparing artifacts,
notes, and validation records.


## Roles Of The Release Candidates

Use explicit release candidates. Do not treat RCs as anonymous nightly snapshots.

| Phase | Purpose | Main output |
|---|---|---|
| Pre-RC | Remove known blockers and prove the release path locally. | Draft notes, local validation, draft artifacts. |
| RC1 | API and architecture candidate. | Public tag, release notes, first broad tester artifacts. |
| RC2 | Documentation and gallery candidate. | Mostly final docs, examples, gallery media, known gaps. |
| RC3 | Packaging and quality candidate. | Final packaging checks, licenses, checksums, blocker fixes only. |
| Final | Publish v0.4.0. | Final tag, PyPI artifacts, documentation, announcement assets. |

RC1 can still expose rough documentation and known issues. RC2 should make the user story coherent.
RC3 should be close enough that only release-blocking defects change before final.


## Release Inputs

Before starting an RC branch pass, collect these inputs:

1. active blockers from `agents/now/STATUS.md`;
2. release sequence from `agents/now/RELEASE.md`;
3. durable policy from `spec/release/READINESS.md` and `spec/release/RC_PROCESS.md`;
4. physical-machine policy from `spec/release/PHYSICAL_VALIDATION.md` and the runnable
   [physical release-validation procedure](release-physical-validation.md);
5. public documentation gates from `agents/now/DOCUMENTATION.md`;
6. wheel workflow details from [Release wheels](release-wheels.md);
7. the flight checklist from [Release flight checklist](release-flight-checklist.md);
8. citation metadata from `CITATION.cff`, [Citation](../reference/citation.md), and the JOSS draft
   in `paper/`.


## Pre-RC Preparation

Start from a clean understanding of the worktree:

```sh
git status --short
git diff --check
```

Resolve or explicitly ignore unrelated work before building release artifacts. Do not publish
artifacts from a tree with unreviewed staged changes, generated binary payloads, or an unintended
`data` submodule update.

For the automation front door, start with a dry run:

```sh
just release-plan 0.4.0rc1
just release-dry-run 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl
just release-candidate 0.4.0rc1 --dry-run
```

`release-dry-run` chains the plan, candidate dry-run, optional validation-pack rehearsal, strict
matrix report when state exists, and publication dry-runs. Add `--write-report` to save
`build/release/<version>/dry-run-report.md`.

After reviewing the dry-run output, run the local candidate flow:

```sh
just release-candidate 0.4.0rc1
just release-notes 0.4.0rc1
just release-docs-validate 0.4.0rc1
just release-report 0.4.0rc1
```

This writes `build/release/0.4.0rc1/release-state.json`, command logs, generated source bundle
metadata, discovered wheel checksums, generated release notes, documentation validation evidence,
and the first release report inputs. It does not tag, upload, publish, push, or create a GitHub
release.

Then run the minimum local proof:

```sh
just build
just test
just spec-check
```

Documentation validation is recorded separately because it produces release evidence:

```sh
just release-docs-validate 0.4.0rc1
```

By default this runs the generated C API classification check and selected fenced code-block
doctests. Pass repeated `--file <path>` arguments to validate additional markdown pages.

For wheel artifacts, run the host-local package proof:

```sh
just wheel-ci-local <host-platform-tag>
```

The release machine validator uses near-final installed wheel artifacts. Its `rc` profile runs
installed Python and C no-window examples in addition to import, CLI, native dependency inventory,
and CMake consumer checks. Its `full` profile adds installed offscreen render examples.

Use the rebuild variant when the local native build configuration is known to be valid:

```sh
just wheel-ci-local <host-platform-tag> 1
```

For Linux `x86_64` RC wheel evidence, prefer the controlled manylinux Docker route over the host
native wheel:

```sh
just wheel-manylinux-docker x86_64
```

Do not broaden this local Docker proof to Linux `aarch64` until the `x86_64` route is repeatably
stable. Cross-arch artifacts are useful inventory coverage, but they are not execution proof unless
they run on a native or otherwise validated runner.


## Version And Tag Policy

The C runtime version, Python package version, release notes, and tag must agree before publication.
During development, the Python package may use a development version such as:

```text
0.4.0.dev0
```

For an RC, set the Python package metadata to the intended PEP 440 release-candidate version, for
example:

```text
0.4.0rc1
```

Use tag names that make the artifact identity obvious:

```text
v0.4.0rc1
v0.4.0rc2
v0.4.0rc3
v0.4.0
```

Record the exact commit in the RC notes.


## Artifact Flow

Build artifacts in this order:

1. native build and tests;
2. source archive candidate;
3. platform wheels;
4. installed wheel smokes;
5. documentation site;
6. gallery and screenshot media;
7. release notes and known-issues page.

Keep wheel building separate from upload. The wheel build workflow produces artifacts. A later
manual upload workflow should publish vetted artifacts to TestPyPI or PyPI.

The candidate state under `build/release/<version>/` is the local handoff between artifact
creation, machine validation, release reports, and later publication automation. If an artifact
checksum changes after validation begins, rerun validation or discard the stale evidence.

Create a portable validation pack for physical validation machines:

```sh
just release-validation-pack 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl
just release-machine-plan 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl
```

Copy the generated `datoviz-0.4.0rc1-validation.tar.gz` to each target machine listed by
`release-machine-plan`. After extraction, run:

```sh
./validate-rc.sh
```

On Windows PowerShell:

```powershell
./validate.ps1 -Profile rc
```

If the target machine already has the repository checkout, it may instead run the
installed-artifact validator directly against the exact wheel or source artifact under review:

```sh
just release-machine-validate 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl --profile rc
```

Use `quick` for a short install/import/CLI smoke, `rc` for CMake consumer evidence, and `full` for
shaderc plus render smoke on a graphics-capable host. The validator writes environment metadata,
logs, failures, and `evidence.json` under `build/release/<version>/evidence/<machine-id>/`.

After installed validation, run the shared
[physical release-validation procedure](release-physical-validation.md)
using its platform section. It requires the isolated candidate-wheel Quickstart and a fixed,
maintainer-guided native interaction set. The wheel does not currently package the rich interactive
examples, so record the exact-wheel Quickstart separately from the checkout-built live examples;
neither may be substituted for the other.

Return each machine evidence directory as-is or as a tarball:

```sh
./archive-evidence.sh <machine-id>
```

On Windows PowerShell, use `./archive-evidence.ps1 -MachineId <machine-id>`.

Ingest returned evidence in the release checkout:

```sh
just release-ingest-evidence 0.4.0rc1 path/to/evidence-<machine-id>.tar.gz
just release-machine-plan 0.4.0rc1
just release-report 0.4.0rc1 --strict-matrix
```

Before any publication rehearsal, generate the release safety summary and local checksum artifacts:

```sh
just release-gates 0.4.0rc1 --write-artifacts --strict-matrix
```

This writes `build/release/<version>/release-report.md`, `SHA256SUMS`, and `SHA512SUMS`. A nonzero
exit means the gate summary still has blocking failures or missing required machine evidence.


## TestPyPI And PyPI

Use TestPyPI for release-candidate upload rehearsal. A TestPyPI upload should happen only after
local and CI wheel validation pass for the intended matrix.

For a local single-platform rehearsal:

```sh
just testpypi-check <host-platform-tag>
just testpypi-upload <host-platform-tag> dist yes
```

For a complete wheelhouse:

```sh
just testpypi-check-all wheelhouse
just testpypi-upload-all wheelhouse yes
```

The release automation wrapper keeps the same irreversible-action split:

```sh
just release-testpypi 0.4.0rc1 --dry-run --dist-dir dist
just release-testpypi 0.4.0rc1 --dist-dir dist --confirm yes
```

It refuses to upload unless the release gates are satisfied, or the maintainer explicitly passes
`--allow-incomplete` for a rehearsal with known gaps.

Before PyPI:

1. compare artifact names and versions against the intended tag;
2. verify wheel contents and native dependency inventory;
3. install from the built wheel into a clean environment;
4. run the CMake consumer smoke;
5. verify the optional Qt probe behavior;
6. confirm source archive build/install behavior;
7. record checksums if the RC process requires them.

The final PyPI wrapper is also dry-run first:

```sh
just release-pypi 0.4.0 --dry-run
just release-pypi 0.4.0 --confirm yes
```

It requires a recorded TestPyPI rehearsal unless the maintainer explicitly passes
`--allow-incomplete`. Prerelease uploads need `--allow-prerelease`.

Final PyPI upload should be manual and irreversible. Do not combine build, test, and final upload in
one unattended workflow.


## Release Notes

Every RC note should include:

1. exact commit and tag;
2. feature status table;
3. known issues;
4. platform validation matrix;
5. tests and static-analysis summary;
6. docs and gallery build links;
7. wheel and source artifacts;
8. migration/status notes from v0.3 and development snapshots;
9. citation status, including whether the Zenodo DOI is final or still pending;
10. JOSS draft or submission status;
11. feedback request targeted at users and contributors.

Keep known issues honest. If a platform or optional provider is untested, record it as untested
rather than supported.

Generate the first draft from release state and git history:

```sh
just release-notes 0.4.0rc1
```

Review `build/release/0.4.0rc1/release-notes.md` and replace generated commit grouping with
user-facing highlights before GitHub draft approval.

The current RC1 draft lives in [v0.4.0rc1 release notes](../releases/v0.4.0rc1.md). Keep it as a
draft until the final commit, tag, artifact URLs, checksums, and platform validation matrix are
filled in.

For final `v0.4.0`, enable or verify GitHub-Zenodo archiving before creating the GitHub release.
After Zenodo archives the release, update `CITATION.cff`, [Citation](../reference/citation.md), and
the final release notes with the exact version DOI, concept DOI, and release date. Do not add a
`.zenodo.json` file unless release-specific Zenodo metadata such as grants, communities, or related
identifiers is needed; if it is added, keep it consistent with `CITATION.cff`.


## What Stays Manual

Keep these as explicit maintainer actions for v0.4:

1. approving movement of draft GitHub Actions workflows into `.github/workflows/`;
2. approving RC and final tag creation;
3. approving TestPyPI upload;
4. approving PyPI upload;
5. approving GitHub draft creation and release publication;
6. approving `data` submodule pointer updates;
7. approving generated binary payloads or vendored runtime libraries;
8. final release-note wording and known-issues classification.

After these approvals exist, tools may execute the corresponding commands and API calls. The
decision remains manual; the mechanics should be automated.

For GitHub release drafts, use a dry run first:

```sh
just release-github-draft 0.4.0rc1 --dry-run
just release-github-draft 0.4.0rc1 --confirm yes
```

The command creates or updates a draft release and uploads the recorded source bundle, wheels,
validation pack, release report, and checksum artifacts. It does not publish the release.

Final tag creation, GitHub publication, and docs publication are separate approval-gated commands:

```sh
just release-dry-run 0.4.0 --wheel path/to/datoviz-0.4.0-...whl --dist-dir dist --write-report
just release-create-tag 0.4.0 --dry-run
just release-create-tag 0.4.0 --confirm yes
just release-github-publish 0.4.0 --dry-run
just release-github-publish 0.4.0 --confirm yes
just release-docs-publish 0.4.0 --dry-run
```

`release-github-publish` only publishes an existing draft. `release-docs-publish` is a placeholder
until a concrete docs deployment command is supplied with `--command`.


## After Each RC

After publishing an RC:

1. verify that artifacts are downloadable;
2. install the published wheel in a fresh environment;
3. check the documentation site link;
4. open or update issues for every reported blocker;
5. update `agents/now/STATUS.md` and release notes with the new state;
6. decide whether the next step is RC feedback, RC2/RC3, or final.

Do not start the next RC until feedback and known failures from the previous RC are triaged.
