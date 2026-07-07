# Website Transition Plan

This document records the v0.4 website direction without changing the current public v0.3 site
before the v0.4 documentation is usable.


## Decision

Build a v0.4 landing page and dark documentation redesign in parallel with the existing public
website. Do not replace the current `datoviz.org` root until v0.4 has at least an RC1-quality public
documentation set.

Preferred final public layout:

1. `datoviz.org/`: v0.4 landing page.
2. `datoviz.org/docs/`: v0.4 documentation.
3. `datoviz.org/v0.3/` or `datoviz.org/legacy/`: archived v0.3 documentation, if practical.

Before the final switch, expose v0.4 through a preview target such as `v04.datoviz.org`,
`preview.datoviz.org`, or a separate GitHub Pages branch/path.


## Rationale

The current public website still serves the v0.3-era documentation and remains useful to existing
users. The v0.4 branch is rebuilding the project surface around the C engine, retained scene API,
DRP2, Vulkan/vklite/canvas runtime, Python binding, and experimental WebGPU/WASM. Publishing the v0.4
landing page too early would make the public root attractive but less useful than the existing site.

The v0.4 website work should therefore be developed as a previewable release asset. Switch the
public root only when the landing page, dark docs theme, gallery media, feature status, and install
paths give early testers a complete enough entry point.


## Required State Before Public Switch

1. Install and build pages are accurate for supported platforms.
2. First C example is runnable and prominent.
3. Feature/status table uses `supported`, `experimental`, `advanced/unstable`, `deferred`, and
   `external/GSP`.
4. Known limitations are visible from the landing page and docs navigation.
5. Gallery/showcase pages have real deterministic media for the front-page card set.
6. Python binding, `datoviz.raw` exact-call scope, DRP2, and WebGPU/WASM scope are documented.
7. Current public URLs either keep working or redirect to a useful replacement/archive page.
8. The release notes or status page clearly identify v0.4 as RC, prerelease, or final.


## Visual Direction

Use the existing gallery identity direction:

1. dark-first documentation and landing page;
2. graphite/cyan base palette from `spec/scene/examples/STYLE.md`;
3. real gallery captures as hero/card media, not mockups;
4. restrained scientific UI style with status labels and source links;
5. a refreshed logo set covering wordmark, square mark, favicon, monochrome, and social preview
   uses.

The current wide rainbow/colormap wordmark may remain as a legacy/reference asset, but it should not
be the only identity asset for v0.4.


## Implementation Order

1. Keep v0.4 work on the v0.4 development branch and preview target.
2. Update MkDocs styling to dark-first while keeping content portable Markdown.
3. Build a compact landing page shape in MkDocs first; add a page-local template override only if
   Markdown and CSS are too constrained.
4. Route full docs under `/docs/` in the preview deployment.
5. Add deterministic gallery media and wire it into generated gallery pages.
6. Prepare redirect/archive rules for old v0.3 URLs.
7. Switch `datoviz.org` only after the required state above is met.


## Non-Goals For The First Pass

1. Do not build a separate custom frontend stack unless MkDocs cannot support the required landing
   and gallery pages.
2. Do not publish generated WebGPU/WASM bundles as committed artifacts without an explicit release
   artifact policy.
3. Do not remove or break the current v0.3 public documentation before a usable archive or redirect
   path exists.
