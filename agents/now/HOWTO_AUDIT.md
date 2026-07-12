# How-To Audit Acceptance Checklist

Status: active remediation. Scope: every file under `docs/how-to/` plus How-To navigation and
validation tooling. This checklist records the July 2026 pre-RC audit acceptance criteria. Remove
this file after every item is closed and durable facts are captured in public docs, tests, or specs.


## A. RC Correctness And Status

- [ ] Fix nested `DvzCameraDesc` fields in `configure-cameras.md`.
- [ ] Fix nested camera fields and remove the nonexistent public orbit controller from
  `coordinate-systems.md`.
- [ ] Replace private `dvz_snprintf()` in `render-offscreen.md` with public standard C.
- [ ] Reconcile `use-ipython.md` with the active terminal-IPython close-hang investigation.
- [ ] Document explicit Qt enablement, prerequisites, provider discovery, installed smoke, and
  platform-specific bridge libraries in `embed-in-qt.md`.
- [ ] Describe Qt as `supported, optional provider`, while noting configuration complexity.
- [ ] Distinguish supported native surfaces from the WebGPU matrix label `native-only` in window
  and offscreen pages.
- [ ] Repair the empty `Build and Serve` section in `deploy-to-web.md`.
- [ ] State the GSP/VisPy2 ownership boundary without implying a currently available stable plotting
  package.
- [ ] Label `v0.4-dev` FetchContent use as development-only and leave an explicit RC-tag
  reconciliation point.


## B. Snippet Contracts And Reliability

- [ ] Define and apply `Complete example`, `Excerpt`, and `Call sequence` labels consistently.
- [ ] Add prerequisites, expected result, and one canonical complete example to substantive pages.
- [ ] Harden canonical capture, video, record/replay, sampled-field, annotation, and adornment
  sequences with pointer/result checks and cleanup where appropriate.
- [ ] Prefer public descriptor constructors/default helpers in sampled-field examples.
- [ ] Correctly describe partial Python and C scene snippets; do not present fragments as complete
  runnable programs.
- [ ] Reduce independently maintained duplicate scene snippets by linking canonical executable
  examples.
- [ ] Mark source-checkout commands and distinguish them from installed-user commands.
- [ ] Add mechanical validation for Python syntax, public C symbols/types/constants, and selected
  complete compiled snippets.


## C. Information Architecture

- [ ] Add `docs/how-to/index.md` with outcome-based routing and a recommended first path.
- [ ] Add the How-To overview to navigation while keeping clear task groups.
- [ ] Establish a visible progression from input events to picking, probing, and selection.
- [ ] Improve the four moved-page stubs with specific titles, canonical links, and search guidance
  or real redirect support.
- [ ] Rank one canonical complete example before optional related showcases.
- [ ] Keep detailed status matrices authoritative in reference/generated pages rather than copying
  them into task pages.


## D. Python Parity

- [ ] Add or clearly route tested Python workflows for visual updates.
- [ ] Add or clearly route tested Python workflows for panzoom.
- [ ] Add or clearly route tested Python workflows for multiple panels.
- [ ] Add or clearly route tested Python workflows for colormaps.
- [ ] Add or clearly route tested Python workflows for axes.
- [ ] Add or clearly route tested Python workflows for offscreen rendering.
- [ ] Add or clearly route tested Python workflows for screenshots.
- [ ] Add or clearly route tested Python workflows for animation.


## E. Consolidation And Advanced Routing

- [ ] Make camera setup, coordinate interpretation, and controller choice separate authoritative
  tasks without repeated contracts.
- [ ] Make offscreen rendering, screenshot capture, and size policy separate authoritative tasks
  without repeated lifecycle prose.
- [ ] Keep deployment, WebGPU diagnosis, and platform diagnosis distinct and remove repeated route
  and status explanations.
- [ ] Keep scene creation, initial visual upload, and visual update pages distinct and remove repeated
  batching guidance.
- [ ] Move or clearly demote NVENC/provider detail from the portable video-export workflow.
- [ ] Route record/replay, raw ctypes, Qt configuration, and runtime internals as advanced or
  specialized without mislabeling their supported status.


## F. Editorial And Accessibility Polish

- [ ] Standardize headings and navigation labels to sentence case.
- [ ] Replace generic image alt text with outcome-specific descriptions.
- [ ] Make dense tables usable on narrow displays.
- [ ] Add result-proving screenshots selectively rather than decoratively.
- [ ] Ensure every substantive page has useful next steps.
- [ ] Reconcile all exact package commands, tags, release links, and status wording at the RC cut.


## G. Final Proof

- [ ] Inventory all How-To pages and confirm every substantive page was re-reviewed.
- [ ] Run Python fence syntax validation.
- [ ] Run public C symbol/type/constant validation and selected compiled snippet checks.
- [ ] Run `just ctypes-check` after Python binding-facing documentation changes.
- [ ] Run the strict documentation build without retaining generated spillover from unrelated work.
- [ ] Run `git diff --check` before every checkpoint commit.
- [ ] Run a fresh multi-agent editorial, technical, and freshness audit.
- [ ] Close every checklist item as fixed, intentionally deferred, or rejected with recorded reason.
