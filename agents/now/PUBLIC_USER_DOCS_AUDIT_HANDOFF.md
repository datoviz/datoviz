# Public User Documentation Audit Handoff

Status: ready for execution by a future agent.

Scope: public website/user documentation only. Do not audit or rewrite durable specs, active
handoffs, task archives, or internal agent notes except when a public page links to them or exposes
their content.

Public-doc scope includes:

- `docs/index.md`
- `docs/start/`
- `docs/how-to/`
- `docs/reference/`
- `docs/examples/`
- `docs/examples/gallery/`
- `docs/releases/`
- `mkdocs.yml`
- website-facing generated assets/metadata and generator scripts when needed
- `README.md` and `CITATION.cff` only where they affect public user-facing documentation posture

Do not edit `spec/`, `agents/` other than this handoff, `docs/tasks/`, excluded legacy docs, or
maintainer-only material unless the user explicitly broadens scope.


## Goal

Improve the Datoviz v0.4 public website end to end so a new user can:

1. understand what Datoviz is and which layer to use;
2. install or build it according to the actual release state;
3. run a first Python or C example without stale API names;
4. browse examples without broken pages, hidden prerequisites, or misleading WebGPU status;
5. use how-to pages as reliable task recipes;
6. use reference pages without internal WIP/legacy terminology leaking into public API docs.

Make logical checkpoint commits after relevant checks pass. Do not commit unrelated user changes,
`data` gitlink updates, generated binaries, runtime libraries, or large payloads.


## Key Findings

### Release And Install Posture

`docs/start/install.md` currently says Datoviz v0.4 is not available as a release package and that
source build is the only path. Public RC notes in `docs/releases/v0.4.0rc1.md` are in navigation,
contain draft placeholders, and also report passed wheel CI evidence.

Decide and apply one public state across the site:

- pre-RC/source-only, or
- RC artifact-ready, or
- final release-ready.

Then update install, quickstart, release notes, README, C integration tag examples, and nav
together. Do not leave a public page saying both "source only" and "wheels passed/published".

Known public examples of drift:

- `docs/start/install.md`
- `docs/releases/v0.4.0rc1.md`
- `mkdocs.yml` release nav
- `README.md`
- `docs/how-to/c-integration.md`
- `docs/reference/build-options.md`


### First-User Journey

The public Get Started nav exposes only Install, Quickstart, and AI-assisted workflow. Important
orientation pages are hidden in `not_in_nav`:

- `docs/start/what-is-datoviz.md`
- `docs/start/choose-your-layer.md`
- `docs/start/first-c-program.md`

Because v0.4 has C, top-level Python facade, raw ctypes, DRP2, WebGPU/WASM, and GSP/VisPy2 boundary
language, users need layer orientation before or inside Quickstart.

Recommended action:

1. Make `What is Datoviz?` and `Choose your layer` visible in Get Started, or merge their essential
   content into `docs/start/quickstart.md`.
2. Replace `first-c-program.md` placeholder content with a real walkthrough or remove it from public
   discovery until complete.
3. Keep the first path practical: Install -> What is Datoviz / Choose your layer -> Quickstart ->
   Examples.


### Stale Or Inconsistent Code Examples

The README first Python example uses `"diameter"`, while current public docs and reference use
`"diameter_px"`.

Known examples:

- `README.md` uses `"diameter"`.
- `docs/index.md`, `docs/start/quickstart.md`, and visual reference use `"diameter_px"`.

Some minimal Python snippets use undefined variables such as `positions`, `colors`, and
`diameters`, which is weak for first-time copy/paste use:

- `docs/how-to/use-python.md`
- `docs/how-to/use-raw-ctypes.md`
- `docs/reference/python-direct-engine.md`

Recommended action:

1. Pick one canonical scatter/point example source.
2. Make README, homepage, quickstart, and start gallery agree on attribute names and call sequence.
3. Add a snippet check where practical, or reduce copied snippets and link to canonical examples.
4. Make "minimal" snippets either fully runnable or explicitly partial.


### Gallery Generation And Broken Public Pages

`docs/examples/examples.json` lists public examples whose generated Markdown pages are missing.
Observed missing pages include:

- `technique_edl`
- `feature_multi_window`
- `feature_view_size_policies`
- `feature_datetime_axis`

Their manifest entries exist in `examples/c/MANIFEST.yaml`, but corresponding
`docs/examples/gallery/.../*.md` pages are absent. Overview counts in generated index pages are
also stale.

Known affected areas:

- `docs/examples/examples.json`
- `docs/examples/index.md`
- `docs/examples/features.md`
- `docs/examples/runtime.md`
- `docs/examples/webgpu-matrix.md`
- `docs/examples/gallery/`
- `examples/c/MANIFEST.yaml`
- `tools/build_gallery.py`

Recommended action:

1. Regenerate public gallery pages and WebGPU matrix from the manifest.
2. Add or strengthen a public-doc check that every `examples.json[].page` exists.
3. Check that every public manifest example appears in the generated matrix or is intentionally
   excluded with visible reason.
4. Fix example counts by generating them from metadata rather than hand-maintaining prose.


### Gallery User Experience

Generated detail pages currently read more like metadata dumps than polished user pages:

- duplicate breadcrumbs such as `Examples / Examples`;
- terse lowercase descriptions;
- important WebGPU/native status hidden inside collapsed "Example details";
- data preparation commands hidden inside collapsed details;
- maintainer-oriented fields such as `Agent copy-safe`, `Build`, `Smoke`, and `Validation`;
- generic sentence "Generated media is prepared in the `data` submodule" even for synthetic or
  cache-backed examples.

Known examples:

- `docs/examples/gallery/start/start_scatter.md`
- `docs/examples/gallery/visuals/point_2d.md`
- `docs/examples/gallery/showcases/protein_arcball_viewer.md`
- `docs/examples/gallery/showcases/point_cloud.md`
- `docs/examples/gallery/showcases/showcase_lipid_brain_atlas.md`
- `docs/examples/gallery/showcases/showcase_synthetic_mouse.md`
- `docs/examples/gallery/showcases/us_state_choropleth.md`

Recommended action:

1. Add visible status badges near the preview:
   - supported / experimental / diagnostic / prototype
   - Live WebGPU / WebGPU planned / WebGPU deferred / Native only
2. Add a visible "Data prerequisites" block before Source for real, prepared, generated, or cached
   examples.
3. Make runtime-data wording conditional:
   - synthetic data: say no external data required;
   - cached data: name the cache path and preparation command;
   - data-submodule data: name expected path and preparation command;
   - generated media: distinguish screenshots/gallery media from runtime data.
4. Keep maintainer-only fields collapsed or move them to contributor docs; avoid exposing
   `Agent copy-safe` to users.
5. Add the start example to the examples index with a real Start section/card and fix empty sibling
   nav.
6. For pages with `_Media pending._`, either generate media before RC or visibly downgrade/label the
   example with the reason.


### WebGPU Status And Counts

`docs/examples/webgpu-matrix.md` is clear, but individual example pages hide status in collapsed
details. `docs/reference/webgpu-subset.md` also had a live-route count drift from the generated
matrix.

Recommended action:

1. Generate the live-route count from the same manifest used by the matrix.
2. Show WebGPU status visibly on every gallery detail page.
3. Link non-live pages to the matrix or WebGPU subset page so users understand planned/deferred vs
   broken.
4. Improve unknown-route handling in `examples/webgpu/live.js` by linking back to the matrix or
   example index.
5. Keep the first example story coherent: if `start_scatter` is the canonical first example but
   `webgpu-deferred`, explain that it is native-first or promote a browser-live first example.


### Reference Discoverability

`docs/reference/index.md` presents several pages as core reference material, but `mkdocs.yml`
places some of them under `not_in_nav`, including:

- `reference/project-status.md`
- `reference/visual-attributes.md`
- `reference/queries.md`
- `reference/errors-and-logging.md`

Several explanation pages are similarly hidden while explanation/index has a reading order.

Recommended action:

1. Either bring these pages into nav where users expect them, or stop presenting them as primary
   reference from `reference/index.md`.
2. Keep nav compact, but avoid a "secret reference" structure where linked public pages are hard to
   rediscover.
3. Ensure status pages, feature status, platform support, and WebGPU subset remain mutually
   consistent.


### Public Reference Language

Generated C reference leaks internal transitional terms into user docs:

- "WIP axis slice" in `docs/reference/c-api/scene.md`
- "legacy path" in `docs/reference/c-api/visuals.md`

Recommended action:

1. Update source comments or generator post-processing so generated reference uses public language.
2. Prefer support-status phrasing:
   - "current first supported slice" instead of "WIP";
   - "corner-vertex compatibility form" or "four-corner form" instead of "legacy path" when this is
     still a valid v0.4 input shape.
3. Rebuild generated C reference after source comment changes.


### C Integration Tags

Some public user docs use `GIT_TAG v0.4.0` before that final release exists.

Known pages:

- `docs/how-to/c-integration.md`
- `docs/reference/build-options.md`

Recommended action:

- Before final release, use `v0.4-dev`, `v0.4.0rc1`, or a placeholder such as `<release-tag>` with
  explicit wording.
- At final release, replace placeholders with the real tag.


### Citation And Version Metadata

`CITATION.cff` declares `version: 0.4.0`, while citation docs still include DOI placeholders and
future-release wording.

Recommended action:

1. Align `CITATION.cff`, `docs/reference/citation.md`, release notes, and actual tag state.
2. Do not imply a final Zenodo DOI or final v0.4.0 release before it exists.
3. For RC docs, clearly label citation examples as provisional.


## Suggested Subagent Work Split

Use one coordinating agent and four subagents. Subagents should work in disjoint write sets where
possible.

### Coordinator

Responsibilities:

1. Confirm current publication target with maintainer if unclear: pre-RC, RC1, or final.
2. Keep scope restricted to public website/user docs.
3. Sequence regeneration so generated docs are not hand-edited after generator changes.
4. Review returned patches for overlap, stale generated output, and user-facing consistency.
5. Run final validation and commit coherent checkpoints.

Guardrails:

- Do not stage or commit unrelated user changes.
- Do not commit `data` submodule pointer changes without explicit current-turn approval.
- Do not commit generated/runtime binary payloads.
- Run `git diff --check` before finalizing every checkpoint.


### Subagent A: First-User Journey And Release Posture

Write ownership:

- `docs/index.md`
- `docs/start/`
- `docs/releases/`
- `mkdocs.yml`
- `README.md`
- `CITATION.cff` and `docs/reference/citation.md` only if release/citation wording is in scope

Tasks:

1. Reconcile install/release/package language.
2. Make `What is Datoviz?` and `Choose your layer` visible or merge their content into first-run
   pages.
3. Replace or hide unfinished placeholder start pages.
4. Fix README/homepage/quickstart scatter drift.
5. Ensure release notes are either finalized or removed from public nav until finalized.

Validation:

- Link check through MkDocs strict build.
- Manual read-through of Home -> Get Started -> Quickstart.


### Subagent B: Gallery Generator And Metadata

Write ownership:

- `tools/build_gallery.py`
- `docs/examples/`
- `docs/examples/gallery/`
- `docs/examples/examples.json`
- `docs/examples/capabilities.json`
- `examples/c/MANIFEST.yaml` only for public metadata corrections
- `examples/webgpu/live.js` only for user-facing route diagnostics

Tasks:

1. Regenerate all missing public gallery pages.
2. Add visible status and WebGPU badges.
3. Add visible data prerequisites.
4. Remove misleading generic data-submodule wording.
5. Fix Start lane breadcrumbs/nav/index coverage.
6. Eliminate `_Media pending._` for supported public examples or visibly label the gap.
7. Add a validation check that metadata page links resolve.

Validation:

```sh
python3 tools/check_example_manifests.py
python3 tools/build_gallery.py
python3 tools/check_example_manifests.py
```

Also run the final MkDocs strict build from the coordinator.


### Subagent C: How-To Pages And Snippet Quality

Write ownership:

- `docs/how-to/`
- `docs/start/quickstart.md`
- `docs/reference/python-direct-engine.md`
- `docs/reference/ctypes.md`

Tasks:

1. Make minimal snippets runnable or clearly partial.
2. Remove undefined-variable "minimal" examples where users are expected to copy/paste.
3. Ensure attribute names and call order match generated examples and visual reference.
4. Ensure each how-to links to one canonical gallery example.
5. Keep C vs Python vs raw ctypes distinctions clear.

Validation:

- Grep for stale attribute names like `"diameter"` when `"diameter_px"` is intended.
- Run docs strict build.


### Subagent D: Reference And Status Polish

Write ownership:

- `docs/reference/`
- generated C reference source comments or generator files if needed
- `mkdocs.yml` only for reference nav changes coordinated with Subagent A

Tasks:

1. Reconcile `project-status`, `feature-status`, `platform-support`, `webgpu-subset`,
   `v03-visible-parity`, and generated C reference language.
2. Remove public "WIP" and ambiguous "legacy" wording.
3. Decide which reference pages belong in nav and update nav/index consistently.
4. Make WebGPU live-route counts generated or obviously non-hand-maintained.
5. Keep Python direct-engine and raw ctypes roles aligned with install/quickstart.

Validation:

- Grep public docs for `WIP`, `placeholder`, `<fill`, stale counts, and accidental internal terms.
- Run docs strict build.


## Validation Commands

Use the repo's docs-capable Python environment. Plain `python -m mkdocs` may fail if MkDocs is not
installed in the active interpreter.

Suggested setup:

```sh
python -m pip install -r requirements-dev.txt
```

Then validate:

```sh
python3 tools/check_example_manifests.py
python -m mkdocs build --strict --site-dir /tmp/datoviz-docs-audit
git diff --check
git status --short
```

If public headers, binding policy, binding generator, or exported API comments are changed to fix
generated reference wording, also follow the repository binding rule:

```sh
just ctypes
just ctypes-check
```

If examples or gallery metadata are changed, use the narrowest relevant example/doc checks before
regenerating and committing generated docs.


## Suggested Commit Boundaries

1. **Docs posture and first-user nav**
   - install/release/README/nav/citation posture
   - first-user layer orientation
   - validation: MkDocs strict build, `git diff --check`

2. **Gallery regeneration and public metadata**
   - missing pages, status badges, data prerequisites, start lane, WebGPU matrix
   - validation: manifest check, gallery generator, MkDocs strict build, `git diff --check`

3. **How-to and snippets**
   - copyable snippets, canonical examples, Python/raw ctypes clarity
   - validation: MkDocs strict build, snippet greps, `git diff --check`

4. **Reference/status polish**
   - nav/index consistency, WIP/legacy language, WebGPU counts/status
   - validation: MkDocs strict build, `git diff --check`; add `just ctypes` /
     `just ctypes-check` if public header/binding comments or generated C API inputs changed


## Final Acceptance Criteria

Before committing the final checkpoint:

1. `mkdocs build --strict` succeeds in a docs-capable environment.
2. No public page contains unresolved release placeholders such as `<fill after ...>` unless the page
   is clearly not published in nav.
3. Install, README, quickstart, and release notes describe one coherent package state.
4. Every example listed in public JSON/manifest metadata has a generated public page or an explicit
   non-public classification.
5. Gallery pages show visible runtime/data/WebGPU status without requiring users to open collapsed
   details.
6. No supported public example card says `_Media pending._` unless the status visibly explains why.
7. WebGPU live-route count is generated or reconciled across matrix, reference page, and live route
   registry.
8. Public reference pages do not expose internal "WIP" wording.
9. `git diff --check` passes.
10. `git status --short` and `git diff --cached --stat` are inspected before commit; staged changes
    exclude unrelated user changes, unapproved `data` gitlink updates, generated binaries, runtime
    libraries, and large payloads.
