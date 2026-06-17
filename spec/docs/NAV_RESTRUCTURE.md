# MkDocs Navigation Restructure

Status: reconciled for the v0.4 documentation rewrite.
Current nav source of truth: `mkdocs.yml`.


## Goal

Use a compact public navigation that reflects Datoviz v0.4 ownership:

```text
Home | Get Started | Examples | How-To | Reference | Internals | Contributing
```

The split is intentional:

1. **Examples** are the executable catalog, screenshots, gallery, WebGPU live routes, and release
   proof.
2. **How-To** guides explain how to adapt those examples to practical tasks.
3. **Reference** pages provide exact facts: API, attributes, statuses, lifetimes, backend support.
4. **Internals** is small and reserved for lower-layer architecture.
5. **Contributing** is top-level so humans and agents changing the repo have a clear entry point.


## Top-Level Sections

### Get Started

Keep the current compact start path:

```yaml
- Get Started:
  - Install: 'start/install.md'
  - Quickstart: 'start/quickstart.md'
  - AI-assisted workflow: 'start/ai-workflow.md'
```

Other start pages may exist outside nav for future reuse, but should not distract from the first
render path.


### Examples

Keep Examples as the public gallery and source-of-truth layer. Generated example detail pages under
`docs/examples/gallery/` remain excluded from nav but are linked from overview pages and How-To or
Reference pages.


### How-To

Use the task-oriented structure recorded in `agents/now/HOWTO_DOCS_WRITING.md` and
`spec/docs/INFORMATION_ARCHITECTURE.md`. Do not put composed walkthroughs in the first-level
How-To nav unless they teach a reusable task pattern.


### Reference

Use structured subgroups:

```yaml
- Reference:
  - Overview
  - API
  - Visual families
  - Core reference
  - Compatibility
  - Backends
```

Keep DRP2 internals out of the public reference nav unless a release decision promotes them as a
public surface.


### Internals

Expose only durable architecture and lower-layer pages:

```yaml
- Internals:
  - Architecture:
    - Why Datoviz
    - Scene model
    - Performance model
    - Portability and WebGPU
  - Lower Layers:
    - vklite
    - Canvas and stream API
    - WebGPU renderer
```

Detailed execution plans, stale explanations, and narrow implementation notes should stay out of
nav or live in `spec/`.


### Contributing

Promote contributor docs to top-level:

```yaml
- Contributing:
  - Getting started
  - Adding content
  - AI workflows
  - Release
```


## Validation

```sh
python -m mkdocs build --strict
git diff --check
git status --short
```
