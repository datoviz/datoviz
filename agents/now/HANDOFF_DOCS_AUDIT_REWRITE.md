# Datoviz v0.4 Public Documentation Rewrite Handoff

Status: active pre-RC documentation campaign.

This is the single active handoff for aggressively rebuilding the public Datoviz v0.4
documentation and website before RC publication. It is written for a high-reasoning coordinator
agent that may use subagents. The objective is not a light cleanup. The objective is a coherent,
progressive, visually strong public documentation site that matches the final pre-RC API and helps
different user categories succeed without reading internal agent notes.


## Coordinator Brief

First action: audit the current docs/site and produce a short maintainer-reviewable plan. Do not
start the broad rewrite until the maintainer approves the proposed IA, tone sample, gallery model,
and release/install posture.

Hard preferences already approved:

1. Make `Examples` the public centerpiece of the site.
2. Prefer `Advanced` over `Internals` as the public section name.
3. Do not keep `Contributing` as a top-level public tab; keep contributor docs reachable under
   `Advanced` or another secondary path.
4. Lead landing and quickstart examples with Python direct-engine code, then C.
5. Keep generated example pages and reference material C-first where that is the source of truth,
   with Python direct-engine or raw-binding guidance where policy supports it.
6. Do not let promoted public examples ship with a generic `_Media pending._`; generate media or
   show a concrete reason/status.
7. Use subagents only with disjoint file ownership. The coordinator owns generation order, final
   review, validation, staging, and commits.
8. Do not touch or stage `data` submodule state, generated binaries, or large media without explicit
   maintainer approval in the current turn.
9. Always run `git diff --check` before finalizing changes.


## Current State Snapshot

The MkDocs navigation has already moved to the simplified public structure:

```text
Home / Get Started / Examples / How-To / Reference / Internals / Contributing
```

The recommended final public navigation is tighter:

```text
Home / Get Started / Examples / How-To / Reference / Advanced
```

Contributor docs should remain reachable, but not as a primary user-facing tab. The intended main
user path remains:

```text
Get Started -> Examples -> How-To -> Reference
```

Current active surfaces:

1. `mkdocs.yml` is the concrete public navigation source of truth.
2. Public Markdown lives under `docs/`.
3. Generated gallery Markdown lives under `docs/examples/` and `docs/examples/gallery/`.
4. Gallery metadata lives in `examples/c/MANIFEST.yaml`.
5. Gallery generators and checks include:

   ```text
   tools/build_gallery.py
   tools/build_examples_manifest.py
   tools/build_capabilities.py
   tools/build_gallery_webp.py
   tools/check_example_manifests.py
   tools/mkdocs_hooks.py
   ```

6. Gallery WebP files are generated into `build/gallery-webp/v0.4` and injected into the built site
   as `assets/gallery/v0.4`.
7. Source screenshots and some data assets live in the `data` submodule. Do not stage or commit
   `data` gitlink changes or binary media without explicit maintainer approval in the current
   turn.
8. Live WebGPU examples are isolated routes:

   ```text
   examples/webgpu/live.html?id=<example-id>
   ```

9. `docs/assets~` contains reference imagery only. It is not the generated public gallery asset
   tree.

Observed current issues to verify and fix during the campaign:

1. `README.md` still uses `"diameter"` in a first Python example while current point/marker docs
   use `"diameter_px"`.
2. `docs/start/install.md` still states source-only install, while release and packaging notes
   describe RC wheel artifacts and package work. Reconcile release posture deliberately.
3. Generated C reference pages still expose terms such as `WIP`, `legacy path`, and some
   `"diameter"` attribute wording. Fix source comments or generators before regenerating.
4. `docs/reference/index.md` promotes pages such as project status, visual attributes, queries, and
   errors/logging that are currently excluded from navigation in `mkdocs.yml`. Either add them to
   nav or stop presenting them as primary reference pages.
5. Generated gallery detail pages still read too much like metadata dumps. Important user-facing
   status is hidden in collapsed details, while maintainer-only fields are visible.
6. Some generated gallery pages show `_Media pending._` even when the example is public and live in
   WebGPU.
7. Generic data-submodule wording appears on synthetic examples. Data/media wording must be
   conditional on the manifest data contract.
8. The current worktree may contain unrelated or stop-sign state such as `data` and untracked
   papers. Inspect before staging. Do not commit unrelated changes.


## Proposed Final Information Architecture

This is the preferred target IA unless the first-pass audit reveals a stronger alternative.

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

Structure rules:

1. `Examples` is a first-class top-level section, not a side gallery.
2. `How-To` answers task questions and links to canonical generated examples.
3. `Reference` gives exact facts, signatures, status, attributes, lifetimes, and backend support.
4. `Advanced` is public but secondary; it serves backend, embedding, WebGPU, Vulkan, and contributor
   readers.
5. Contributor and release-maintainer docs may remain in the site, but they must not dominate the
   primary user path.


## Start Condition

The completed pre-RC public API cleanup branch has landed on `v0.4-dev`; the broad documentation
rewrite is unblocked.

The documentation must describe the final pre-RC API, not transitional pre-cleanup APIs. Treat the
following as execution-time truth:

1. public headers in `include/datoviz/`;
2. exported API/status manifests under `spec/api/`;
3. binding policy under `spec/bindings/`;
4. generated `ctypes` policy and outputs;
5. canonical examples under `examples/c/`;
6. generated gallery metadata and pages;
7. generated C reference from `tools/build_api_c.py`;
8. tests where behavior is contract-sensitive.


## Non-Negotiable Constraints

1. Do not preserve v0.3 compatibility at the expense of v0.4 architecture, correctness, or
   maintainability.
2. Do not revive the old Python-first plotting guide as current Datoviz v0.4 guidance.
3. Do not document DRP2 as a primary public user layer. Mention it as an internal/advanced
   transport where relevant.
4. Do not hand-edit generated gallery Markdown as the primary fix. Change manifest metadata,
   templates, or generators, then regenerate.
5. Do not stage, commit, or push `data` submodule changes unless explicitly approved in the current
   turn.
6. Do not stage generated/runtime binary payloads, vendored runtime libraries, `.DS_Store`, large
   media, `*.npy`, or `*.npz` unless explicitly approved in the current turn.
7. Always run `git diff --check` before finalizing code or documentation changes.
8. Before any commit, run `git status --short` and `git diff --cached --stat`; verify the staged set
   excludes unapproved `data`, generated binaries, large media, and unrelated user changes.
9. After changing public headers, exported API, binding policy, or binding generators, refresh and
   validate local Python bindings with:

   ```sh
   just ctypes
   just ctypes-check
   ```


## Source Of Truth To Read First

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

If documentation-planning files conflict, prefer `spec/docs/V04_DOCUMENTATION_DECISIONS.md`, then
reconcile older planning text as part of the campaign.


## Target Documentation Product

The final site should feel like one public product, not a repository of agent notes.

Required qualities:

1. Clear first-viewport positioning: Datoviz v0.4 is a low-level GPU scientific visualization
   engine with C, direct-engine Python, Vulkan, and experimental WebGPU surfaces.
2. A progressive path for each major reader:
   - Python scientists using the direct-engine facade or raw `ctypes` with AI assistance;
   - C/C++ application developers;
   - browser/WebGPU integrators;
   - Vulkan/vklite and embedding developers;
   - contributors and coding agents.
3. A gallery-first examples experience:
   - one public visual family gets one minimal example;
   - one public feature gets one minimal example;
   - one semantic composite gets one minimal example;
   - showcases are polished composed stories, not substitutes for minimal examples;
   - each promoted example has one visual artifact when possible;
   - each live WebGPU example has both an embedded iframe and a standalone link.
4. Consistent tone, terminology, status labels, and page structure.
5. Clear progression from install to first render to examples to task guides to reference.
6. Verified, runnable snippets whenever a snippet is presented as copy/paste code.
7. Explicit limits for experimental, advanced/unstable, deferred, native-only, and external/GSP
   features.
8. Strong cross-links between task guides, examples, visual-family pages, generated gallery detail
   pages, and API reference.
9. Website media that loads cleanly, has useful alt text, and shows real Datoviz outputs.
10. No stale v0.3 promises, hidden compatibility assumptions, obsolete wrapper examples, or
    internal planning language in public pages.


## Audience And Positioning

Primary reader: scientists and technical users who need Datoviz v0.4 now, especially Python users
waiting for VisPy2/GSP and using the direct-engine facade or raw `ctypes` layer with AI assistance.

Secondary readers:

1. C/C++ application developers;
2. browser/WebGPU integrators;
3. Vulkan/vklite users;
4. embedding and Qt/provider developers;
5. contributors and coding agents.

Positioning rules:

1. C engine and scene/app path are first-class.
2. `datoviz.raw` is the exact generated `ctypes` layer.
3. Top-level `import datoviz as dvz` is the planned/active array-aware direct-engine facade where
   policy supports it.
4. High-level plotting APIs belong to GSP/VisPy2, not Datoviz v0.4.
5. WebGPU/WASM is experimental but real and website-visible for promoted examples.
6. DRP2 is an internal/advanced transport, not the main public layer.
7. Datoviz is not domain-specific. Neuroscience, geoscience, molecular, climate, image, graph, and
   engineering examples should coexist without one domain dominating the public identity.


## Required First Pass

Before broad rewriting, produce a short reviewable plan for maintainer feedback. Do not start the
full rewrite until the maintainer approves the information architecture, tone sample, and release
posture.

Required first-pass deliverables:

1. Inventory active public documentation, generated outputs, website assets, and excluded legacy
   docs.
2. Identify stale, duplicate, legacy, missing, structurally misplaced, or public/private mixed
   content.
3. Confirm or revise the proposed final information architecture and top-level MkDocs navigation.
4. Propose the website integration plan for gallery media, videos, screenshots, WebGPU live embeds,
   and missing media.
5. Propose the examples/gallery product model:
   - one visual = one minimal example;
   - one feature = one minimal example;
   - one showcase = one polished composed story;
   - one public example = one visual artifact where possible.
6. Rewrite a small representative page set to establish tone, structure, and cross-linking.
7. Wait for maintainer feedback.

Preferred tone sample set:

1. homepage or landing page;
2. install page;
3. quickstart page;
4. one how-to page;
5. one visual-family or API reference page;
6. one generated-gallery/template change if gallery tone is part of the proposal.

Landing and quickstart samples should show Python first, then C. Generated example/reference pages
may remain C-first where C is the executable source of truth.

After approval, work autonomously through the rewrite with checkpoint commits.


## Scope

This is an aggressive pre-RC rewrite. You may delete, move, merge, split, or rewrite public docs
when it improves structure, progression, correctness, tone, or maintainability.

Keep useful information. If deleted legacy content contains still-valid information, migrate that
information into the new structure before removal. If information is obsolete, contradictory, or
tied to v0.3-era APIs, remove it instead of preserving compatibility language.

In scope:

1. `mkdocs.yml` navigation and site configuration;
2. public docs under `docs/`;
3. generated gallery pages under `docs/examples/`;
4. gallery/example metadata in `examples/c/MANIFEST.yaml`;
5. gallery generation templates and scripts;
6. WebGPU live-example integration under `examples/webgpu/` and docs embeds;
7. docs CSS and layout under `docs/stylesheets/`;
8. generated C reference pages and source tooling when needed;
9. Python direct-engine and raw `ctypes` docs;
10. media integration for screenshots, PNG/WebP assets, videos, and browser embeds;
11. release/status/reference pages that users will read before RC;
12. README, citation, and release notes where public posture must be consistent.

Out of scope unless the maintainer explicitly expands the task:

1. changing runtime behavior to match docs;
2. committing `data` submodule changes;
3. publishing the docs site;
4. reviving v0.3 Python-first documentation as current v0.4 guidance;
5. implementing large new runtime features to fill documentation gaps.


## Execution Phases

Use checkpoint commits after each coherent phase and validation pass. Prefer small, reviewable
commits with direct messages. Do not commit unrelated user changes.

### Phase 0: Audit And Plan

Output: maintainer-reviewable plan, not a broad patch.

Tasks:

1. Build an inventory of active nav pages, unlisted docs, excluded legacy docs, generated pages,
   generated assets, and source metadata.
2. Run targeted searches for stale terms: `WIP`, `placeholder`, `<fill`, `legacy path`,
   ambiguous "old API" wording, v0.3 wrappers, stale attribute names such as `"diameter"`.
3. Compare `docs/reference/index.md`, `mkdocs.yml`, and `not_in_nav` for discoverability drift.
4. Compare release/install posture across `README.md`, `docs/start/install.md`,
   `docs/releases/v0.4.0rc1.md`, `docs/how-to/c-integration.md`, and
   `docs/reference/build-options.md`.
5. Compare gallery JSON, manifest rows, generated pages, and WebGPU route registry.
6. Inspect generated gallery detail pages for user experience and metadata leakage.
7. Propose final IA, page templates, gallery templates, media plan, validation commands, and
   subagent split.

### Phase 1: Tone Sample

Output: small patch plus maintainer feedback request.

Suggested files:

1. `docs/index.md`;
2. `docs/start/install.md`;
3. `docs/start/quickstart.md`;
4. one `docs/how-to/*.md`;
5. one `docs/reference/visual-families/*.md` or `docs/reference/ctypes.md`;
6. `tools/build_gallery.py` or its templates if generated gallery tone is sampled.

The sample should demonstrate:

1. final tone and status language;
2. screenshot/live-example treatment;
3. C and Python direct-engine/raw-binding positioning;
4. cross-linking pattern;
5. page-level structure.

### Phase 2: Navigation And Structure

Tasks:

1. Update `mkdocs.yml` only after IA approval.
2. Move, delete, or quarantine stale docs.
3. Reconcile `spec/docs/INFORMATION_ARCHITECTURE.md` with current decisions.
4. Rename the public secondary section from `Internals` to `Advanced` unless the maintainer reverses
   this preference.
5. Move top-level `Contributing` content under `Advanced` or another secondary path unless the
   maintainer reverses this preference.
6. Keep public user paths separate from advanced, contributor, and release-maintainer content.
7. Ensure pages presented as primary reference material are discoverable from navigation.

### Phase 3: Get Started Path

Tasks:

1. Make the homepage and Get Started pages answer:
   - what Datoviz is;
   - who should use which layer;
   - how to install or build now;
   - how to render the first scene;
   - where to go next.
2. Reconcile source-only versus RC artifact-ready install language.
3. Fix README, homepage, quickstart, and first examples to use current attributes and call order.
4. Keep the first example zero-data-dependency.
5. Make AI-assisted workflow concrete and linked to examples, how-to pages, and reference pages.

### Phase 4: Examples And Gallery

Tasks:

1. Treat `examples/c/MANIFEST.yaml` as the authority for public example IDs, categories, titles,
   data contracts, WebGPU routes, gallery metadata, and agent-copy safety.
2. Ensure every public built example has a manifest row or explicit non-public classification.
3. Ensure lab and legacy examples do not leak into public release proof unless promoted.
4. Ensure public route IDs, C scenario IDs, and WebGPU routes have unambiguous mappings.
5. Generate WebGPU route counts from metadata, not hand-maintained prose.
6. Keep data preparation commands visible when required.
7. Make synthetic, simulated, generated, prepared, real, and external-data wording conditional.
8. Move maintainer-only fields out of primary user flow or keep them collapsed.
9. Show status badges near previews for native support and WebGPU support.
10. Replace generic `_Media pending._` with generated media, a specific reason, or a visible
    pre-RC media task.
11. Keep live WebGPU examples isolated in iframes with standalone links.
12. Improve unknown-route handling in `examples/webgpu/live.js` by linking back to the matrix or
    example index where relevant.

### Phase 5: How-To Guides

Tasks:

1. Make pages task-oriented, not feature dumps.
2. Use a consistent structure:
   - task statement;
   - when to use this pattern;
   - minimal sequence of calls;
   - canonical examples and source links;
   - ownership, lifetime, coordinate, async, backend, or validation details;
   - common mistakes;
   - see-also links.
3. Link to generated examples instead of duplicating full source.
4. Remove stale or partial Python snippets unless clearly labelled as partial.
5. Make copy/paste snippets runnable where they are presented as full examples.

### Phase 6: Reference

Tasks:

1. Reconcile feature/status, project/status, v0.3 visible parity, generated C reference, raw
   `ctypes`, direct-engine docs, and GSP/VisPy2 boundary language.
2. Keep support labels consistent: `supported`, `experimental`, `advanced/unstable`, `deferred`,
   `external/GSP`, and `native-only` where relevant.
3. Fix internal/transitional language at source comments or generator layer, then rebuild generated
   docs.
4. Bring each visual-family page to a consistent template:
   - description;
   - status and backend support;
   - attributes/parameters;
   - minimal C example link;
   - minimal Python direct-engine/raw-binding guidance where policy supports it;
   - screenshot;
   - related how-to and examples.
5. Do not hand-maintain exhaustive C or raw-binding symbol catalogs in prose.

### Phase 7: Advanced And Contributors

Tasks:

1. Keep vklite, canvas/stream, WebGPU renderer, DRP2, and architecture docs out of the main
   first-user path.
2. Preserve advanced docs where they help backend, embedding, or contributor users.
3. Remove private planning language from public pages or move it to `agents/` or `spec/` if it is
   still useful.
4. Keep contributor docs reachable under `Advanced` or another secondary path, not as a primary
   public user tab.
5. Keep contributor docs practical: source of truth, validation command, page type, unsupported
   feature status, and generated-output rules.

### Phase 8: Website Polish

Tasks:

1. Audit the built website, not only Markdown source.
2. Check landing hierarchy, first-viewport message, hero/media treatment, gallery density, mobile
   layout, and no overlapping text/media.
3. Check PNG/WebP availability, dimensions, alt text, and visual quality.
4. Check WebGPU iframe styling, loading behavior, fallback links, mobile sizing, and standalone
   routes.
5. Check link paths after page moves.
6. Keep CSS consistent across home, gallery, how-to, reference, and advanced pages.

### Phase 9: Final Proof And Release Reconciliation

Tasks:

1. Run strict docs build and relevant generated checks.
2. Re-run stale-language searches.
3. Reconcile release tags, citation, release notes, and version metadata.
4. Record skipped checks and environment limitations honestly.
5. Inspect staged files before each commit.


## Audit Leads To Verify Or Close

The following are historical or current leads. Verify each against the current tree before editing,
then fix it or record why it is closed.

### Release And Install Posture

Known pages to reconcile:

1. `README.md`;
2. `docs/start/install.md`;
3. `docs/releases/v0.4.0rc1.md`;
4. `docs/how-to/c-integration.md`;
5. `docs/reference/build-options.md`;
6. `docs/reference/citation.md`;
7. `CITATION.cff`.

Do not leave one page saying "source only" while another advertises published wheels. Before final
release, use `v0.4-dev`, `v0.4.0rc1`, or `<release-tag>` with explicit wording. Do not imply a
final Zenodo DOI before it exists.

### First-User Journey

Older orientation pages may exist outside the active nav:

1. `docs/start/what-is-datoviz.md`;
2. `docs/start/choose-your-layer.md`;
3. `docs/start/first-c-program.md`.

Decide whether to merge their essential content into the active Get Started path, re-add them to
nav, or keep them intentionally unlisted.

### Snippet Correctness

Check README, homepage, quickstart, how-to pages, Python direct-engine docs, and raw `ctypes` docs
for:

1. stale attribute names such as `"diameter"` where `"diameter_px"` is public;
2. undefined variables in minimal snippets;
3. outdated call ordering;
4. old Python wrapper classes or v0.3 plotting idioms;
5. partial snippets presented as copy/paste examples.

Known pages to check:

1. `README.md`;
2. `docs/index.md`;
3. `docs/start/quickstart.md`;
4. `docs/how-to/use-python.md`;
5. `docs/how-to/use-raw-ctypes.md`;
6. `docs/reference/python-direct-engine.md`;
7. `docs/reference/ctypes.md`.

### Gallery Generation And Missing Pages

Previously observed missing pages now appear to exist, but still verify generator consistency:

1. `technique_edl`;
2. `feature_multi_window`;
3. `feature_view_size_policies`;
4. `feature_datetime_axis`.

Add or strengthen checks that every public `docs/examples/examples.json` page exists and that public
manifest examples appear in the generated matrix or have an explicit visible exclusion.

### Gallery User Experience

Generated detail pages should become polished user pages. Check for:

1. duplicate or awkward breadcrumbs;
2. lowercase or terse descriptions;
3. important native/WebGPU status hidden inside collapsed details;
4. data preparation commands hidden inside collapsed details;
5. maintainer-oriented fields such as `Agent copy-safe`, `Build`, `Smoke`, and `Validation` in the
   primary user path;
6. generic data-submodule wording for synthetic or cache-backed examples;
7. `_Media pending._` on supported public examples;
8. missing screenshot alt text;
9. weak next/previous navigation;
10. live WebGPU routes without clear standalone links.

### WebGPU Status And Counts

`docs/reference/webgpu-subset.md`, `docs/examples/webgpu-matrix.md`, generated gallery pages,
`examples/webgpu/live_examples.js`, and manifest WebGPU metadata should agree on:

1. live route count;
2. planned/deferred/native-only status;
3. scenario IDs;
4. browser capability requirements;
5. unsupported-feature diagnostics.

### Public Reference Language

Search public docs and generated references for:

```text
WIP
placeholder
<fill
legacy path
old API
TODO
diameter
```

Prefer public support-status phrasing:

1. "current supported slice" instead of "WIP";
2. "four-corner form" or "corner-vertex compatibility form" instead of "legacy path" when the
   input remains valid v0.4 behavior;
3. `"diameter_px"` for current screen-space point/marker diameter attributes where applicable.

Fix generated reference wording at the source comment or generator layer, then rebuild generated
outputs.


## Coordinator And Subagent Model

Use subagents only when the coordinator can keep ownership boundaries disjoint. The coordinator owns
the final integration, generation order, validation, and commits.

### Coordinator Responsibilities

1. Read all source-of-truth docs first.
2. Create the first-pass plan and obtain maintainer approval before broad rewriting.
3. Assign subagents with explicit file ownership and no overlapping write sets.
4. Sequence generated outputs after metadata/template changes.
5. Review subagent patches for correctness, tone, API truth, and conflicts.
6. Run final validation.
7. Make checkpoint commits only with approved, relevant files.

### Subagent Report Contract

Each subagent should return:

1. files inspected;
2. files changed;
3. source-of-truth checks used;
4. unresolved decisions;
5. validation commands run and results;
6. generated outputs touched;
7. any paths intentionally left unchanged.

### Recommended Subagent Splits

#### First-User Journey And Release Posture

Owns:

1. `README.md`;
2. `docs/index.md`;
3. `docs/start/`;
4. `docs/releases/`;
5. release/citation wording in `docs/reference/citation.md` and `CITATION.cff` when in scope;
6. relevant `mkdocs.yml` nav entries.

Focus:

1. homepage;
2. install;
3. quickstart;
4. AI-assisted workflow;
5. version and artifact status consistency.

#### Gallery Generator And Metadata

Owns:

1. `examples/c/MANIFEST.yaml`;
2. `tools/build_gallery.py`;
3. `tools/build_examples_manifest.py`;
4. `tools/build_capabilities.py`;
5. `tools/build_gallery_webp.py`;
6. generated `docs/examples/`;
7. generated `docs/examples/gallery/`;
8. `examples/webgpu/live_examples.js`;
9. `examples/webgpu/live.js` when route diagnostics are in scope.

Focus:

1. polished generated pages;
2. metadata consistency;
3. screenshots/media status;
4. WebGPU status badges, route links, and matrix consistency.

#### How-To And Snippets

Owns:

1. `docs/how-to/`;
2. snippet-heavy sections of `docs/start/quickstart.md`;
3. `docs/reference/python-direct-engine.md`;
4. `docs/reference/ctypes.md`.

Focus:

1. task structure;
2. runnable snippets;
3. Python direct-engine/raw-binding boundaries;
4. example cross-links.

#### Reference And Status

Owns:

1. `docs/reference/`;
2. generated C reference source comments or generator files when needed;
3. `spec/api/` and `spec/bindings/` only if a documentation truth mismatch reveals a source issue;
4. `mkdocs.yml` reference navigation coordination.

Focus:

1. feature/status tables;
2. generated C reference wording;
3. visual-family page templates;
4. object lifetime, callbacks, queries, platform/build, WebGPU, compute+graphics.

#### Website Polish

Owns:

1. `docs/stylesheets/`;
2. homepage/gallery layout Markdown and generated templates as coordinated;
3. media presentation;
4. MkDocs build and screenshot review.

Focus:

1. visual density;
2. mobile layout;
3. iframe presentation;
4. image alt text and sizing;
5. CSS consistency.


## Validation

Use the narrowest useful loop while iterating, then a broader final loop.

Documentation-only:

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

Verify the staged set excludes unapproved `data`, generated/runtime binary payloads, vendored
runtime libraries, large media payloads, and unrelated user changes.


## Commit And Safety Rules

Do not stage, commit, or push:

1. `data` submodule gitlink changes without explicit approval in the current turn;
2. generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`, `*.so`, `*.dll`, `*.npy`,
   `*.npz`, or `.DS_Store`;
3. large screenshot/video/media payloads unless explicitly approved;
4. unrelated user changes.

If media regeneration reveals missing or stale images, record the needed command and artifact paths,
then ask before staging submodule or binary changes.

When deleting large doc areas, make the commit message explicit about why they were removed and
where any still-useful information moved.


## Done Criteria

The campaign is done when:

1. the maintainer has approved the IA and tone sample;
2. the public docs match the final pre-RC API and feature status;
3. the website has a coherent user path from landing page to examples/how-to/reference;
4. each public visual family has a minimal example or a visible deferment;
5. each public feature has a minimal example or a visible deferment;
6. showcases are polished, screenshot/live-first, and clearly distinct from minimal examples;
7. generated gallery pages and manifests are regenerated from source metadata without drift;
8. screenshots, videos, and WebGPU embeds are integrated consistently and degrade with clear links;
9. stale v0.3-era docs are removed, rewritten, or clearly excluded from the public site;
10. release/install/citation/version wording is consistent with the actual RC state;
11. validation commands and skipped checks are recorded;
12. `git diff --check` passes;
13. checkpoint commits contain only approved, relevant changes.
