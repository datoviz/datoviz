# Datoviz v0.4 Public Documentation Audit And Rewrite Handoff

Status: planned pre-RC documentation campaign.

This handoff is for a high-capacity agent auditing, restructuring, proofreading, and rewriting the
public Datoviz v0.4 documentation and website before RC publication. This is the single active
handoff for the public documentation rewrite; older public-doc audit notes have been folded here.


## Start Condition

The completed pre-RC public API cleanup branch has landed on `v0.4-dev`; the broad documentation
rewrite is unblocked.

The documentation must describe the final pre-RC API, not transitional pre-cleanup APIs. Treat
headers, exported API manifests, generated ctypes policy, examples, and generated references at
execution time as the source of truth.


## Mandatory First Pass

Before rewriting the full docs, produce a short reviewable plan for maintainer feedback.

Required first-pass deliverables:

1. Inventory the active public documentation and website surfaces.
2. Identify stale, duplicate, legacy, missing, or structurally misplaced content.
3. Propose the final information architecture and top-level MkDocs navigation.
4. Propose the website integration plan for gallery media, videos, screenshots, and WebGPU live
   embeds.
5. Rewrite a small representative set of first pages to establish tone and structure.
6. Wait for maintainer feedback before broad autonomous rewriting.

Preferred tone sample set:

1. landing page or homepage;
2. install page;
3. quickstart page;
4. one how-to page;
5. one visual-family or API reference page;
6. one generated-gallery/template change if gallery tone is part of the proposal.

After maintainer approval, work autonomously through the rest of the rewrite with checkpoint commits.


## Scope

This is an aggressive pre-RC rewrite. You may delete, move, merge, split, or rewrite public docs when
it improves structure, progression, correctness, tone, or maintainability.

Keep useful information. If deleted legacy content contains still-valid information, migrate that
information into the new structure before removal. If information is obsolete, contradictory, or tied
to v0.3-era APIs, remove it instead of preserving compatibility language.

In scope:

1. `mkdocs.yml` navigation and site configuration;
2. public docs under `docs/`;
3. generated gallery pages under `docs/examples/`;
4. gallery/example metadata in `examples/c/MANIFEST.yaml`;
5. gallery generation templates and scripts when needed;
6. WebGPU live-example integration under `examples/webgpu/` and docs embeds;
7. docs CSS and layout under `docs/stylesheets/`;
8. generated C reference pages and source tooling when needed;
9. Python direct-engine and raw `ctypes` docs;
10. media integration for screenshots, PNG/WebP assets, videos, and browser embeds;
11. release/status/reference pages that users will read before RC.

Out of scope unless the maintainer explicitly expands the task:

1. changing runtime behavior to match docs;
2. committing `data` submodule changes;
3. publishing the docs site;
4. reviving v0.3 Python-first documentation as current v0.4 guidance.


## Source Of Truth

Read these before planning:

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

Where documentation-planning files conflict, prefer
`spec/docs/V04_DOCUMENTATION_DECISIONS.md` and reconcile the older planning text in the same
campaign. Keep the main user path centered on:

```text
Get Started / Examples / How-To / Reference
```

Internals, contributor, and release-maintainer docs may remain reachable, but they should not
dominate the public user path.


## Audience And Positioning

Primary reader: scientists and technical users who need Datoviz v0.4 now, especially Python users
waiting for VisPy2/GSP and using the direct-engine or raw ctypes layer with AI assistance.

Secondary readers:

1. C/C++ application developers;
2. browser/WebGPU integrators;
3. Vulkan/vklite users;
4. embedding and Qt/provider developers;
5. contributors and coding agents.

Position Datoviz v0.4 as the low-level GPU scientific visualization engine:

1. C engine and scene/app path are first-class.
2. `datoviz.raw` is the exact generated ctypes layer.
3. top-level `import datoviz as dvz` is the planned/active array-aware direct-engine facade where
   policy supports it.
4. high-level plotting APIs belong to GSP/VisPy2, not Datoviz v0.4.
5. WebGPU/WASM is experimental but real and website-visible for promoted examples.


## Rewrite Requirements

Make the public docs feel like one coherent website, not an accumulation of agent notes.

Required qualities:

1. consistent tone, terminology, status labels, and page structure;
2. clear progression from install to first render to examples to task guides to reference;
3. enough detail for users and coding agents to succeed without reading old internal notes;
4. explicit limits for experimental, advanced/unstable, deferred, and external/GSP features;
5. code snippets verified against the final public API;
6. strong cross-links between task guides, examples, visual-family pages, and API reference;
7. website media that loads cleanly and explains the actual public surface;
8. no stale v0.3 promises, hidden compatibility assumptions, or obsolete Pythonic wrapper examples.


## Known Findings To Preserve

These are concrete public-site issues found before this handoff was consolidated. Verify each
against the current tree before editing, then fix or explicitly close it.

### Release And Install Posture

The public site has drifted between source-only, RC artifact-ready, and final-release language.
Known pages to reconcile together:

1. `docs/start/install.md`;
2. `docs/releases/v0.4.0rc1.md`;
3. `mkdocs.yml` release navigation;
4. `README.md`;
5. `docs/how-to/c-integration.md`;
6. `docs/reference/build-options.md`.

Decide the current public state with the maintainer if unclear. Do not leave one page saying
"source only" while another advertises passed or published wheels.

### First-User Journey

Important orientation pages have existed outside the main Get Started path:

1. `docs/start/what-is-datoviz.md`;
2. `docs/start/choose-your-layer.md`;
3. `docs/start/first-c-program.md`.

Make "what Datoviz is" and "which layer to use" visible in Get Started, or merge their essential
content into the first-run pages. Replace placeholder first-C-program content with a real
walkthrough or remove it from public discovery until complete.

### Stale Or Weak Snippets

The README first Python example previously used `"diameter"` while current marker docs use
`"diameter_px"`. Verify README, homepage, quickstart, and reference examples agree on attribute
names and call order.

Some "minimal" Python snippets used undefined variables such as `positions`, `colors`, or
`diameters`. Known pages to check:

1. `docs/how-to/use-python.md`;
2. `docs/how-to/use-raw-ctypes.md`;
3. `docs/reference/python-direct-engine.md`.

Make snippets fully runnable when they are presented as copy/paste examples. If a snippet is
intentionally partial, label it that way and link to the canonical runnable example.

### Gallery Generation And Missing Pages

Generated public gallery pages have drifted from manifest metadata. Previously observed missing
pages included:

1. `technique_edl`;
2. `feature_multi_window`;
3. `feature_view_size_policies`;
4. `feature_datetime_axis`.

Regenerate from metadata rather than hand-editing generated gallery Markdown. Add or strengthen a
check that every public `docs/examples/examples.json` page exists and that public manifest examples
appear in the generated matrix or have an explicit visible exclusion.

### Gallery User Experience

Generated detail pages have read like metadata dumps rather than polished user pages. Check for:

1. duplicate breadcrumbs such as `Examples / Examples`;
2. terse lowercase descriptions;
3. important native/WebGPU status hidden inside collapsed details;
4. data preparation commands hidden inside collapsed details;
5. maintainer-oriented fields such as `Agent copy-safe`, `Build`, `Smoke`, and `Validation`;
6. generic data-submodule wording shown for synthetic or cache-backed examples;
7. `_Media pending._` on supported public examples.

Preferred fixes:

1. show visible support and WebGPU status badges near the preview;
2. show visible data prerequisites before source links;
3. make runtime-data wording conditional for synthetic, cached, data-submodule, and generated-media
   cases;
4. keep maintainer-only fields collapsed or move them to contributor docs;
5. make the start example visible in the examples index with a real Start lane/card;
6. generate media before RC or visibly label the reason for a media gap.

### WebGPU Status And Counts

Individual example pages should surface WebGPU status without requiring users to open collapsed
metadata. `docs/reference/webgpu-subset.md`, the WebGPU matrix, gallery pages, and live-route
registry should agree on counts and route status. Non-live pages should link to the matrix or subset
page so users can distinguish planned/deferred from broken.

Improve unknown-route handling in `examples/webgpu/live.js` by linking back to the matrix or example
index when relevant.

### Reference Discoverability

If reference index pages present a page as core material, navigation should make that page easy to
rediscover. Pages to check include:

1. `docs/reference/project-status.md`;
2. `docs/reference/visual-attributes.md`;
3. `docs/reference/queries.md`;
4. `docs/reference/errors-and-logging.md`;
5. explanation pages linked from `docs/explanation/`.

Either bring them into navigation where users expect them, or stop presenting them as primary
reference pages.

### Public Reference Language

Generated C reference and public docs should not leak internal transitional terms. Search public
docs and generated references for terms such as `WIP`, `placeholder`, `<fill`, `legacy path`, and
ambiguous "old API" wording.

Prefer public support-status phrasing:

1. "current supported slice" instead of "WIP";
2. "four-corner form" or "corner-vertex compatibility form" instead of "legacy path" when the input
   remains valid v0.4 behavior.

Fix generated reference wording at the source comment or generator layer, then rebuild generated
outputs.

### Release Tags, Citation, And Version Metadata

Public docs previously used `GIT_TAG v0.4.0` before the final release existed. Before final release,
use `v0.4-dev`, `v0.4.0rc1`, or `<release-tag>` with explicit wording. At final release, replace
placeholders with the real tag.

Align `CITATION.cff`, `docs/reference/citation.md`, release notes, and actual tag state. Do not
imply a final Zenodo DOI or final v0.4.0 release before it exists; RC citation examples should be
clearly provisional.

Generated docs must be handled through their source metadata, templates, or generation tools.
Do not hand-edit generated gallery Markdown as the primary fix. The current generated-gallery source
chain includes:

```text
examples/c/MANIFEST.yaml
tools/build_gallery.py
tools/build_examples_manifest.py
tools/build_capabilities.py
tools/build_gallery_webp.py
docs/examples/
```


## Website And Media Integration

Audit the built website, not only Markdown source.

Check:

1. landing page hierarchy, hero/media treatment, and first-viewport message;
2. MkDocs navigation and section order;
3. gallery card consistency and visual density;
4. detail pages for generated examples;
5. PNG/WebP availability, dimensions, alt text, and visual quality;
6. video pages and generated video guidance;
7. WebGPU iframe styling, loading behavior, fallback links, mobile sizing, and standalone routes;
8. CSS consistency across home, gallery, how-to, reference, and advanced pages;
9. mobile layout and no overlapping text/media;
10. link paths after page moves.

Live WebGPU examples should stay isolated in iframe routes such as:

```text
examples/webgpu/live.html?id=<example-id>
```

Each embedded live example needs a standalone link. Do not inline the WebGPU runtime directly into
ordinary Markdown pages unless a later maintainer decision changes the architecture.

Gallery screenshots are sourced from the `data` submodule and converted to build-local WebP assets.
You may audit, regenerate, and report media needs, but do not stage or commit `data` gitlink changes
or generated binary/media payloads without explicit maintainer approval in the current turn.


## API Truth Check

For every user-facing API claim, verify against final pre-RC sources:

1. public headers in `include/datoviz/`;
2. exported API/status manifests under `spec/api/`;
3. binding policy under `spec/bindings/`;
4. generated ctypes output and checks;
5. canonical examples under `examples/c/`;
6. generated API docs from `tools/build_api_c.py`;
7. tests where behavior is contract-sensitive.

After public headers, exported API, binding policy, or binding generators change, follow the repo
rule: refresh and validate local Python bindings with `just ctypes` and `just ctypes-check` before
Python, GSP, or packaging validation.


## Suggested Work Phases

Use checkpoint commits after each coherent phase and validation pass. Prefer small, reviewable
commits with direct messages.

1. **Audit and plan:** inventory docs, website surfaces, generated sources, stale content, and
   proposed IA. Commit only if the maintainer wants the plan recorded.
2. **Tone sample:** rewrite the agreed representative first pages and get maintainer feedback.
3. **Navigation and structure:** update `mkdocs.yml`, move/delete/quarantine stale docs, reconcile
   `spec/docs/INFORMATION_ARCHITECTURE.md` with current decisions.
4. **Get Started path:** install, quickstart, first C/Python/direct-engine path, AI-assisted
   workflow.
5. **Examples and gallery:** generated indexes, detail page templates, media links, metadata,
   WebGPU live embeds, gallery matrix.
6. **How-To pages:** task-oriented guides with canonical examples, ownership/lifetime notes, common
   mistakes, validation commands, and see-also links.
7. **Reference pages:** final API, visual families, objects/lifetimes, callbacks, platform/build,
   feature status, WebGPU subset, compute/graphics, ctypes/direct-engine.
8. **Advanced pages:** vklite, canvas/stream, WebGPU renderer, DRP2 internals only where needed for
   advanced users.
9. **Website polish:** CSS/layout, landing page, mobile checks, gallery media, iframe presentation,
   links, search/navigation consistency.
10. **Final proofread and validation:** full pass for tone, structure, broken links, stale claims,
    generated drift, and repository hygiene.


## Optional Parallel Work Split

If the coordinator uses subagents, keep write ownership disjoint and sequence generated outputs
after source metadata/template changes.

1. **Coordinator:** confirm publication target, enforce scope, sequence generation, review returned
   patches for overlap, run final validation, and commit coherent checkpoints.
2. **First-user journey and release posture:** own `docs/index.md`, `docs/start/`,
   `docs/releases/`, `mkdocs.yml`, `README.md`, and citation files when version wording is in
   scope.
3. **Gallery generator and metadata:** own `tools/build_gallery.py`, `docs/examples/`,
   `docs/examples/gallery/`, `docs/examples/examples.json`, `docs/examples/capabilities.json`,
   public metadata corrections in `examples/c/MANIFEST.yaml`, and user-facing route diagnostics in
   `examples/webgpu/live.js`.
4. **How-to pages and snippets:** own `docs/how-to/`, `docs/start/quickstart.md`,
   `docs/reference/python-direct-engine.md`, and `docs/reference/ctypes.md`.
5. **Reference and status polish:** own `docs/reference/`, generated C reference source comments or
   generator files when needed, and reference nav/index changes coordinated with the first-user
   journey owner.


## Validation

Use the narrowest useful loop while iterating, then a broader final loop.

Baseline for documentation-only commits:

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
4. generated gallery pages and manifests are regenerated from source metadata without drift;
5. screenshots, videos, and WebGPU embeds are integrated consistently and degrade with clear links;
6. stale v0.3-era docs are removed, rewritten, or clearly excluded from the public site;
7. validation commands and any skipped checks are recorded;
8. `git diff --check` passes;
9. checkpoint commits contain only approved, relevant changes.
