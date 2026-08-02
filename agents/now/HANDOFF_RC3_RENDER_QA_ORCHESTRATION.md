# RC3 Render And QA Orchestration

Status: **approved and active**. Integration branch: `orchestrate/rc3-render-qa`. Updated: 2026-08-02.

This handoff coordinates two substantial pre-RC3 lanes: the remaining C source audit and the render-product/technique-composition refactor. It is written for one `sol-medium` primary orchestrator using bounded lower-cost subagents while the maintainer is away.

## Where These Documents Belong

The planning documents should not live only on `qa/rc3-source-audit`. They govern the release and a separate rendering branch, so their durable destination is the approved shared RC3 development base. The current `plan/rc3-render-qa-orchestration` branch is the review vehicle and intentionally starts from the QA mapping commit so it can refine `QA_SOURCE_AUDIT.md` without modifying QA history during review.

After maintainer approval, prefer this sequence:

1. choose the exact shared integration base, normally the then-current `v0.4-dev` plus approved prerequisite QA checkpoints;
2. cherry-pick or rebase the single approved planning checkpoint onto that base;
3. create separate QA and render worktrees from recorded commits;
4. do not merge the render implementation into `qa/rc3-source-audit` as its working branch;
5. preserve the QA mapping/fix history and integrate coherent QA checkpoints into the shared base through the primary integrator.

No branch push, pull request, GitHub comment, or other publication is authorized by this document.

## Entry Gate

Execution begins only after the maintainer approves:

1. the decisions in `spec/scene/proposals/active/RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md`;
2. the packet DAG in [HANDOFF_RENDER_PRODUCTS_REFACTOR.md](HANDOFF_RENDER_PRODUCTS_REFACTOR.md);
3. this branch/worktree, model, lock, integration, and QA invalidation policy;
4. the exact base commits and branch names;
5. whether the render lane includes issue #137 directly or requires a preceding narrow stabilization patch.

The maintainer resolved this gate on 2026-08-02 with “ok i approve everything”, then authorized execution with “ok so let's start and stop when you need my approval”. This approves all recommended decisions, the R0-R9 packet DAG, the two-lane orchestration policy, the recommended branch/worktree names, issue #137 directly in R7 without a preceding blur-only patch, and local checkpoint commits. It does not authorize pushes or publication. The integration parent is fetched `origin/v0.4-dev` commit `2751887de`; approved QA and planning checkpoints were replayed on `orchestrate/rc3-render-qa` before the execution base was frozen.

## Roles And Authority

### Primary Orchestrator

Use one `sol-medium` primary agent. It owns:

- architecture interpretation and escalation;
- exact branch/worktree/base selection;
- packet assignment and overlap checks;
- shared locks and build scheduling;
- review of every subagent diff and test claim;
- public API, bindings, DRP2 authority changes, and integration decisions;
- staging and logical checkpoint commits;
- landing manifests, QA invalidation, final matrices, and user handoff.

The primary does not delegate commits or architecture decisions. It may delegate bounded edits only after their contracts and path ownership are frozen.

### Subagents

At most three subagents run alongside the primary because the available team has four total slots. Recommended allocation:

| Work | Model/effort | Mode |
| --- | --- | --- |
| Search, caller/test inventory, stale-reference sweep, plan/fixture comparison, generated-output diff review | `terra-low` | Read-only by default; narrow mechanical edits only with exact paths. |
| Focused tests, panel-coordinate plumbing after schema freeze, trace migration, shader translation after ABI freeze, isolated cleanup | `terra-medium` | One bounded writer in one worktree with an explicit path lease. |
| Independent correctness or ownership review of a completed packet | `terra-medium` | Read-only; no build unless granted the lane build lock. |
| Product schema, composer, resolve semantics, shared material lighting, API design, DRP2 change, integration | `sol-medium` primary | Never delegated as a decision. |

Escalate to maintainer review when a packet discovers an architecture/API/ownership decision outside the approved contracts. Do not silently raise subagent reasoning or let a cheap agent choose a new boundary.

## Branches And Worktrees

Names are recommendations; record exact resolved paths and commits before use.

| Purpose | Recommended branch | Recommended worktree | Writer |
| --- | --- | --- | --- |
| Reviewed planning/integration control | `orchestrate/rc3-render-qa` | main repository or dedicated controller worktree | Primary only. |
| QA source audit | `qa/rc3-source-audit` | sibling `datoviz-qa-rc3` | QA writer under primary review. |
| Rendering implementation | `refactor/rc3-render-products` | sibling `datoviz-render-products` | Render writer under primary review. |
| Temporary packet work | `task/rc3-render-<packet>` only when needed | disposable sibling worktree | One explicitly assigned subagent. |

Do not let two worktrees check out the same branch. Do not rebase or move a branch while an agent is working from it. Do not allow a QA slice to absorb a moving render branch; every slice records its base and head.

Each lane uses separate normal and sanitizer build directories. Never point two worktrees at the same configured build tree, generated-binding output, shader generation output, package staging directory, or recording/capture output.

## Two-Lane Schedule

### Lane A — QA Before Render Landing

The safe pre-landing audit order is:

1. `math` complete module;
2. `window` host/wrap/headless/GLFW ownership, callback, configuration, and metrics slice, while withholding final canvas/presentation integration sign-off;
3. `stream` registry and core state machine only, while deferring canvas/video attachment integration;
4. `video` encoder core, mux, provider selection, and provider-specific ownership slices only, while deferring `video_sink.c` and canvas/stream integration sign-off.

Defer `canvas`, `vk`, `vklite`, DRP2 backend/runtime, affected scene planning/runtime/technique/shader work, and `app` until the render landing stabilizes. Pure DRP2 wire/recording or isolated scene CPU work may be inspected read-only but should not be declared finally audited while public/shared contracts can still move.

QA checkpoint commits must not touch render-leased paths, public render headers, shared graph/DRP2 contracts, bindings, generated shaders, or render fixtures. A finding there becomes a cross-lane record for the primary, not an opportunistic QA patch.

### Lane B — Rendering Refactor

Execute R0 through R9 from [HANDOFF_RENDER_PRODUCTS_REFACTOR.md](HANDOFF_RENDER_PRODUCTS_REFACTOR.md). The render lane owns the changed rendering contracts and produces a landing manifest after every checkpoint, with a complete cumulative manifest at R9.

Do not run the affected QA audit against intermediate architecture checkpoints and call it final. Focused correctness tests and independent code review are required within the render lane, but formal affected-source-audit conclusions wait until the architecture lands and legacy paths are removed.

### Synchronization Points

1. **S0 — Base freeze:** planning approved, branches/worktrees created, path leases recorded, normal baselines green.
2. **S1 — QA CPU checkpoint:** `math` complete; safe window/stream/video slices may continue while render R0-R2 proceeds.
3. **S2 — Render schema freeze:** R1-R2 complete; API/product/phase shape reviewed by primary; R3/R4 work may fan out.
4. **S3 — Runtime convergence:** R3-R5 complete; no effect-name runtime identity remains in migrated paths; QA avoids integrating render-owned shared headers until this point.
5. **S4 — Semantic completion:** R6-R8 complete; AO/EDL/transparency/API behavior frozen; stop both writers for integration preparation.
6. **S5 — Render landing:** R9 complete, render branch tests green, cumulative landing manifest reviewed, render checkpoints integrated onto the shared base.
7. **S6 — Affected QA:** audit only mechanically invalidated slices plus all deferred modules, fix findings through normal QA checkpoints, and update the manifest.
8. **S7 — Final convergence:** integrate remaining QA fixes, run exact combined release-quality matrix, record limitations, and return to normal RC3 release sequencing.

## Shared Locks

The primary owns a simple cooperative lock table in its running handoff/status notes. An agent must receive a lock before acting and release it with exact output/status.

| Lock | Exclusive scope |
| --- | --- |
| `writer:<paths>` | Editing overlapping source/spec/test paths. Non-overlapping worktrees do not override a shared-header lease. |
| `build:normal:<lane>` | Configuring or building one lane's normal tree. Separate CPU-only compile locks may overlap only when host capacity is explicitly judged safe. |
| `build:sanitizer` | Any ASan/UBSan/LSan/TSan/MSan configuration or execution. |
| `gpu` | Vulkan, vklite, DRP2 runtime, offscreen GPU, live GLFW/WSI, validation-layer, CUDA, NVENC, or physical capture execution. |
| `shader-generated` | Shader compilation, registry regeneration, embedded shader outputs, or shader ABI updates. |
| `bindings` | `just ctypes`, binding policy/generator edits, generated binding output, ABI probes, and Python binding smoke. |
| `drp2-contract` | DRP2 prose, schema, fixtures, command metadata, packet/wire behavior, and WebGPU preflight updates. |
| `integration` | Rebases, cherry-picks, merges, conflict resolution, branch movement, or combined test preparation. All writers stop. |

Never run concurrent heavy builds, sanitizers, shaderc first-use tests, GPU/WSI tests, validation layers, CUDA/NVENC tests, binding generation, DRP2 fixture regeneration, or packaging. These are host-state or generated-output hazards even across separate worktrees.

## Agent Packet Template

The primary supplies every subagent this exact information:

```text
Objective:
Base commit and worktree:
Assigned paths:
Read-only dependencies:
Approved semantic decisions:
Required specs:
Allowed edits:
Explicit exclusions:
Deliverable:
Acceptance commands:
Held locks:
Stop and escalate when:
Do not stage, commit, push, publish, regenerate shared outputs, or run ungranted heavy tests.
Return files inspected/changed, findings, commands/results, limitations, and worktree status.
```

The primary reviews the actual diff, not only the subagent summary. It runs or independently verifies the narrow acceptance loop before staging a checkpoint.

## Commit And Integration Policy

Each lane has one committer: the primary orchestrator. A supervised writer may edit its leased paths but does not stage or commit.

Before every checkpoint:

1. inspect `git status --short`;
2. inspect all unstaged and staged diffs;
3. run the packet's focused checks and `git diff --check`;
4. stage only the coherent leased paths;
5. inspect `git diff --cached --stat` and the staged patch;
6. verify exclusion of `data`, generated/runtime binaries, vendored libraries, unrelated user changes, and cross-lane files;
7. commit with the packet's logical checkpoint message;
8. record commit, base, tests, limitations, and manifest delta.

Integrate completed QA fixes into the render/common base at deliberate synchronization points, not continuously. My preference is one pre-render integration after the safe CPU QA checkpoints and one post-render integration for affected QA fixes. If a QA fix touches a render dependency or shared public header, the primary decides whether to integrate it before R1, port it deliberately after R9, or escalate; no lane performs a blind merge.

Use non-interactive cherry-pick or rebase only after the primary has inspected the exact commits and stopped all writers with the `integration` lock. Resolve semantic conflicts in the owning lane, rerun both affected test sets, and preserve authorship/history. Do not push without a fresh explicit user request for that exact branch action.

## Landing Manifest And QA Invalidation

R9 must produce a machine-reviewable or Markdown landing manifest containing:

1. merge base, ordered render commits, and final head;
2. every changed public and shared internal header;
3. every changed module/path, generated input/output, shader, build definition, fixture, and test runner;
4. contract deltas for ownership, frame state, synchronization, resource identity, target layout, format/sample/resolve, row/pixel rules, callbacks/threading, errors, and recovery;
5. DRP2 prose/schema/fixture changes and WebGPU implications;
6. exact validation configuration and GPU/provider identities;
7. explicit unaffected claims for completed QA slices, justified against the merge base.

Invalidation is contract-based:

- A changed isolated implementation reruns that slice's static checks, focused normal/sanitizer tests, and direct boundary consumer tests.
- A public or shared-header change reruns every recorded slice that includes or relies on it, even when its source directory is textually unchanged.
- A DRP2 command/schema/semantic change reruns affected C tests, fixtures, `just drp2-fixtures`, WebGPU preflight, and scene emission for that command class.
- A shader/technique/resource-contract change reruns shader ABI/source guards, relevant vklite/DRP2 runtime, scene technique/emission, validated offscreen, and visual conformance.
- A canvas/window/swapchain/frame-lifecycle change reruns window/canvas/stream/video integration and bounded live presentation; offscreen proof is insufficient.
- A public API, binding policy, header, or generator change runs `just ctypes` and `just ctypes-check` before Python, GSP, or packaging proof.

The exact affected-slice matrix lives in [QA_SOURCE_AUDIT.md](QA_SOURCE_AUDIT.md). “Only affected modules” never means “only directories with changed text.”

## Failure And Recovery Protocol

1. A focused test failure stays with the active packet until diagnosed, recorded as a pre-existing baseline, or escalated.
2. A subagent that reaches an architecture, public API, ownership, DRP2, or scope decision stops and returns alternatives with evidence.
3. A conflicting user edit or staged stop-sign path stops staging and integration; preserve it and ask the user if it cannot be isolated.
4. A provider absence, tool stall, sanitizer incompatibility, or unavailable physical platform is recorded as a limitation, not a pass; continue with other authorized evidence.
5. A regression that cannot be isolated returns to the last green logical checkpoint through a new fix commit; do not use destructive resets or discard user work.

## Autonomous Stop Conditions

The primary stops the autonomous campaign and returns to the maintainer when:

1. an approved architecture decision must change;
2. a new public API direction or compatibility layer is proposed;
3. DRP2 needs a materially broader command/capability contract than planned;
4. scene would need backend-specific or Vulkan-native behavior;
5. render work changes window/canvas/app ownership beyond the landing manifest's anticipated scope;
6. exact target branches or prerequisite histories diverge ambiguously;
7. a stop-sign path is staged, user work overlaps required edits, or publication authority is needed;
8. the final result would retain parallel old/new rendering paths.

## Final Completion Gate

The two-lane campaign is complete when:

1. all approved render packets and safe/deferred QA slices have checkpoint evidence;
2. the render landing manifest has mechanically driven the affected QA rerun;
3. affected findings are fixed and reintegrated without legacy paths or cross-lane omissions;
4. the exact combined commit passes the proportional build, test, spec, binding, shader, DRP2/WebGPU, sanitizer, Vulkan/offscreen/live, multi-panel, MSAA, EDL, transparency, issue #137, and packaging-facing gates available on the host;
5. unavailable Windows/macOS/provider/physical evidence remains explicit in RC3 status;
6. `git diff --check`, staged-set inspection, and final worktree status are clean;
7. the primary reports exact commits, results, limitations, remaining release gates, and unpublished branch state to the maintainer.
