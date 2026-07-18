# Release Validation Campaigns

Status: active v0.4 release-validation contract.

This document extends [ARTIFACT_EVIDENCE.md](ARTIFACT_EVIDENCE.md),
[PHYSICAL_VALIDATION.md](PHYSICAL_VALIDATION.md), and
[VALIDATION_MATRIX.md](VALIDATION_MATRIX.md). A campaign binds unattended cloud evidence and
physical-machine evidence to one immutable wheel workflow run.


## Campaign Identity

A campaign is identified by:

1. release version;
2. wheel workflow run ID and URL;
3. wheel workflow `headSha`;
4. the SHA-256 digest of every wheel used by validation; and
5. the validation-tool commit, which may descend from the artifact commit.

The wheel and validator commits are deliberately separate. New validation tooling may inspect an
unchanged candidate artifact, but a product, packaging, example, or build change requires new
wheels. Reports must show both commits.


## Shared Automated Contract

Cloud and physical machines run the same unattended profile from the installed candidate wheel.
The profile covers wheel identity, Release mode, imports, embedded shaders, runtime shader
compilation when packaged, native dependencies, installed CMake/Python/C consumers, offscreen
rendering, capture, repeated execution, and deterministic capture checks.

Physical mode is the unattended profile plus a human-guided interaction phase. It is not a smaller
or independent smoke test. An unattended failure blocks physical approval unless the maintainer
explicitly runs the manual phase for diagnosis; diagnostic manual work does not count as approval.


## Environment Classes

Every evidence bundle declares both execution and graphics classes. Supported execution classes
are:

- `physical-interactive`;
- `physical-unattended`;
- `self-hosted-remote`;
- `github-hosted-hardware-gpu`;
- `github-hosted-software-gpu`; and
- `github-hosted-no-gpu`.

Graphics metadata records the Vulkan loader, ICD, API, physical device, driver, and whether the
render path is hardware or software. Synthetic interaction is reported separately from human
interaction. Cloud evidence never satisfies a physical-machine or human-observation requirement.


## Evidence Bundle

Each machine writes a self-contained directory containing:

```text
evidence/<machine-id>/
  evidence.json
  environment.json
  failures.md
  logs/
  captures/
  manual-observations.json
```

`evidence.json` records campaign identity, environment class, unattended results, capture
fingerprints, explicit skips, failures, and manual state. Manual state progresses through
`not-applicable`, `pending`, `approved`, or `rejected`. Only an explicit maintainer decision may
produce `approved`.

Captured images record dimensions, PNG color type, byte size, SHA-256, nontransparent coverage,
per-channel extrema and means, and the digest of a repeated render. Exact equality is required for
repeat captures in one fixed environment. Cross-driver images are displayed together and compared
with structural statistics; they are not required to have identical hashes.


## Cloud Workflow

The conformance workflow accepts an exact Wheels run ID. Each platform job downloads the immutable
wheel artifact from that run, creates a fresh environment, runs the shared unattended profile, and
uploads short-lived evidence. An `if: always()` aggregation job validates every returned bundle and
uploads one durable report artifact containing:

```text
index.html
report.json
manifest.json
platform evidence, logs, and full-resolution captures
```

The aggregate report is produced even when a platform fails. A final gate fails only after the
diagnostic report has been uploaded.


## Physical Campaign Flow

The release coordinator is also a physical validation worker and must submit evidence for its own
machine. Other machines discover the active campaign, download the matching wheel, run unattended
validation, complete the guided interaction set, and submit a checksummed bundle after explicit
maintainer approval.

Durable campaign state must survive agent sessions and machine restarts. The release coordinator
may poll submitted evidence and regenerate the HTML report at any time. Missing and unavailable
physical classes remain visible and are never inferred from cloud success.


## Human Interaction Set

The attended phase reuses automated scenario identities and adds real presentation and input proof:

1. 2D resize, pan, and zoom;
2. 3D rotate and zoom;
3. text/layout inspection during resize;
4. image probe or color-scale interaction;
5. textured mesh depth and texture inspection;
6. picking/query feedback; and
7. normal close plus one reopen.

The agent launches one example at a time and records the maintainer's observation. Process exit,
PNG output, or synthetic input cannot be promoted to a human pass.


## Report Gates

Reports calculate separate gates for hosted artifact conformance, hosted rendering, physical
unattended validation, physical human interaction, required machine coverage, and evidence
integrity. The overall release gate passes only when every required category passes or has an
explicit release-policy disposition.
