# Agent Release Checklist

This page tells an automation agent how to assist a Datoviz v0.4 release without taking over
maintainer decisions.


## Read First

Before running release commands, read:

1. `AGENTS.md`;
2. `agents/now/START.md`;
3. `agents/now/STATUS.md`;
4. `agents/now/RELEASE.md`;
5. `spec/release/RELEASE_AUTOMATION.md`;
6. `spec/release/ARTIFACT_EVIDENCE.md`;
7. `spec/release/VALIDATION_MATRIX.md`;
8. `docs/contributors/release-process.md`;
9. `docs/contributors/release-flight-checklist.md`.


## Local Candidate Flow

For a requested version such as `0.4.0rc1`:

```sh
just release-plan 0.4.0rc1
just release-dry-run 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl
just release-candidate 0.4.0rc1 --dry-run
```

Summarize the plan and ask the maintainer before running the non-dry candidate command. State your
preference with the ask.

After approval:

```sh
just release-candidate 0.4.0rc1
just release-notes 0.4.0rc1
just release-docs-validate 0.4.0rc1
just release-validation-pack 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl
just release-report 0.4.0rc1
```

Do not tag, push, upload, publish, create GitHub releases, or modify package registry state during
this local candidate flow.

Summarize the generated `build/release/<version>/release-notes.md` as a draft, not final wording.
Ask the maintainer to review known issues, highlights, artifact URLs, and validation status before
GitHub draft approval.
Treat a failed `release-docs-validate` result as a release blocker unless the maintainer explicitly
accepts it as a known exclusion.


## Machine Instructions

When the candidate report is ready, give the maintainer exact commands for each physical machine.
For the first automated evidence slice, use:

```sh
just release-validation-pack 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl
```

Then send the generated `build/release/<version>/validation-pack/datoviz-<version>-validation.tar.gz`
to each physical machine. After extraction, use `./validate-quick.sh`, `./validate-rc.sh`,
`./validate-full.sh`, or `./validate.ps1 -Profile rc`.

If the target machine has the repository checkout, it may instead run:

```sh
just release-machine-validate 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl --profile rc
```

Use `quick` for a short install/import/CLI smoke, `rc` for the required CMake consumer and
installed Python/C no-window example smokes, and `full` when the machine should also attempt
shaderc and installed offscreen render example smokes. Clearly separate required machines from
optional or unavailable machines.
Treat error-level diagnostics in machine evidence as blockers even when the underlying command
returned 0. Summarize warning-level diagnostics separately for maintainer review.

The instruction should identify:

1. candidate version and commit;
2. artifact path or URL;
3. requested profile: `quick`, `rc`, `full`, or `manual`;
4. expected evidence output path;
5. how to return the evidence bundle.

The default evidence output is:

```text
build/release/<version>/evidence/<machine-id>/
```

Ask the maintainer to return either that directory or a tarball:

```sh
tar -czf evidence-<machine-id>.tar.gz evidence/<machine-id>
```

Ingest returned evidence in the release checkout:

```sh
just release-ingest-evidence 0.4.0rc1 path/to/evidence-or-tar
just release-report 0.4.0rc1 --strict-matrix
just release-gates 0.4.0rc1 --write-artifacts --strict-matrix
```

Before asking for upload or GitHub draft approval, run dry rehearsals:

```sh
just release-dry-run 0.4.0rc1 --wheel path/to/datoviz-0.4.0rc1-...whl --dist-dir dist --write-report
just release-testpypi 0.4.0rc1 --dry-run --dist-dir dist
just release-github-draft 0.4.0rc1 --dry-run
just release-create-tag 0.4.0 --dry-run
just release-pypi 0.4.0 --dry-run
just release-github-publish 0.4.0 --dry-run
just release-docs-publish 0.4.0 --dry-run
```


## Approval Gates

Ask before every irreversible action:

1. version bump;
2. tag creation;
3. push;
4. TestPyPI upload;
5. PyPI upload;
6. GitHub draft creation;
7. GitHub release publication;
8. documentation publication;
9. `data` submodule pointer update;
10. acceptance of a failed required check as a known issue.

If a command requires a `yes` argument, do not add it unless the maintainer explicitly approved
that exact action in the current turn.


## Reporting

Lead with blockers:

1. failed commands;
2. missing required artifacts;
3. artifact checksum drift;
4. missing required machine evidence;
5. version identity mismatches;
6. unreviewed known issues.

Then summarize passed checks and next approval gates.
