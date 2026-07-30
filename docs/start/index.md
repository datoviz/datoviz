# Get started

<div class="dvz-doc-hero">
  <div class="dvz-doc-hero__content">
    <p class="dvz-doc-hero__eyebrow">Python + NumPy · C/C++ · Vulkan</p>
    <p>Datoviz turns scientific arrays into interactive 2D and 3D GPU visualizations. Start with Python and NumPy or build directly with C; both paths use the same scene, visual, and data contracts.</p>
    <div class="dvz-doc-actions">
      <a class="md-button md-button--primary" href="quickstart/">Python Quickstart</a>
      <a class="md-button" href="first-c-program/">First C Program</a>
    </div>
  </div>
  <figure class="dvz-output-example">
    <a href="quickstart/">
      <img src="../assets/gallery/v0.4/start/start_scatter.webp" alt="Ten thousand colored points rendered in a Datoviz window" loading="lazy">
    </a>
    <figcaption><strong>Quickstart result.</strong> The same retained scene and array contracts are available from Python and C. <a href="quickstart/">Open the canonical example and source.</a></figcaption>
  </figure>
</div>

<div class="dvz-section-grid">
  <a class="dvz-section-card" href="install/">
    <strong>Install Datoviz</strong>
    <span>Choose the current package or source-build path for your platform.</span>
  </a>
  <a class="dvz-section-card" href="quickstart/">
    <strong>Python quickstart</strong>
    <span>Render 10,000 points from NumPy arrays in an interactive window.</span>
  </a>
  <a class="dvz-section-card" href="first-c-program/">
    <strong>First C program</strong>
    <span>Build and run the same scene through the native API.</span>
  </a>
  <a class="dvz-section-card" href="core-concepts/">
    <strong>Core concepts</strong>
    <span>Understand scenes, figures, panels, visuals, attributes, controllers, and views.</span>
  </a>
  <a class="dvz-section-card" href="choose-your-layer/">
    <strong>Choose an integration layer</strong>
    <span>Compare Python, C/C++, browser examples, and lower runtime layers.</span>
  </a>
  <a class="dvz-section-card" href="ai-workflow/">
    <strong>AI-assisted workflow</strong>
    <span>Ask a coding assistant for current, verified Datoviz v0.4 code.</span>
  </a>
</div>

## Recommended first path

<div class="dvz-step-flow" role="list" aria-label="Recommended first Datoviz workflow">
  <a class="dvz-step-flow__step" href="install/" role="listitem">
    <span class="dvz-step-flow__number">01</span>
    <strong>Install</strong>
    <span>Choose a package or source build.</span>
  </a>
  <a class="dvz-step-flow__step" href="core-concepts/" role="listitem">
    <span class="dvz-step-flow__number">02</span>
    <strong>Create a scene</strong>
    <span>Add a figure, panel, and visual.</span>
  </a>
  <a class="dvz-step-flow__step" href="quickstart/" role="listitem">
    <span class="dvz-step-flow__number">03</span>
    <strong>Add data</strong>
    <span>Upload arrays to named attributes.</span>
  </a>
  <a class="dvz-step-flow__step" href="../how-to/" role="listitem">
    <span class="dvz-step-flow__number">04</span>
    <strong>Interact or capture</strong>
    <span>Open a view or render offscreen.</span>
  </a>
</div>

Python users should run the complete [Quickstart](quickstart.md). C or C++ users should run the complete [First C Program](first-c-program.md), then integrate it with [CMake or `datoviz-config`](../how-to/c-integration.md). Read [Core concepts](core-concepts.md) once you have seen the first window.

If your target is a browser, Qt application, offscreen service, or lower runtime layer, use
[Choose your layer](choose-your-layer.md) before writing code. If a coding assistant will write the
first draft, use the [AI-assisted workflow](ai-workflow.md).

## After the Quickstart

- Browse [Examples](../examples/index.md) and adapt the closest working visual or feature.
- Use [How-To guides](../how-to/index.md) for focused tasks such as scenes, panels,
  interaction, annotations, capture, and export.
- Use [Reference](../reference/index.md) when you need exact status, supported attributes, or API
  details.
