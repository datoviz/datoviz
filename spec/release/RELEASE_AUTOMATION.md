# Release Automation Contract

This document defines the v0.4 release automation shape. It is a contract for maintainer tools and
agents, not a promise that every command exists yet.


## Goals

The release process should be boring to execute and hard to publish accidentally.

1. Maintainers decide when to create candidates, approve irreversible actions, and accept known
   issues.
2. Tools run checks, build or collect artifacts, compute checksums, create reports, and call
   publication APIs after approval.
3. Agents follow the same machine-readable state as humans, ask for approval at gates, and avoid
   reconstructing release state from conversation.
4. Physical-machine validation uses near-final artifacts, not an arbitrary development checkout.
5. Every release candidate has an evidence bundle that can explain what was tested, where, and
   with which artifacts.


## Phases

### Candidate

`release-candidate` creates a local release state directory for one version. It may run local
preflight checks, create the source bundle, collect wheel metadata from `dist/`, and write the first
`release-state.json`.

Candidate creation must not tag, upload, publish, push, or mutate a GitHub release.

### Machine Validation

Each validation host installs the candidate artifacts into a clean environment and runs a selected
profile. The validator writes an evidence bundle with environment metadata, command results,
captures, logs, skips, and failures.

The required v0.4 validation hosts are listed in [VALIDATION_MATRIX.md](VALIDATION_MATRIX.md). A
missing host is recorded as missing evidence, not silently treated as support.

### Report

`release-report` merges local state and machine evidence into a maintainer-facing summary. The
report should show artifact identity, command outcomes, platform coverage, visual/capture
differences, known skips, and remaining approval gates.

### Rehearsal

TestPyPI and draft GitHub releases are rehearsals. They require explicit approval, but they are not
the final publication gate. Rehearsal commands may upload to TestPyPI or create a GitHub draft
release only after the candidate evidence is coherent.

### Publication

Publication commands execute maintainer-approved irreversible actions:

1. create or push the final tag;
2. upload to PyPI;
3. publish the GitHub release;
4. publish documentation;
5. publish announcements.

Each irreversible class of action must require an explicit approval argument, normally `yes`, and
must refuse if artifact checksums or release identity changed after validation.


## Approval Gates

Agents and commands may proceed without maintainer approval for:

1. read-only inspection;
2. local preflight checks;
3. local candidate artifact creation;
4. local report generation;
5. dry-run publication planning.

Agents and commands must ask for approval before:

1. changing release versions;
2. creating tags;
3. pushing commits, tags, or branches;
4. uploading to TestPyPI, PyPI, or another package registry;
5. creating or publishing GitHub releases;
6. publishing documentation;
7. updating `data` submodule pointers;
8. staging generated/runtime binary payloads or vendored runtime libraries;
9. accepting a failed required machine as a known release exclusion.


## Command Surface

The intended maintainer command surface is:

```sh
just release-plan 0.4.0rc1
just release-dry-run 0.4.0rc1 --wheel path/to/wheel.whl
just release-candidate 0.4.0rc1
just release-validation-pack 0.4.0rc1 --wheel path/to/wheel.whl
just release-machine-validate 0.4.0rc1
just release-ingest-evidence path/to/evidence.tar.zst
just release-report 0.4.0rc1 --strict-matrix
just release-gates 0.4.0rc1 --write-artifacts --strict-matrix
just release-testpypi 0.4.0rc1 --dry-run --dist-dir dist
just release-testpypi 0.4.0rc1 --dist-dir dist --confirm yes
just release-github-draft 0.4.0rc1 --dry-run
just release-github-draft 0.4.0rc1 --confirm yes
just release-create-tag 0.4.0 --dry-run
just release-create-tag 0.4.0 --confirm yes
just release-pypi 0.4.0 --dry-run
just release-pypi 0.4.0 --confirm yes
just release-github-publish 0.4.0 --dry-run
just release-github-publish 0.4.0 --confirm yes
just release-docs-publish 0.4.0 --dry-run
```

`release-dry-run` is the agent-friendly front door: it chains the local plan, candidate dry-run,
validation-pack rehearsal when wheels are supplied, strict matrix reporting when state exists, and
all publication dry-runs without tagging, uploading, publishing, or pushing. `release-plan`,
`release-candidate`, `release-validation-pack`, `release-machine-validate`, and `release-report`
cover the lower-level local automation slices. Publication commands stay explicit approval-gated
operations.


## Agent Behavior

An agent running the release process should:

1. read this contract, [ARTIFACT_EVIDENCE.md](ARTIFACT_EVIDENCE.md), and
   [VALIDATION_MATRIX.md](VALIDATION_MATRIX.md);
2. run `just release-dry-run <version>` before mutating local release state;
3. run candidate/report commands and summarize failures in plain language;
4. give the maintainer exact per-machine validation commands;
5. ask for approval at every approval gate;
6. never tag, push, upload, publish, or approve known exclusions from inference alone.


## Publication Safety

Publication commands must check:

1. version identity across package metadata, release notes, tag, and artifacts;
2. current commit matches the candidate state;
3. artifact checksums match the candidate evidence;
4. required validation evidence is present or explicitly waived;
5. `git diff --check` passes;
6. `git status --short` has no unreviewed staged surprises;
7. no unapproved `data` gitlink update is staged;
8. no unapproved generated/runtime binary payload is staged.

Final PyPI and GitHub publication should be separate commands. A single unattended command must not
build, validate, tag, upload, and publish the final release.
