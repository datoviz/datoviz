# Gallery Site Notes

This document owns public-doc-site implementation notes for the v0.4 gallery. Scene example style
rules live in [`../scene/examples/STYLE.md`](../scene/examples/STYLE.md).


## MkDocs Material Direction

For v0.4, keep the public website and documentation in one MkDocs Material site. The goal is not a
separate marketing stack; it is a visual documentation front door backed by reproducible gallery
assets. A custom web application becomes useful later only if Datoviz needs live WebGPU embeds,
benchmark dashboards, richer release/news pages, or product-level storytelling that MkDocs cannot
carry cleanly.

Use this order of complexity:

1. Markdown plus Material cards for most index pages.
2. Custom CSS in `docs/stylesheets/extra.css` for Datoviz-specific card grids, image overlays, dark
   gallery bands, responsive media, badges, and lead cards.
3. Generated Markdown for gallery pages from example metadata.
4. A page-local template override only for the homepage or top gallery index if Markdown becomes
   too constrained.
5. Additional JavaScript only for progressive enhancement such as lightweight filters,
   media lazy-loading, or WebGPU demo mounting.

Do not fork the Material theme for normal gallery work. Prefer small CSS and template overrides that
can survive theme upgrades.


## Page Shapes

| Page | Shape |
| --- | --- |
| Home | compact hero, six-card showcase strip, start links, current status |
| Gallery index | lane overview with large cards for showcases and smaller cards for visual/features |
| Showcase page | one large screenshot or looped video, short thesis, source link, run/capture command, feature tags |
| Visual-family page | controlled baseline image, supported attributes, minimal C example, deferred variants |
| Technique page | before/after or side-by-side image pair, limits, backend requirements |
| Runtime/WebGPU page | explicit experimental labels, supported subset, unsupported-feature diagnostics |

Example cards should use real `<img>` or `<video>` elements, not CSS background images, so alt text,
loading behavior, social previews, and future asset checks stay straightforward.


## Front-Page Card Set

When the homepage can show only six cards or videos, prefer the current v0.4 set:

1. dense LiDAR;
2. molecular arcball;
3. brain volume plus transparent mesh;
4. weather field;
5. linked probe plus colorbar panels;
6. WebGPU browser preview or high-density 2D signals.
