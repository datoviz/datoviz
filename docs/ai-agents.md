# AI Agents Start Here

This is the canonical public contract for coding agents that generate Datoviz v0.4 code. Datoviz
v0.4 is a release candidate and may still change before the final release, so use current
documentation and executable examples as evidence. Do not rely on model memory, v0.3 tutorials, or
plausible-looking function names.

<div class="dvz-context-strip">
  <span>Canonical agent entry point</span>
  <span>Datoviz v0.4 only</span>
  <span>Python or C</span>
  <span>Verify before generating</span>
</div>


## What Datoviz v0.4 is

Datoviz v0.4 is a GPU scientific-visualization engine. Its main public workflow is: create a scene,
figure, and panel; create a visual; upload typed arrays to named visual attributes; attach the
visual to the panel; then run a native view or capture offscreen output.

For Python, prefer `import datoviz as dvz`, which exposes generated `dvz_*` calls with documented
NumPy adaptation. Use `datoviz.raw` only for an exact pointer/count, callback, or ABI-level call
that the normal form does not cover. For native applications, use the C scene/app API. Browser
WebGPU support is an experimental subset, not a second scene API.


## Required workflow

Give the agent this workflow:

1. **Choose the language and output.** Use Python unless the user asks for C/C++, embedding, or an
   operation documented only in C. Decide between a native window, offscreen capture, or a promoted
   browser example.
2. **Find the closest executable example.** Search [Examples](examples/index.md) by visual family,
   feature, data kind, or domain. Adapt its object order and data contract; do not combine unrelated
   snippets until one minimal scene works.
3. **Read the task guide.** Use one focused [How-To guide](how-to/index.md) for layout, interaction,
   updates, capture, or integration.
4. **Verify every API element.** Check the [C API](reference/c-api/index.md),
   [Python API](reference/ctypes.md), and the relevant
   [visual-family page](reference/visual-families/index.md). Verify function existence, argument
   order, return behavior, enum names, string attribute names, array dtype/shape, item counts,
   coordinates/units, and Python adaptation.
5. **Check status and backend support.** Consult [Feature status](reference/feature-status.md) and,
   for browser work, the [WebGPU matrix](examples/webgpu-matrix.md). Never infer WebGPU support from
   native support.
6. **Write one complete minimal program.** Include imports or headers, deterministic or user data,
   scene construction, panel attachment, output/session code, and cleanup where the language
   requires it. Label any later fragment as an excerpt and name the objects it assumes.
7. **Report evidence and uncertainty.** List the example and documentation pages used. If a detail
   is not verified, say so and omit or isolate it; do not guess.


## Canonical prompt to copy

```text
Use https://datoviz.org/ai-agents/ and the linked Datoviz v0.4 documentation to write my example.

Task: <describe the visualization here>

Prefer Python with `import datoviz as dvz` unless I ask for C.
Start by finding the closest working example on datoviz.org.
Read the relevant How-To and visual-family pages, then check the API reference before using each
Datoviz function. Verify function signatures, return behavior, enum names, attribute names, array
dtypes/shapes/counts, coordinates/units, and backend status.
If a Python call does not support the needed array upload, use `datoviz.raw` with explicit
pointers/counts only when the Python reference documents that path; otherwise switch to C and
explain why.
Return one complete minimal runnable program. Label any additional fragment as an excerpt and state
what it assumes. Then list the Datoviz pages and examples you used and identify anything you could
not verify.
```


## Evidence precedence

When sources disagree, use this order:

1. current generated API reference and explicit status/reference contracts;
2. current generated example page and its canonical v0.4 source;
3. focused v0.4 How-To guidance;
4. architecture or explanation pages;
5. model memory, old releases, third-party snippets, and search summaries—never authoritative.

Generated inventories help an agent select evidence without guessing from filenames:

- [`llms.txt`](llms.txt) is the compact root discovery map for documentation-aware tools;
- [`examples.json`](examples/examples.json) describes current examples and source locations;
- [`capabilities.json`](examples/capabilities.json) maps visuals, features, data kinds, domains, and
  backend requirements to examples.

Use these inventories for discovery, then read the selected human page and API contract before
writing code.


## Important boundaries

Datoviz v0.4 is the lower-level rendering engine. It gives direct control over scenes, figures,
panels, visuals, data arrays, windows, offscreen capture, and low-level Python bindings.

GSP/VisPy2 is the intended high-level scientific plotting layer, but it is still work in progress.
Until that layer is ready, Python users can use Datoviz directly through one generated `ctypes`
binding. The documented `import datoviz as dvz` path handles supported NumPy array uploads;
`datoviz.raw` is only for the exact pointer/count call form.

Do not invent high-level plotting functions such as `scatter()` or `imshow()` as Datoviz v0.4 API.
If the user asks for that style, explain that the high-level API belongs to GSP/VisPy2 and write the
closest current Datoviz scene example instead.

Do not:

- preserve a v0.3 call merely because it appears in an old example;
- invent wrapper classes, keyword arguments, enum values, or visual attributes;
- pass an array without checking dtype, shape, contiguity requirements, and item-count semantics;
- omit `dvz_panel_add_visual()` after preparing a visual;
- present a function-body fragment as a runnable program;
- claim that a `webgpu-live` route proves full browser or adapter parity;
- use DRP2 or lower runtime APIs for an ordinary scene unless the task explicitly requires them.


## Output checklist

Before returning code, confirm:

- [ ] Language and output target match the request.
- [ ] The program starts from one named current example.
- [ ] Every Datoviz symbol and visual attribute was verified in current documentation.
- [ ] Array dtypes, shapes, counts, coordinates, and units are explicit.
- [ ] The scene includes figure, panel, visual data, panel attachment, and output/session setup.
- [ ] C ownership and cleanup order, or Python callback/buffer lifetime, is handled when relevant.
- [ ] Experimental, advanced/unstable, deferred, and external features are labeled honestly.
- [ ] The answer distinguishes complete code from excerpts.
- [ ] Evidence links and remaining uncertainties are listed.


## Best sources by need

| Need | Source |
| --- | --- |
| complete first program | [Quickstart](start/quickstart.md) |
| nearest executable visual or feature | [Examples](examples/index.md) |
| task workflow | [How-To guides](how-to/index.md) |
| visual attribute contract | [Visual families](reference/visual-families/index.md) and [visual attributes](reference/visual-attributes.md) |
| scale, normalization, category, and color contract | [Scales and colormaps](reference/colormaps.md) |
| exact C names and signatures | [C API reference](reference/c-api/index.md) |
| NumPy-adapted versus exact Python calls | [Python API](reference/ctypes.md) |
| object ownership and cleanup | [Objects and lifetimes](reference/objects-and-lifetimes.md) |
| current feature classification | [Feature status](reference/feature-status.md) |
| browser route support | [WebGPU matrix](examples/webgpu-matrix.md) and [WebGPU subset](reference/webgpu-subset.md) |
| C, Python, browser, or runtime choice | [Choose your layer](start/choose-your-layer.md) |
