# Post-RC2 Branch Cutover

Status: complete. Updated: 2026-08-15.

## Goal

Preserve the old v0.3 `main` line as `v0.3-maintenance`, rename the active v0.4 `v0.4-dev` line to `main`, make the renamed v0.4 line the GitHub default, and reconcile live automation and guidance without merging incompatible histories, rewriting commits, moving tags, or force-updating release refs.

## Current State

- GitHub default and remote HEAD: `main`.
- Active v0.4 development branch: `main`.
- Preserved old v0.3 branch: protected `v0.3-maintenance`.
- Open PRs #132 and #136: retargeted to `main` with their head SHAs unchanged.
- Additional `dev` branch: legacy v0.3.6-development history; retain unchanged and frozen through the cutover.

## Execution Record

The maintainer approved the exact cutover operations on 2026-08-15. Immediately before mutation, the worktree was clean, no Actions runs were queued or active, and all audited mutable refs and settings matched the approval snapshot below.

| Ref or setting | Executed value |
| --- | --- |
| pre-cutover active v0.4 tip | `9cd3b0eee44928cd3602db48f79a3cb4ae31918e` |
| preserved `v0.3-maintenance` tip | `c1725df786dfdb7849f4a0d2928968aabfe002ec` |
| unchanged `dev` tip | `8857bb6b3a769c3c128f13e85c687362982f2a4e` |
| `v0.4.0rc1^{commit}` | `12da2a6019c2e73bf29734eed482fffcd1e376b3` |
| `v0.4.0rc2^{commit}` | `8a3bd75096bb1124707d2fd547de512c20211c85` |
| unchanged `data` gitlink | `a9542d20f2d29aecb9518738f6b7ba1914b63997` |

GitHub did not move the default branch atomically with the second rename: it briefly retained the now-missing `v0.4-dev` name. The approved fallback explicitly set `default_branch=main`, after which both PRs targeted `main`. Ruleset `17684715` continues to protect `~DEFAULT_BRANCH`; ruleset `19166518` is now named `protect-v0.3-maintenance` and targets `refs/heads/v0.3-maintenance`. Both branches report protected with deletion and non-fast-forward rules, and neither branch tip changed during the operation.

## Verification Record

The reconciliation landed in `5c5d5320f`, and CI correction commits `c2bb1389e` and `d3d7142f0` made push-mode wheel reuse safe across the renamed branch boundary. A fresh unqualified shallow clone checked out `main` at the expected tip with `data` uninitialized, while a fresh `--branch v0.3-maintenance` clone checked out exact preserved v0.3 tip `c1725df786dfdb7849f4a0d2928968aabfe002ec`. CircleCI accepted every reconciliation push with HTTP 200.

Exact validation head `d3d7142f0ec5ceb0f1cb7508f247d428c22e7afb` passed [Test run 31900610395](https://github.com/datoviz/datoviz/actions/runs/31900610395) on Linux, macOS, and Windows and [Wheel conformance run 31900601977](https://github.com/datoviz/datoviz/actions/runs/31900601977). Local validation passed generated-gallery drift, example manifests, strict documentation build, specifications and 125 DRP2 fixtures, 16 release-conformance tests, and `git diff --check`. Final verification confirmed `main`, `v0.3-maintenance`, `dev`, both RC tag commits, both PR bases, both rulesets, and the unchanged `data` gitlink all match the approved targets; no remote `v0.4-dev` parent branch remains.

## Audited Snapshot

Read-only preflight completed at `2026-08-03T12:27:21+02:00` after refreshing named branch refs and v0.4 tags. The local `origin/HEAD` cache was stale and has been refreshed to `origin/v0.4-dev`. A broad tag fetch also exposed an unrelated local/remote `nightly` tag collision; neither tag was changed, and the collision must not be resolved as part of the branch cutover.

| Ref | Audited object |
| --- | --- |
| `origin/v0.4-dev` | `a8caae27364049ab5e59d20832d798f2cc5e905b` |
| local `v0.4-dev` | `66e3f80719572b1784036d3f53d0bf77df04470f`, one unpublished cleanup commit ahead |
| `origin/main` | `c1725df786dfdb7849f4a0d2928968aabfe002ec` |
| `origin/dev` | `8857bb6b3a769c3c128f13e85c687362982f2a4e` |
| `v0.4.0rc1^{commit}` | `12da2a6019c2e73bf29734eed482fffcd1e376b3` |
| `v0.4.0rc2^{commit}` | `8a3bd75096bb1124707d2fd547de512c20211c85` |

The old `main` and active `v0.4-dev` lines have 60 and 5,919 unique commits respectively after their historical merge base. They must be renamed, never merged. The `dev` branch has three commits absent from old `main`: the v0.3.6-development version bump, a packaging update, and a mesh-texture example. Retaining it unchanged as a frozen legacy branch is the lowest-risk disposition; any later recovery into `v0.3-maintenance` requires a separate review.

The parent gitlink and checked-out `data` submodule both point to `a9542d20f2d29aecb9518738f6b7ba1914b63997`, which is the tip of `datoviz/data:v0.4-dev`. The `.gitmodules` branch setting and all `spec/data/` references therefore remain intentionally unchanged because the data repository is outside this cutover.

GitHub has no Pages deployment or configured environments for this repository. No Actions runs were queued or in progress during the audit. The active CircleCI webhook has broad repository events but no checked-in `.circleci` configuration; verify delivery after the rename without changing the hook. The scheduled nightly-pruning workflow follows the default branch and requires no authored branch-filter change.

Open PRs #132 and #136 both target `v0.4-dev`; verify that GitHub retargets both to the renamed `main`. PR #132 remains open while the original author's feedback on #136 is pending.

Two active rulesets apply deletion and non-fast-forward protection: `prevent-force-push-main` follows `~DEFAULT_BRANCH`, while `protect-v0.4-dev` explicitly names `refs/heads/v0.4-dev`. There are no required-status-check or review rules. After the rename, keep the default-branch ruleset on new `main` and repurpose the explicit ruleset as `protect-v0.3-maintenance` so the preserved v0.3 line receives the same deletion and force-push protection.

## Branch-Reference Audit

The audit found 792 `v0.4-dev` or `v0.3-maintenance` occurrences across 184 tracked files. They are classified as follows:

- 664 generated gallery/example occurrences across 117 files: change `tools/build_gallery.py` to emit `main`, then regenerate the manifest, gallery pages, validation gallery, and WebGPU matrix rather than hand-editing generated output.
- 100 occurrences across 55 live or semantic-review files: change current workflow filters, badges, clone and FetchContent guidance, repository source links, release-conformance defaults, website policy, active agent status, and other instructions that identify the live branch; preserve language that names the historical v0.4-dev development phase or protocol/API assignments.
- 11 occurrences across nine completed plans, tasks, and evidence files: preserve as historical records.
- Five occurrences across `.gitmodules` and `spec/data/`: preserve because they refer to `datoviz/data:v0.4-dev`.
- 12 occurrences in this cutover plan: update current-state wording after completion while retaining the recorded pre-cutover names and evidence.

Four workflows require an immediate authored update from `v0.4-dev` to `main`: `test.yml`, `wheel-conformance.yml`, `package-index-verification.yml`, and `physical-evidence-intake.yml`. The first post-rename reconciliation commit must contain these filters so its push and subsequent PRs exercise the new branch.

## Preparation

0. Before the cutover, complete the narrow core-independence prerequisite in [`../../spec/data/ASSET_ARCHITECTURE.md`](../../spec/data/ASSET_ARCHITECTURE.md). The reviewed Source/Noto family, deterministic Source atlas products, five primary roles, custom-font behavior, source-defined generic file fixtures, no-data build, isolated source-package install, hermetic source-archive documentation generation, and authored core/wheel CI removal of `data` and LFS hydration are complete. Remaining prerequisite proof runs the hosted core workflows and final wheel/release artifacts from the exact candidate with `data` uninitialized. Keep `datoviz/data` unchanged during the branch rename; data-backed website publication, downloader implementation, catalog publication, broad example/gallery migration, and submodule retirement remain separately reviewable post-cutover work rather than new RC3 cutover blockers.
1. Require a clean source worktree except explicitly ignored user files, fetch all remotes and tags, and record the exact tips of `origin/main`, `origin/v0.4-dev`, `origin/dev`, and all v0.4 RC tags.
2. Record the current GitHub default branch, branch protections or rulesets, required checks, environments, Pages settings, open pull-request base branches, scheduled or queued workflows, and external integrations that name either branch.
3. Audit live branch-name references in `.github/`, `README.md`, `docs/`, `mkdocs.yml`, `justfile`, `justfiles/`, packaging recipes, release tooling, badges, clone commands, contribution guidance, and deployment scripts.
4. Classify references before editing: live instructions and automation must change; tag-pinned RC1/RC2 links, release evidence, and intentional historical prose must remain unchanged.
5. Retain `dev` unchanged and frozen as legacy v0.3.6-development history; do not delete, rename, repoint, or merge it during this cutover.
6. Announce or enforce a short push freeze for the exact cutover window if other contributors or automation could advance either branch.

## External Cutover Sequence

Execute only after the maintainer approves the recorded tips and exact repository-setting operations. Refresh every tip immediately before execution because the snapshot above is evidence, not a lease on mutable refs:

1. Publish the local `v0.4-dev` through the committed audit record as an exact fast-forward, verify that validated cleanup commit `66e3f8071` is in its ancestry, refetch, and freeze pushes while confirming that neither source tip changed unexpectedly.
2. Rename the old v0.3 `main` branch to `v0.3-maintenance` and verify that `c1725df786dfdb7849f4a0d2928968aabfe002ec` remains its tip unless a newly audited pre-cutover tip supersedes this snapshot.
3. Rename the default `v0.4-dev` branch to `main` and verify that the approved, freshly recorded v0.4 tip remains byte-for-byte unchanged.
4. Confirm that GitHub sets the renamed v0.4 `main` as the default, both open PRs now target `main`, the default-branch protection follows `main`, and the explicit ruleset is updated to protect `v0.3-maintenance`.
5. Do not merge `v0.3-maintenance` into the new `main`, force-push either line, rewrite history, move RC tags, alter the `nightly` tags, or change the `data` gitlink.

## Repository Reconciliation

1. Update live Actions branch filters, workflow-dispatch examples, badges, source links, clone instructions, FetchContent guidance, contributor documentation, release recipes, and deployment validation to use the new `main`.
2. Replace live v0.3 maintenance instructions with `v0.3-maintenance` where appropriate.
3. Preserve `v0.4.0rc1`, `v0.4.0rc2`, and other tag-pinned links and historical evidence exactly.
4. Update `AGENTS.md`, `agents/now/START.md`, `agents/now/STATUS.md`, and `agents/now/RELEASE.md` to describe `main` as the active v0.4 line only after the external rename succeeds.
5. Keep the reconciliation commit focused and inspect every changed branch reference for intent rather than applying an unrestricted global substitution.

## Verification

1. Verify remote branch tips and RC tags against the recorded pre-cutover hashes.
2. Verify a fresh recursive clone with no branch argument checks out v0.4 `main` and passes the narrow repository/bootstrap smoke.
3. Verify a fresh recursive clone of `v0.3-maintenance` checks out the preserved v0.3 tree.
4. Verify required Actions trigger on the new `main`, branch protections and required checks apply, badges resolve, source links return success, and documentation/deployment recipes accept the new branch.
5. Search for residual `v0.4-dev` references and classify every result as stale or intentionally historical.
6. Verify no release, tag, package, website, or `data` state changed as an unintended side effect.

## Local Clone Migration

After the remote cutover, existing clones should fetch with pruning, rename their local v0.4 branch to `main`, set it to track `origin/main`, and fetch the preserved `v0.3-maintenance` branch only when needed. Exact commands must be reviewed against each clone's local branches and unpushed work before execution.

## Rollback Boundary

If verification exposes a settings or reference problem, repair the settings or reconciliation commit while preserving both recorded branch tips. Do not use history rewriting or force-push as rollback. Any proposal to reverse branch names must receive a new explicit approval after confirming that neither line advanced during the cutover.

## Completion Criteria

The cutover is complete when v0.4 is the default `main`, v0.3 is preserved as `v0.3-maintenance`, live automation and guidance use the intended names, fresh clones of both lines work, protections and required checks are active, historical links and release refs are unchanged, and the `dev` branch has an explicit recorded disposition.
