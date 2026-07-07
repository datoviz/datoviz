# Datoviz v0.4 Public Documentation Rewrite Handoff

Status: active pre-RC documentation campaign.

This is the active handoff for rebuilding the public Datoviz v0.4 documentation and website before
RC publication. Use it as a mission brief, not a script. The coordinator may use subagents, but
must keep ownership boundaries clear and run final integration, validation, staging, and commits.


## Mission

Rework the docs into a coherent public website for Datoviz v0.4:

1. progressive paths for Python scientists, C/C++ app developers, WebGPU/browser users, advanced
   rendering developers, and contributors;
2. `Examples` as the centerpiece, with one visual or feature represented by one focused example
   where possible;
3. polished showcase pages with real visual artifacts;
4. clear install, release, API, and feature-status posture for the pre-RC state;
5. no stale v0.3 compatibility promises, obsolete Python wrapper examples, or internal agent-note
   tone in public pages.

The first-pass audit and tone-sample review have been completed. Continue in focused checkpoint
passes, keep commits scoped, and ask for maintainer review after each batch that changes public
voice or page structure.


## Approved Direction

These choices are already approved:

1. Make `Examples` the public centerpiece.
2. Prefer `Advanced` over `Internals` as the public section name.
3. Do not keep `Contributing` as a top-level public tab; keep contributor docs reachable under
   `Advanced` or another secondary path.
4. Lead the landing page and quickstart with Python direct-engine code, then C.
5. Keep generated examples and reference material C-first where C is the executable source of truth,
   with Python direct-engine or `datoviz.raw` exact-call guidance where policy supports it.
6. Do not let promoted public examples ship with generic `_Media pending._`; generate media or show
   a concrete reason/status.
7. Keep WebGPU live examples isolated in iframe routes such as:

   ```text
   examples/webgpu/live.html?id=<example-id>
   ```

8. Change generated gallery pages through manifest metadata, templates, or generators; do not
   hand-edit generated Markdown as the primary fix.
9. Do not stage or commit `data` submodule changes, generated binaries, or large media without
   explicit maintainer approval in the current turn.


## Public Voice Rules

These rules are mandatory for public pages under `docs/` and for generated public example pages.
They supersede earlier agent-facing wording when there is a conflict.

1. Write for beginner users first: primarily scientists who may know Python and data analysis, but
   may not have much C, graphics, GPU, or visualization-engine experience. Treat them as competent
   adults learning a new technical tool, not as children.
2. Avoid internal/developer language on public pages unless the page is explicitly in `Advanced`.
   Do not lead with ownership, lifetimes, transport layers, renderer internals, DRP2, ABI details,
   or release-process notes on beginner pages.
3. Explain the user outcome before the API. Start pages with what the user can do, what they will
   see, and which path to take.
4. Prefer plain, professional language over architecture shorthand. Define terms in accessible
   words, but do not oversimplify into vague phrases such as "the thing being drawn." For example,
   explain that a panel is the drawing area inside a figure, and that a visual is a renderable
   collection such as points, lines, an image, a mesh, or text labels.
5. Avoid compressed implementation labels in beginner-facing prose. Do not pack import location,
   engine layer, binding implementation, and naming convention into one noun phrase. Say what the
   user should do and why, for example: "In Python, use `import datoviz as dvz`; these calls use the
   same `dvz_*` function names as the C examples and accept NumPy arrays for supported visual-data
   uploads."
6. Do not make broad claims that are only approximately true. Say "most public functions use
   `dvz_...` names" rather than "every function follows one convention" unless verified.
7. Keep Quickstart and first-user pages short and structured. Move side topics such as offscreen
   capture, `datoviz.raw` exact calls, CMake integration, and lifecycle edge cases to focused How-To or
   Reference pages.
8. The AI-assisted workflow page must be simple. Its main prompt should tell the coding assistant
   to browse `datoviz.org` and write the requested Datoviz v0.4 example. The canonical public
   entry point for agents is `/ai-agents/`. The prompt must tell agents to inspect examples, check
   the API reference, and verify every function name, signature, enum, attribute name, and array
   shape they use. Do not require users to manually assemble a context bundle of several pages
   unless they want better precision.
9. Write installation instructions as decision paths by audience and operating system: Python user,
   C/C++ user, source build, macOS, Linux, Windows/WSL, native Windows. Be explicit about what is
   currently public, what is pre-release, and what is not yet published.
10. Generated examples need more explanation, not less. Each public example should explain:
   what the example demonstrates, what visual result to expect, which data attributes matter, which
   user interaction applies, and where to go next. Code comments in examples should describe intent
   and important choices, not merely restate the function name.
11. Generated `What To Look For` sections must be example-specific and should come from the public
    source example's top block comment. Do not use generic category prose such as "this focused
    feature example demonstrates..." as the first paragraph. Each top block comment should explain
    what the example demonstrates, what to inspect visually, which data arrays or attributes matter,
    and which interaction applies. Generator fallbacks are only safety nets for missing comments,
    not the normal public copy.
12. When describing the usual scene workflow, include the whole user-visible sequence: create the
    scene/figure/panel, create a visual, attach data arrays to visual attributes, add the visual to a
    panel, create a window or offscreen target, then run the app or capture the frame.
13. Explain visual granularity with realistic examples. Do not rely on extreme comparisons such as
    one million one-item visuals. Say that semantically linked elements of the same visual family
    should usually be grouped into one visual, for example 100 related points in one point visual
    with 100 positions, colors, and sizes.
14. For How-To pages, start from a concrete user situation, then give the workflow, then explain the
    available choices with practical examples. Keep tables, code snippets, and "common mistakes"
    sections, but make sure prose around them explains why the user would choose each path.
15. Prefer short, readable public page slugs. When rewriting or moving task pages, simplify verbose
    filenames and URLs where it improves the site, for example `how-to/multiple-panels/` rather than
    `how-to/create-multiple-panels/`. Update `mkdocs.yml`, cross-links, generated references, and
    redirects or compatibility aliases as needed so existing links do not silently break. Check that
    the slug, page title, and navigation label describe the same task; they need not be identical,
    but they should not drift into different wording or scope.
16. If an example is minimal, say why it is minimal. If it is advanced, say what prior concepts the
    reader should know before using it.
17. Treat gallery screenshots and live examples as related but separate outputs. Generated gallery
    screenshots should follow the current manifest/example screenshot size, currently 1280x720 unless
    an example explicitly overrides it. Browser live examples may remain responsive or use a
    different viewport when that is better for interaction.
18. Public gallery visibility is controlled by the manifest review `batches`. A required public
    example should either appear in one of those batches or have an explicit deferment/status reason;
    do not rely on a manifest row alone. Screenshot capture and media checks should default to the
    reviewed public batch set, with explicit opt-in for unreviewed examples.
19. Do not ask users to run an extra WebGPU/WASM build step before serving the docs. `just serve`
    should run a fast preflight that rebuilds `build-wasm-scene/wasm/datoviz_wasm_scene.mjs` only
    when it is missing or stale.


## Current State

Current public navigation in `mkdocs.yml`:

```text
Home / Get Started / Examples / How-To / Reference / Advanced
```

Approved final navigation:

```text
Home / Get Started / Examples / How-To / Reference / Advanced
```

Active source chain:

1. `mkdocs.yml` is the concrete navigation source of truth.
2. Public Markdown lives under `docs/`.
3. Generated gallery Markdown lives under `docs/examples/` and `docs/examples/gallery/`.
4. Gallery metadata lives in `examples/c/MANIFEST.yaml`.
5. Gallery/WebGPU generation and checks include:

   ```text
   tools/build_gallery.py
   tools/build_examples_manifest.py
   tools/build_capabilities.py
   tools/build_gallery_webp.py
   tools/check_example_manifests.py
   tools/mkdocs_hooks.py
   tools/ensure_wasm_scene_build.py
   examples/webgpu/live_examples.js
   examples/webgpu/live.js
   ```

6. Gallery WebP assets are generated into `build/gallery-webp/v0.4` and injected into the built site
   as `assets/gallery/v0.4`.
7. `just serve` depends on `docs-assets` and `wasm-scene-build-if-needed`; the latter should be a
   no-op when the WASM scene module and its build stamp are current.
8. Source screenshots and some data assets live in the `data` submodule. Treat `data` as a stop-sign
   path for staging/committing unless explicitly approved.
9. `docs/assets~` contains reference imagery only; it is not the generated public gallery asset
   tree.

Unrelated or stop-sign working-tree state may exist, including `data` and untracked paper artifacts.
Inspect before staging and do not commit unrelated changes.


## Proposed Final IA

Use this as the target unless the first-pass audit finds a better structure:

```text
Get Started
  Install
  Quickstart
  Choose your layer
  AI-assisted workflow

Examples
  Overview / gallery
  Showcases
  Visuals and composites
  Features
  Runtime and capture
  Advanced examples
  WebGPU matrix

How-To
  Core workflow
  Data to visuals
  Layout and annotation
  Interaction
  Rendering
  Output
  Integration
  Diagnostics

Reference
  API
  Visual families
  Scene contracts
  Feature, platform, and release status
  Backends

Advanced
  Architecture
  WebGPU/WASM
  vklite
  Canvas and stream API
  DRP2 internals
  Contributing
  Release-maintainer docs
```

Rules:

1. `Get Started -> Examples -> How-To -> Reference` is the main user path.
2. `Examples` are executable truth and visual proof, not an appendix.
3. `How-To` answers task questions and links to canonical examples.
4. `Reference` gives exact status, signatures, attributes, lifetimes, and backend support.
5. `Advanced` is public but secondary; it serves backend, embedding, WebGPU, Vulkan, contributor,
   and release-maintainer readers.


## Source Of Truth

Read these before planning or assigning subagents:

1. `AGENTS.md`;
2. `agents/now/START.md`;
3. `agents/now/STATUS.md`;
4. `agents/now/RELEASE.md`;
5. `agents/now/DOCUMENTATION.md`;
6. `spec/docs/V04_DOCUMENTATION_DECISIONS.md`;
7. `spec/docs/INFORMATION_ARCHITECTURE.md`;
8. `spec/api/PUBLIC_API_CONVENTIONS.md`;
9. `spec/api/PYTHON_GSP_SCOPE.md`;
10. `spec/bindings/ARRAY_FACADE.md`;
11. `spec/bindings/CTYPES_POLICY.md`;
12. `spec/scene/README.md`;
13. `spec/drp2/README.md` if touching advanced transport/runtime docs.

If planning files conflict, prefer `spec/docs/V04_DOCUMENTATION_DECISIONS.md`, then reconcile older
planning text as part of the campaign.

For API claims, verify against execution-time truth: public headers, API/status manifests, binding
policy, generated `ctypes`, canonical C examples, generated gallery metadata, generated C
reference, and contract-sensitive tests.


## First-Pass Gate

Before broad autonomous rewriting, deliver a short plan for maintainer feedback:

1. inventory active nav pages, unlisted docs, excluded legacy docs, generated pages, generated
   assets, and source metadata;
2. identify stale, duplicate, missing, misplaced, public/private mixed, or legacy content;
3. confirm or revise the proposed IA and top-level MkDocs navigation;
4. propose gallery/media/WebGPU integration, including missing-media policy;
5. propose the examples model: one visual = one minimal example, one feature = one minimal example,
   one showcase = one polished composed story;
6. rewrite a small tone sample:
   - homepage or landing page;
   - install page;
   - quickstart page;
   - one how-to page;
   - one visual-family or API reference page;
   - one generated-gallery/template change if gallery tone is part of the proposal.

Landing and quickstart samples should show Python first, then C. Generated example/reference pages
may remain C-first where C is the source of truth.

The accepted How-To tone sample is `docs/how-to/update-visual-data.md`: practical, beginner-facing,
and explicit about arrays, attributes, batching, and update choices without leading with internals.
Use it as the style target for the next How-To and feature-example rewrites. The tone should be
clear enough for a first-time scientific user, but still professional: avoid both engine jargon and
overly childish definitions.


## First-Pass Audit Snapshot

Status: reviewed and approved as the basis for the full documentation campaign. Audit date:
2026-07-05.

The read-only audit used subagents for disjoint slices: navigation/inventory,
gallery/WebGPU metadata, first-user journey/snippets, and reference/status pages. The coordinator
checked `mkdocs.yml`, generated gallery counts, WebGPU route alignment, public wording searches, and
repository hygiene.

### Confirmed Inventory

1. `mkdocs.yml` exposes six top-level tabs: `Home`, `Get Started`, `Examples`, `How-To`,
   `Reference`, and `Advanced`.
2. Active nav contains 109 pages. Public-looking hidden candidates include
   `docs/start/choose-your-layer.md`, `docs/start/what-is-datoviz.md`,
   `docs/reference/project-status.md`, `docs/reference/visual-attributes.md`,
   `docs/reference/queries.md`, and `docs/reference/errors-and-logging.md`.
3. Excluded legacy trees remain under `docs/architecture/`, `docs/blog/`, `docs/discussions/`,
   old `docs/gallery/`, `docs/guide/`, and `docs/tasks/`.
4. `docs/reference/api_c.md` exists in the working tree but is neither in nav nor `not_in_nav`.
   It is stale generated material and should be excluded, removed, or regenerated before it is ever
   surfaced.
5. The generated public C gallery currently contains 107 examples and 107 generated detail pages.
   Experimental examples outside reviewed public batches should not appear on the website.

### Main Findings

1. **IA conflict:** the approved public IA is
   `Home / Get Started / Examples / How-To / Reference / Advanced`, but current nav still has
   top-level `Internals` and `Contributing`.
2. **Reference discoverability:** `docs/reference/index.md` promotes pages that are hidden from
   nav, including project status, visual attributes, queries, errors/logging, and DRP2 reference
   material.
3. **Release/install posture:** `docs/start/install.md`, `README.md`,
   `docs/how-to/c-integration.md`, `docs/reference/build-options.md`, and
   `docs/releases/v0.4.0rc1.md` mix source-only, post-wheel, and draft-RC language.
4. **Public placeholders:** `docs/releases/v0.4.0rc1.md` still has `<fill ...>` fields and a
   maintainer checklist while being reachable from public nav.
5. **Snippet correctness:** `README.md` still uses the stale point attribute `"diameter"` instead
   of `"diameter_px"`. Several Python/ctypes snippets are partial but look copyable because they use
   undefined arrays or handles.
6. **Generated API language:** generated C API pages leak `WIP` and `legacy path` wording from
   public header comments. The generated C API table formatting also mishandles multiline return
   descriptions.
7. **Gallery generation drift:** generated overview pages and the WebGPU matrix can become stale if
   `tools/build_gallery.py`, `tools/build_examples_manifest.py`, and `tools/build_capabilities.py`
   are not rerun after manifest or source-comment changes.
8. **Gallery metadata gap:** `docs/examples/examples.json` may lack WebGPU status, route, requirement,
   media, and data fields even though the manifest has that information.
9. **WebGPU registry:** current `webgpu-live` rows are internally aligned: 66 manifest live entries,
   66 `examples/webgpu/live_examples.js` entries, and no route-id mismatch found. However live route
   JS does not carry manifest requirement metadata for route-level diagnostics.
10. **Media posture:** public gallery pages should not expose `_Media pending._` for promoted
    examples. `python3 tools/check_gallery_media.py --ignore-cache` should pass for reviewed public
    screenshots; cache staleness can be expected after comment-only source edits.
11. **Data submodule:** source screenshots and stale/orphan media are in `data/gallery/v0.4`.
    Do not stage `data` changes or generated WebP/binary outputs without explicit approval.
12. **Public/private tone:** generated example pages and visual-family pages expose
    `Agent copy-safe`; `docs/start/choose-your-layer.md` speaks directly to coding agents. Keep
    agent-oriented material in `Advanced` contributor paths or the deterministic AI workflow page,
    not in primary public flow.

### Approved Direction For Full Rewrite

Use this direction for broad rewriting:

1. **IA:** switch public nav to
   `Home / Get Started / Examples / How-To / Reference / Advanced`. Move contributor and release
   maintainer docs under `Advanced` or keep them secondary; remove top-level `Contributing`.
   Rename `Internals` to `Advanced`.
2. **Get Started tone sample:** make landing, install, and quickstart concise, public, and current:
   Python direct-engine first, C second, no v0.3 compatibility promise, no hidden partial snippets
   presented as runnable, and one clear install posture: source builds now; RC packages only when
   actually published.
3. **Examples model:** keep Examples as the centerpiece. One visual gets one minimal example, one
   feature gets one minimal example, one showcase gets one polished composed story. Do not let
   showcase pages replace minimal visual/feature coverage.
4. **Gallery model:** change generated gallery pages through manifest metadata, templates, and
   generators. Surface native/WebGPU status near the preview, move maintainer metadata such as
   `Agent copy-safe` out of the primary public flow, make data prerequisites conditional on data
   kind, and generate or explain media gaps before promotion.
5. **WebGPU model:** keep live routes isolated at
   `examples/webgpu/live.html?id=<example-id>`. Preserve the rule that browser JS is host glue for
   canonical C examples or portable scenarios. Add requirement metadata to public JSON or live route
   data if needed for diagnostics.
6. **Reference model:** keep exact facts in Reference. Promote status, visual attributes, queries,
   and errors/logging into nav or demote their index links. Keep DRP2 as advanced/internal context,
   not a primary user surface. Fix generated API wording at header/generator source before
   regenerating.
7. **Release/install posture:** publish public release notes only as an honest RC draft with
   remaining blanks explicitly marked as pre-publication metadata, or remove the draft from nav
   until tag/publication. Reconcile README, install, C integration, build options, citation, and
   RC notes as one commit.

### Proposed Checkpoint Phases

1. **Navigation checkpoint:** `mkdocs.yml`, reference index, and secondary contributor/release nav.
   Validation: `git diff --check`, `mkdocs build --strict` when dependencies are available.
2. **First-user checkpoint:** README, landing, install, quickstart, choose-your-layer, AI workflow,
   C integration, build options, citation, and RC note posture. Validation: snippet/source checks
   plus `git diff --check`.
3. **Gallery generator checkpoint:** manifest/generator/json/template fixes, regenerate gallery
   Markdown, confirm `feature_camera_manual`, move maintainer metadata, enrich JSON, and rerun
   `python3 tools/check_example_manifests.py`.
4. **Media checkpoint:** decide which missing media can be generated locally and which should be
   explicitly labeled as pending with a concrete reason. Run `python3 tools/check_gallery_media.py`
   and WebP dry run. Commit source changes only; do not commit `data` or binary WebP output unless
   explicitly approved.
5. **How-to/snippet checkpoint:** make partial snippets visibly partial or complete; fix Python,
   `datoviz.raw` exact calls, direct-engine, and visual-family examples against the active API.
6. **Reference/API checkpoint:** fix source comments and C API generator formatting, regenerate API
   docs, reconcile feature/status/project/parity pages, and run `just docs-api-check` if generation
   is touched.
7. **Final strict-site checkpoint:** run narrow validation, then full strict build if dependencies
   and generated assets are available. Record skipped checks and environment blockers.

### Audit Validation Evidence

Commands run during audit:

```sh
git status --short
git diff --cached --stat
python3 tools/check_example_manifests.py
node --check examples/webgpu/live_examples.js
node --check examples/webgpu/live.js
git diff --check
```

Expected audit failures reported by the gallery slice:

```sh
python3 tools/check_gallery_media.py
python3 tools/build_gallery_webp.py --dry-run --strict --quiet-missing
```

`check_gallery_media.py` reports missing or invalid gallery screenshots. The WebP dry run reports
many conversions available and missing source screenshots. These are release-media work items, not
unexpected tool failures.


## Execution Plan

After maintainer approval, work in checkpoint-sized phases:

1. **Navigation and structure:** update `mkdocs.yml`, rename `Internals` to `Advanced`, move
   top-level `Contributing` under a secondary path, and remove/quarantine stale docs.
2. **Get Started:** reconcile homepage, install, quickstart, choose-your-layer, AI workflow, README,
   release notes, citation, and first examples.
3. **Examples and gallery:** fix manifest/template/generator drift, status badges, screenshots,
   WebGPU links, data prerequisites, metadata leakage, and media gaps.
4. **How-To:** make pages task-oriented, runnable where copy/paste is promised, and linked to
   canonical examples instead of duplicated source.
5. **Reference:** reconcile feature/status, v0.3 visible parity, generated C reference, raw
   `ctypes`, direct-engine docs, visual-family pages, object lifetimes, callbacks, queries,
   platform/build, WebGPU, and compute+graphics.
6. **Advanced and contributors:** keep lower-layer, architecture, DRP2, WebGPU, vklite, canvas,
   contributor, and release-maintainer docs useful but secondary.
7. **Website polish and final proof:** build the site, inspect layout/media/WebGPU embeds, rerun
   stale-language searches, run validation, and record skipped checks.


## Audit Checklist

Verify or close these current leads:

1. **Release/install posture:** reconcile `README.md`, `docs/start/install.md`,
   `docs/releases/v0.4.0rc1.md`, `docs/how-to/c-integration.md`,
   `docs/reference/build-options.md`, `docs/reference/citation.md`, and `CITATION.cff`. Do not mix
   source-only, RC-artifact-ready, and final-release language.
2. **First-user journey:** decide whether to merge, re-nav, or keep unlisted
   `docs/start/what-is-datoviz.md`, `docs/start/choose-your-layer.md`, and
   `docs/start/first-c-program.md`.
3. **Snippet correctness:** check README, homepage, quickstart, Python how-to, `datoviz.raw`,
   direct-engine docs, and reference pages for stale `"diameter"` vs `"diameter_px"`, undefined
   variables, old wrappers, and partial snippets presented as runnable.
4. **Generated reference language:** search public docs and generated references for `WIP`,
   `placeholder`, `<fill`, `legacy path`, `old API`, `TODO`, and stale attribute names. Fix source
   comments or generators before regenerating.
5. **Gallery consistency:** verify manifest rows, `docs/examples/examples.json`, generated detail
   pages, WebGPU matrix, and live route registry agree. Previously missing pages such as
   `technique_edl`, `feature_multi_window`, `feature_view_size_policies`, and
   `feature_datetime_axis` now appear to exist, but still verify generator coverage.
6. **Gallery UX:** surface native/WebGPU status near previews; keep user data prerequisites visible;
   make data wording conditional for synthetic, simulated, generated, prepared, real, and external
   data; move maintainer-only fields out of the primary flow.
7. **WebGPU status:** keep `docs/reference/webgpu-subset.md`,
   `docs/examples/webgpu-matrix.md`, generated gallery pages, `examples/webgpu/live_examples.js`,
   and manifest WebGPU metadata aligned on live/planned/deferred/native-only status, route count,
   scenario IDs, capability requirements, and unsupported-feature diagnostics.
8. **Reference discoverability:** pages promoted by `docs/reference/index.md` should be in nav or
   clearly secondary. Known candidates: project status, visual attributes, queries, and
   errors/logging.


## Subagent Model

Use subagents only with disjoint write ownership. Each subagent reports files inspected, files
changed, source-of-truth checks, unresolved decisions, validation run, generated outputs touched,
and paths intentionally left unchanged.

Recommended splits:

1. **First-user journey and release posture:** `README.md`, `docs/index.md`, `docs/start/`,
   `docs/releases/`, citation/version wording, and relevant `mkdocs.yml` entries.
2. **Gallery generator and metadata:** `examples/c/MANIFEST.yaml`, gallery generators, generated
   `docs/examples/`, `docs/examples/gallery/`, `examples/webgpu/live_examples.js`, and
   `examples/webgpu/live.js`.
3. **How-to and snippets:** `docs/how-to/`, snippet-heavy quickstart sections,
   `docs/reference/python-direct-engine.md`, and `docs/reference/ctypes.md`.
4. **Reference and status:** `docs/reference/`, generated C reference source comments or
   generators, visual-family templates, feature/status tables, and reference nav.
5. **Website polish:** `docs/stylesheets/`, homepage/gallery layout, generated templates as
   coordinated, media presentation, MkDocs build review, and mobile/layout checks.

The coordinator owns generation order, final review, conflict resolution, validation, staging, and
checkpoint commits.


## Validation And Safety

Documentation-only baseline:

```sh
git diff --check
git status --short
```

Generated API docs:

```sh
just docs-api
just docs-api-check
```

Gallery and example metadata:

```sh
just gallery
just check-example-manifests
just check-gallery-media
```

Docs assets and local site:

```sh
just docs-assets
uv run --with mkdocs-material --with 'mkdocstrings[python]' --with pillow mkdocs build --strict
```

WebGPU live-example integration when relevant:

```sh
node --check examples/webgpu/live_examples.js
node --check examples/webgpu/live.js
node --check tools/wasm_scene_smoke.mjs
node --check tools/webgpu_browser_smoke.mjs
just wasm-scene-smoke
just webgpu-browser-smoke
```

Public API or binding changes:

```sh
just ctypes
just ctypes-check
```

Before every commit:

```sh
git diff --check
git status --short
git diff --cached --stat
```

Do not stage or commit:

1. `data` submodule gitlink changes without explicit approval in the current turn;
2. generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`, `*.so`, `*.dll`, `*.npy`,
   `*.npz`, or `.DS_Store`;
3. large screenshot/video/media payloads without explicit approval;
4. unrelated user changes.


## Done Criteria

The campaign is done when:

1. the maintainer has approved IA and tone sample;
2. public docs match final pre-RC API and feature status;
3. the website has a coherent path from landing page to examples, how-to, and reference;
4. public visual families and features have minimal examples or visible deferments;
5. showcases are polished, screenshot/live-first, and distinct from minimal examples;
6. generated gallery pages, manifests, WebGPU routes, and media are consistent;
7. stale v0.3-era docs are removed, rewritten, or excluded from the public site;
8. release/install/citation/version wording matches the actual RC state;
9. validation commands and skipped checks are recorded;
10. checkpoint commits contain only approved, relevant changes.
