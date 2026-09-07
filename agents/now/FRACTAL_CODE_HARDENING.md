# Fractal Autonomous Code Hardening

Status: planned, not executed. Prepared 2026-09-07 for the Fractal Linux machine. This campaign prioritizes coding, focused refactoring, and static/dynamic analysis. It is development evidence, not release approval or exact-artifact proof.

## Objective And Scope

Run a sustained autonomous campaign, initially targeting roughly two hours of useful work, to strengthen failure handling and resource ownership with reproducible tests and focused fixes. Finish a coherent checkpoint rather than starting another large task near the time limit. Do not spend time merely to consume tokens.

Read [dispatch](START.md), [current status](STATUS.md), the completed [differential campaign](QA_DIFFERENTIAL_CAMPAIGN.md), and applicable repository rules before implementation. Do not restart the completed broad source audit. Record the starting commit and inspect production changes since the differential campaign's validated integration before choosing affected paths.

Prioritize deterministic allocation-failure coverage for buffer/descriptor transactions: rollback, ownership, cleanup, retry behavior, and preservation of the previous valid state. The prior campaign explicitly leaves exhaustive allocation-failure injection incomplete. Confirm the gap still exists before implementing narrowly scoped test hooks. Improve coarse DRP2 diagnostics only as a secondary task if evidence supports a bounded change that preserves protocol contracts.

Refactor cleanup or transaction code when a reproduced defect or a concrete testability problem justifies it. Preserve existing subsystem boundaries and borrowed Vulkan ownership. Broad architectural redesign, new features, unrelated style cleanup, public prose rewriting, and release publication are outside this campaign.

## Autonomous Execution

1. Establish the Linux baseline: commit, working-tree state, compiler/tool versions, GPU/driver, Vulkan provider, available sanitizers, and build/test routes. Preserve unrelated local changes; use an isolated checkout/worktree if needed under repository hygiene rules.
2. Inventory affected production paths and existing regression coverage. Select a bounded ownership or transaction boundary, document acceptance criteria, and assign distinct worker responsibilities.
3. Review failure paths and reproduce credible defects. Add deterministic failure injection only where existing facilities cannot exercise the relevant allocation/unwind behavior. Test externally meaningful invariants rather than mirroring implementation.
4. Run differential clang-tidy and bounded cppcheck on relevant translation units. Inspect underlying diagnostics and distinguish actionable findings from provider, configuration, and advisory output.
5. Run focused CPU ASan/UBSan/LSan and selected Valgrind checks where available. Bound potentially hanging runs; provider-induced Vulkan/shaderc stalls remain inconclusive. Continue independent checks instead of repeatedly retrying an unchanged blocker.
6. Implement focused fixes and justified refactors, then run appropriate regression, contract, binding, and native validation. After public-header/API/binding changes, run `just ctypes` and `just ctypes-check` before Python-facing validation. Follow build/GPU locks and applicable validation rules.
7. Obtain a fresh worker review of each substantive final change, resolve findings, and make coherent local checkpoint commits after required checks pass. The coordinator owns integration and commits.
8. Finish with a durable evidence update in this file: exact commits, changes, commands, results, timeouts, limitations, and prioritized follow-ups. Run `git diff --check` and inspect repository status and the staged set before committing. Do not claim unavailable validation passed.

Routine implementation decisions and bounded local fixes should proceed without manual intervention. If a material scope decision or an explicit approval boundary blocks one lane, record it and continue independent authorized work. No push, workflow dispatch, upload, PR, or other external publication is authorized by this execution plan. The user's approval to publish this plan does not authorize publishing later implementation commits. Leave `data`, runtime libraries, generated binaries, and unrelated user files unstaged and uncommitted.

## Subagent Plan

Use at most three workers alongside the coordinator, subject to the runner's actual concurrency limit. This plan explicitly requests subagent delegation and the model/effort assignments below. Pass narrow task context and the relevant contracts when using model overrides; do not rely on inherited full conversation history.

| Role | Preferred model / effort | Bounded responsibility |
| --- | --- | --- |
| Coordinator | Current strong model | Scope, architecture decisions, shared infrastructure, integration, validation decisions, and commits |
| Failure-path reviewer | `gpt-5.6-sol`, high | Review one ownership/transaction boundary; report contract violations, reproducers, and proposed fixes |
| Analysis worker | `gpt-5.6-luna`, medium | Run prescribed analysis, deduplicate diagnostics, and report relevant findings with file locations and evidence |
| Test worker | `gpt-5.6-sol`, medium | Implement deterministic regression tests in an assigned area and run focused sanitizer checks |

Give every worker explicit files, acceptance criteria, expected output, and a bounded assignment. Assign disjoint edit ownership; the coordinator owns shared allocation/test infrastructure. Serialize expensive builds and GPU runs against shared outputs and hardware. Workers must not commit, publish, or modify another worker's files without coordination.

Rotate completed workers into new bounded tasks. Use a fresh review assignment for the final diff and contracts. Escalate difficult Vulkan lifetime and transactional-semantics questions to the coordinator or a stronger available model. If a named model is unavailable, use an available equivalent or the current model and record the substitution; model availability must not block useful work. Reduce task scope before reducing reasoning effort for correctness review.

## Separate Prose Campaign

The alternative broad prose pass is a separate future campaign, not an automatic fallback or part of this coding run. It would use a strong coordinator, two `gpt-5.6-luna` medium-effort editors with disjoint documentation sections, and a `gpt-5.6-sol` high-effort technical reviewer. Apply the available `humanizer` skill after locating and reading it on Fractal; its availability on the planning Mac does not establish Linux availability.

Calibrate against a small representative batch and existing documentation conventions before expanding. Preserve technical meaning, qualifications, API contracts, code examples, and terminology; humanizer is an editing guide, not a word blacklist. Start with user guides and API explanations. Respect the existing maintainer voice-review gate before broad course rewriting. Follow documentation gates and finish with an integrated terminology, link, and semantic review.

## Execution Results

Pending execution on Fractal. Record evidence here when the campaign runs.
