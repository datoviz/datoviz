# Datoviz v0.4 Documentation Architecture

This directory records the documentation structure and authoring rules for the v0.4 public
documentation rebuild.

`mkdocs.yml` is the current source of truth for the public documentation navigation. The files in
`spec/docs/` explain the intent, coverage rules, and authoring policy behind that navigation. When
the MkDocs navigation changes, update these specs to match it instead of treating an older spec list
as authoritative.

Datoviz v0.4 documentation is C-engine-first. It documents the low-level renderer/runtime,
native scene/app layer, raw generated `ctypes` bindings, backend portability surface, and
contributor workflow. It does not recreate the v0.3 Pythonic plotting documentation. High-level
scientific plotting belongs to VisPy2/GSP, with Datoviz as one rendering backend.


## Documents

- [INFORMATION_ARCHITECTURE.md](INFORMATION_ARCHITECTURE.md): public documentation structure,
  audiences, Diataxis mapping, navigation, and content boundaries.
- [EXAMPLE_COVERAGE.md](EXAMPLE_COVERAGE.md): required minimal C examples, feature coverage,
  showcase policy, and example metadata.
- [AI_DOCUMENTATION.md](AI_DOCUMENTATION.md): LLM-friendly documentation rules, user-facing AI
  support-pack plan, and contributor boundaries for users and agents.
- [GALLERY_SITE.md](GALLERY_SITE.md): MkDocs/gallery page-shape notes and front-page gallery
  policy.

AI-facing usage contracts are routed through this directory first. Scene-level default API guidance
lives in [`../scene/api/API_SURFACE.md`](../scene/api/API_SURFACE.md), copy-safe example policy
lives in [`../scene/examples/`](../scene/examples/), diagnostic shape lives in
[`../scene/validation/DIAGNOSTICS.md`](../scene/validation/DIAGNOSTICS.md), and Python scope lives
in [`../api/PYTHON_GSP_SCOPE.md`](../api/PYTHON_GSP_SCOPE.md) plus
[`../bindings/`](../bindings/).

Release readiness, RC process, launch communication, and scientific-dataset outreach policy live in
[`../release/`](../release/). This docs spec owns public documentation structure and example
coverage, not release sequencing or announcement policy.


## Site Generator Decision

Use MkDocs Material for the v0.4 documentation rebuild unless there is a concrete implementation
blocker.

Reasons:

1. The project already uses MkDocs Material and the maintainers like its documentation experience.
2. It provides polished navigation, search, code blocks, admonitions, and responsive layout without
   requiring a custom frontend.
3. It is a pragmatic fit for a fast v0.4 documentation reset.

Keep the content portable:

1. write mostly plain Markdown;
2. keep plugin usage minimal;
3. generate reference and example tables as Markdown where possible;
4. avoid theme-specific overrides unless the benefit is clear;
5. pin documentation dependencies;
6. keep a future migration path open for Zensical or a smaller static generator if the ecosystem
   shifts.

Zensical is not adopted for v0.4 until it is mature enough to replace MkDocs Material without
destabilizing the release documentation. Zola remains a reasonable fallback if the project later
prioritizes a single-binary, low-JavaScript static site over the Material documentation experience.


## Public Documentation Root

The v0.4 branch may aggressively rebuild `docs/` in place. The main branch and released website
remain the home of v0.3-era public documentation. In this branch, old v0.3 material may be deleted,
rewritten, or selectively mined when it still describes durable concepts.

Do not create a v0.3-to-v0.4 migration guide for the old Pythonic Datoviz API. Instead, document the
layer boundary:

1. use VisPy2/GSP for high-level scientific plotting;
2. use Datoviz v0.4 for the C engine, low-level runtime, raw bindings, and backend work;
3. treat old Datoviz Python plotting APIs as outside the v0.4 Datoviz documentation scope.


## Authoring Priorities

1. Make every public visual and public feature discoverable through a minimal runnable example.
2. Keep examples, reference pages, feature-status tables, and generated artifacts linked by stable
   identifiers.
3. Keep user-facing pages concise and task-oriented.
4. Keep architecture explanations explicit enough for contributors and coding agents to avoid
   inventing parallel runtime paths.
5. Prefer exact status labels over vague promises: `supported`, `experimental`,
   `advanced/unstable`, `deferred`, or `external/GSP`.
