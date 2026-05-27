# Docs Authoring

Draft contributor stub.

Write public documentation in `docs/`, durable behavior specs in `spec/`, and execution handoff
notes in `agents/`. Keep pages short, task-oriented, and linked to runnable examples.


## Visual Gallery Pages

The v0.4 docs should stay on MkDocs Material, but gallery-facing pages should be more visual than
ordinary reference pages. Treat the homepage and gallery index as visual documentation surfaces, not
as a separate marketing website.

Use these mechanisms before considering a custom site:

1. Markdown plus Material card grids for visual indexes.
2. Custom CSS in `docs/stylesheets/extra.css` for Datoviz-specific media cards, overlays, large
   lead cards, badges, and responsive gallery layouts.
3. Generated Markdown from example metadata for showcase, visual-family, feature, technique, and
   runtime pages.
4. A page-local Material template override only for the homepage or top gallery page if Markdown
   layout becomes too constrained.
5. Small JavaScript only for progressive enhancement; screenshots, links, text, and run commands
   should remain useful without it.

Keep durable gallery design rules in
`spec/scene/examples/GALLERY_VISUAL_IDENTITY.md`. Keep example ownership, scenario IDs, lanes, and
metadata rules in `spec/scene/examples/EXAMPLE_ORGANIZATION.md`.

Material's `attr_list` and `md_in_html` extensions are already enabled, so pages can use blocks like
this:

```md
<div class="dvz-showcase-grid" markdown>

<a class="dvz-showcase-card dvz-showcase-card--large" href="../examples/showcases/lidar/">
  <img src="../assets/gallery/showcases/lidar.webp" alt="Dense LiDAR flythrough">
  <span class="dvz-card-label">
    <strong>Dense LiDAR Flythrough</strong>
    <small>Millions of points, EDL, fly camera</small>
  </span>
</a>

<a class="dvz-showcase-card" href="../examples/showcases/molecule/">
  <video autoplay muted loop playsinline poster="../assets/gallery/showcases/molecule.webp">
    <source src="../assets/gallery/showcases/molecule.webm" type="video/webm">
  </video>
  <span class="dvz-card-label">
    <strong>Molecular Arcball Viewer</strong>
    <small>Spheres, mesh context, SSAO/MSAA</small>
  </span>
</a>

</div>
```

Use normal `<img>` and `<video>` elements rather than CSS background images so assets can have alt
text, lazy-loading behavior, validation checks, and useful link previews.
