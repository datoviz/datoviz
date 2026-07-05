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

First action: audit the current docs/site and produce a short maintainer-reviewable plan. Do not
begin broad rewriting until the maintainer approves the IA, tone sample, gallery model, and
release/install posture.


## Approved Direction

These choices are already approved:

1. Make `Examples` the public centerpiece.
2. Prefer `Advanced` over `Internals` as the public section name.
3. Do not keep `Contributing` as a top-level public tab; keep contributor docs reachable under
   `Advanced` or another secondary path.
4. Lead the landing page and quickstart with Python direct-engine code, then C.
5. Keep generated examples and reference material C-first where C is the executable source of truth,
   with Python direct-engine or raw `ctypes` guidance where policy supports it.
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


## Current State

Current public navigation in `mkdocs.yml`:

```text
Home / Get Started / Examples / How-To / Reference / Internals / Contributing
```

Preferred final navigation:

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
   examples/webgpu/live_examples.js
   examples/webgpu/live.js
   ```

6. Gallery WebP assets are generated into `build/gallery-webp/v0.4` and injected into the built site
   as `assets/gallery/v0.4`.
7. Source screenshots and some data assets live in the `data` submodule. Treat `data` as a stop-sign
   path for staging/committing unless explicitly approved.
8. `docs/assets~` contains reference imagery only; it is not the generated public gallery asset
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
3. **Snippet correctness:** check README, homepage, quickstart, Python how-to, raw `ctypes`,
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
