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
4. public documentation gates from `agents/now/DOCUMENTATION.md`;
5. wheel workflow details from [Release wheels](release-wheels.md);
6. the flight checklist from [Release flight checklist](release-flight-checklist.md).


## Pre-RC Preparation

Start from a clean understanding of the worktree:

```sh
git status --short
git diff --check
```

Resolve or explicitly ignore unrelated work before building release artifacts. Do not publish
artifacts from a tree with unreviewed staged changes, generated binary payloads, or an unintended
`data` submodule update.

Then run the minimum local proof:

```sh
just build
just test
just spec-check
```

For wheel artifacts, run the host-local package proof:

```sh
just wheel-ci-local <host-platform-tag>
```

Use the rebuild variant when the local native build configuration is known to be valid:

```sh
just wheel-ci-local <host-platform-tag> 1
```


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

Before PyPI:

1. compare artifact names and versions against the intended tag;
2. verify wheel contents and native dependency inventory;
3. install from the built wheel into a clean environment;
4. run the CMake consumer smoke;
5. verify the optional Qt probe behavior;
6. confirm source archive build/install behavior;
7. record checksums if the RC process requires them.

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
9. feedback request targeted at users and contributors.

Keep known issues honest. If a platform or optional provider is untested, record it as untested
rather than supported.


## What Stays Manual

Keep these as explicit maintainer actions for v0.4:

1. moving draft GitHub Actions workflows into `.github/workflows/`;
2. cutting RC and final tags;
3. publishing to TestPyPI;
4. publishing to PyPI;
5. creating GitHub releases;
6. approving `data` submodule pointer updates;
7. approving generated binary payloads or vendored runtime libraries;
8. final release-note wording and known-issues classification.


## After Each RC

After publishing an RC:

1. verify that artifacts are downloadable;
2. install the published wheel in a fresh environment;
3. check the documentation site link;
4. open or update issues for every reported blocker;
5. update `agents/now/STATUS.md` and release notes with the new state;
6. decide whether the next step is RC feedback, RC2/RC3, or final.

Do not start the next RC until feedback and known failures from the previous RC are triaged.
