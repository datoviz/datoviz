# How-To Audit Acceptance Checklist

Status: complete. Scope: every file under `docs/how-to/` plus How-To navigation and validation
tooling. This checklist records the July 2026 pre-RC audit acceptance criteria and closure proof.
The exact RC tag/package command remains a release-cut substitution: current public prose marks
`v0.4-dev` as development-only and requires the published tag rather than inventing one early.


## A. RC Correctness And Status

- [x] Fix nested `DvzCameraDesc` fields in `configure-cameras.md`.
- [x] Fix nested camera fields and remove the nonexistent public orbit controller from
  `coordinate-systems.md`.
- [x] Replace private `dvz_snprintf()` in `render-offscreen.md` with public standard C.
- [x] Reconcile `use-ipython.md` with the active terminal-IPython close-hang investigation.
- [x] Document explicit Qt enablement, prerequisites, provider discovery, installed smoke, and
  platform-specific bridge libraries in `embed-in-qt.md`.
- [x] Describe Qt as implemented but source-build-only in RC1, while noting configuration
  complexity and the packaged-provider plan, now scheduled for RC3.
- [x] Distinguish supported native surfaces from the WebGPU matrix label `native-only` in window
  and offscreen pages.
- [x] Repair the empty `Build and Serve` section in `deploy-to-web.md`.
- [x] State the GSP/VisPy2 ownership boundary without implying a currently available stable plotting
  package.
- [x] Label `v0.4-dev` FetchContent use as development-only and leave an explicit RC-tag
  reconciliation point.


## B. Snippet Contracts And Reliability

- [x] Define and apply `Complete example`, `Excerpt`, and `Call sequence` labels consistently.
- [x] Add prerequisites, expected result, and one canonical complete example to substantive pages.
- [x] Harden canonical capture, video, record/replay, sampled-field, annotation, and adornment
  sequences with pointer/result checks and cleanup where appropriate.
- [x] Prefer public descriptor constructors/default helpers in sampled-field examples.
- [x] Correctly describe partial Python and C scene snippets; do not present fragments as complete
  runnable programs.
- [x] Reduce independently maintained duplicate scene snippets by linking canonical executable
  examples.
- [x] Mark source-checkout commands and distinguish them from installed-user commands.
- [x] Add mechanical validation for Python syntax, public C symbols/types/constants, and selected
  complete compiled snippets.


## C. Information Architecture

- [x] Add `docs/how-to/index.md` with outcome-based routing and a recommended first path.
- [x] Add the How-To overview to navigation while keeping clear task groups.
- [x] Establish a visible progression from input events to picking, probing, and selection.
- [x] Improve the four moved-page stubs with specific titles, canonical links, and search guidance
  or real redirect support.
- [x] Rank one canonical complete example before optional related showcases.
- [x] Keep detailed status matrices authoritative in reference/generated pages rather than copying
  them into task pages.


## D. Python Parity

- [x] Add or clearly route tested Python workflows for visual updates.
- [x] Add or clearly route tested Python workflows for panzoom.
- [x] Add or clearly route tested Python workflows for multiple panels.
- [x] Add or clearly route tested Python workflows for colormaps.
- [x] Add or clearly route tested Python workflows for axes.
- [x] Add or clearly route tested Python workflows for offscreen rendering.
- [x] Add or clearly route tested Python workflows for screenshots.
- [x] Add or clearly route tested Python workflows for animation.


## E. Consolidation And Advanced Routing

- [x] Make camera setup, coordinate interpretation, and controller choice separate authoritative
  tasks without repeated contracts.
- [x] Make offscreen rendering, screenshot capture, and size policy separate authoritative tasks
  without repeated lifecycle prose.
- [x] Keep deployment, WebGPU diagnosis, and platform diagnosis distinct and remove repeated route
  and status explanations.
- [x] Keep scene creation, initial visual upload, and visual update pages distinct and remove repeated
  batching guidance.
- [x] Move or clearly demote NVENC/provider detail from the portable video-export workflow.
- [x] Route record/replay, raw ctypes, Qt configuration, and runtime internals as advanced or
  specialized without mislabeling their supported status.


## F. Editorial And Accessibility Polish

- [x] Standardize headings and navigation labels to sentence case.
- [x] Replace generic image alt text with outcome-specific descriptions.
- [x] Make dense tables usable on narrow displays.
- [x] Add result-proving screenshots selectively rather than decoratively.
- [x] Ensure every substantive page has useful next steps.
- [x] Reconcile all exact package commands, tags, release links, and status wording at the RC cut.


## G. Final Proof

- [x] Inventory all How-To pages and confirm every substantive page was re-reviewed.
- [x] Run Python fence syntax validation.
- [x] Run public C symbol/type/constant validation and selected compiled snippet checks.
- [x] Run `just ctypes-check` after Python binding-facing documentation changes.
- [x] Run the strict documentation build without retaining generated spillover from unrelated work.
- [x] Run `git diff --check` before every checkpoint commit.
- [x] Run a fresh multi-agent editorial, technical, and freshness audit.
- [x] Close every checklist item as fixed, intentionally deferred, or rejected with recorded reason.


## Closure Evidence

- Checkpoint `232855a30` fixed the RC correctness, status, navigation, and moved-page findings.
- Checkpoint `9460cbed4` added Python parity, snippet reliability, consolidation, accessibility, and
  the first mechanical How-To checker.
- The final reconciliation strengthened that checker to cover indented MkDocs tabs, public C
  declarations, and balanced call arity; it also fixed the stale `dvz_panel_position_to_data()`
  call exposed by the stronger validation.
- `just check-howto-snippets` covers every current Python and C fence. Context excerpts are not
  falsely compiled as standalone programs; `just quickstart-check` supplies compile/run proof for
  the canonical complete first-program fixtures, while How-To pages link canonical built examples.
- `just ctypes-check`, `just docs-build-check`, `just quickstart-check`, and `git diff --check`
  passed after the final edits.
- Fresh editorial, technical, and architecture/status audits re-read the assembled section. Their
  residual findings were fixed and verified with a final residual-only pass.
