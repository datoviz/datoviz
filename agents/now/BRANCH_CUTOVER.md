# Post-RC2 Branch Cutover

Status: required post-RC2 plan; external branch operations require later approval of the exact actions. Updated: 2026-07-20.

## Goal

Preserve the old v0.3 `main` line as `v0.3-maintenance`, rename the active v0.4 `v0.4-dev` line to `main`, make the renamed v0.4 line the GitHub default, and reconcile live automation and guidance without merging incompatible histories, rewriting commits, moving tags, or force-updating release refs.

## Current State

- GitHub default and remote HEAD: `v0.4-dev`.
- Active v0.4 development branch: `v0.4-dev`.
- Old v0.3 branch: `main`.
- Intended v0.3 maintenance branch: not yet created.
- Additional `dev` branch: retained pending an explicit ownership and disposition audit.

## Preparation

1. Require a clean source worktree except explicitly ignored user files, fetch all remotes and tags, and record the exact tips of `origin/main`, `origin/v0.4-dev`, `origin/dev`, and all v0.4 RC tags.
2. Record the current GitHub default branch, branch protections or rulesets, required checks, environments, Pages settings, open pull-request base branches, scheduled or queued workflows, and external integrations that name either branch.
3. Audit live branch-name references in `.github/`, `README.md`, `docs/`, `mkdocs.yml`, `justfile`, `justfiles/`, packaging recipes, release tooling, badges, clone commands, contribution guidance, and deployment scripts.
4. Classify references before editing: live instructions and automation must change; tag-pinned RC1/RC2 links, release evidence, and intentional historical prose must remain unchanged.
5. Decide the `dev` branch disposition explicitly; do not delete, rename, or repoint it merely because it appears old.
6. Announce or enforce a short push freeze for the exact cutover window if other contributors or automation could advance either branch.

## External Cutover Sequence

Execute only after the maintainer approves the recorded tips and exact repository-setting operations:

1. Rename the old v0.3 `main` branch to `v0.3-maintenance` and verify that its tip is byte-for-byte unchanged.
2. Rename the default `v0.4-dev` branch to `main` and verify that its tip is byte-for-byte unchanged.
3. Confirm that GitHub sets the renamed v0.4 `main` as the default and that branch rules, pull-request bases, environments, Pages, and Actions permissions remain correct.
4. Do not merge `v0.3-maintenance` into the new `main`, force-push either line, rewrite history, or move RC tags.

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
