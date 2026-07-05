# Datoviz v0.4 Public Documentation Audit And Rewrite Handoff

Status: planned pre-RC documentation campaign.

This handoff is for a high-capacity agent auditing, restructuring, proofreading, and rewriting the
public Datoviz v0.4 documentation and website before RC publication.


## Start Condition

Do not start the broad rewrite until the current pre-RC public API cleanup/refactor has landed or
the maintainer explicitly freezes the public API surface for documentation work.

The documentation must describe the final pre-RC API, not the transitional API visible before the
cleanup campaign completes. Treat headers, exported API manifests, generated ctypes policy, examples,
and generated references at execution time as the source of truth.


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
campaign. The current desired public structure is five sections:

```text
Get Started / Examples / How-To / Reference / Advanced
```

Contributor and release-maintainer docs may remain reachable, but they should not dominate the main
user path.


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
