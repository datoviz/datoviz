# AI-Friendly Documentation And Contributor Rules

Datoviz users and contributors are expected to use LLMs and coding agents. The documentation should
be structured so agents can retrieve the right context, write correct C examples, respect module
boundaries, and run the correct validation loops.

Agents are a first-class audience for v0.4 documentation. Many users will ask an assistant to write,
adapt, or debug Datoviz code instead of reading the whole API directly. The documentation therefore
has to make the preferred API path easy to infer, examples safe to copy, and failures specific enough
for generated code to be repaired.


## User-Facing AI Goals

Users should be able to ask an assistant to write Datoviz code and get code that follows the v0.4
surface rather than old Pythonic Datoviz APIs or invented plotting shortcuts.

Documentation should make these choices explicit:

1. ask for Datoviz C code when using the v0.4 engine directly;
2. ask for raw `ctypes` only when low-level Python binding access is intended;
3. ask for VisPy2/GSP when high-level scientific plotting is intended;
4. start from a named minimal example whenever possible;
5. preserve Datoviz ownership and destroy rules;
6. run the documented validation command for the example or subsystem.


## User-Facing AI Support Pack

The v0.4 documentation set should include a small AI/developer support pack for users who ask
assistants to write Datoviz code. The pack is a routing and recipe layer, not a new source of truth
for API behavior.

Recommended public layout:

```text
docs/ai/
  datoviz-c.md
  datoviz-ctypes.md
  api-status.md
  ownership-and-lifetimes.md
  examples.md
  SKILL.md
```

The pack should cover two supported user workflows:

1. C-first Datoviz development through the public headers, scene/app examples, and documented
   build/run commands;
2. low-level Python use through the exact generated `datoviz.raw` `ctypes` layer, with explicit
   NumPy buffer, dtype, contiguity, and lifetime rules.

The pack should also state the main routing boundaries:

1. Datoviz v0.4 owns the C engine, scene/app path, raw/generated Python binding surface,
   offscreen/raster capture, and experimental backend slices;
2. high-level object-oriented Python plotting belongs to VisPy2/GSP;
3. raw `ctypes` is not a plotting API;
4. old v0.3 Pythonic examples are not current Datoviz v0.4 guidance.

`docs/ai/SKILL.md` should be short and portable. It should tell coding agents where to start, which
examples to copy, what not to invent, and which validation command to run. It should link to the
canonical docs and specs instead of repeating symbol catalogs or behavior contracts.

The skill should instruct agents to:

1. prefer public `include/datoviz/` headers and current `examples/c/` files for C code;
2. prefer `datoviz.raw` for exact low-level Python bindings;
3. keep NumPy arrays alive while C code may read from them;
4. check dtype, shape, alignment, and contiguity before passing array memory through `ctypes`;
5. preserve documented ownership, callback, readback, and destroy-order rules;
6. avoid invented helpers such as `plot()`, `scatter()`, or `imshow()` unless they belong to
   VisPy2/GSP documentation;
7. include the narrowest documented build, run, or smoke command with generated examples.

`docs/ai/examples.md` should be recipe-oriented rather than exhaustive. The first recipe set should
cover:

1. create a scene/app canvas;
2. add points;
3. add an image;
4. add a mesh or textured mesh;
5. render offscreen and capture a raster image;
6. load `datoviz.raw` and check ABI/layout;
7. pass a NumPy array safely through raw `ctypes`;
8. clean up objects in the documented order.

If the public website publishes an `llms.txt` or similar LLM-readable index, it should point to this
support pack, the feature/status page, the public C API reference, raw binding docs, examples, known
issues, and release notes. Treat that index as a curated navigation aid, not as a replacement for
normal docs, sitemaps, or reference generation.


## Agent-Default API Path

For normal user requests, documentation should steer assistants toward the native scene/app layer:

```text
figure -> panel -> visual or retained object -> data/resources -> render/show/capture -> pick/probe
```

The lower layers remain documented, but they should not look like the default answer to ordinary
visualization prompts:

| User intent | Preferred Datoviz surface | Avoid as default |
| --- | --- | --- |
| Create a scientific visualization in C | `scene` and `app` APIs | DRP2, vklite, raw Vulkan |
| Render offscreen or capture an image | `app`/scene offscreen path | hand-built swapchains |
| Add point, image, mesh, volume, or text content | public scene visual/object API | custom shaders |
| Debug generated scene code | scene validation and diagnostics | backend-only error strings |
| Replay or test renderer protocol behavior | DRP2/DVZR | scene examples as protocol specs |
| Use Datoviz from Python at low level | `datoviz.raw` or narrow host helpers | invented plotting APIs |
| Use Pythonic scientific plotting | VisPy2/GSP | Datoviz v0.3-style APIs |

Pages should state this routing directly. If a feature is advanced, backend-specific, or unstable,
mark it as such instead of letting agents discover it through examples and assume it is the default.


## Copy-Safe Examples

Agents learn Datoviz patterns mostly from examples. Every public feature that users are expected to
request should have at least one complete, current, copy-safe example.

An example is copy-safe when it:

1. starts from the preferred `scene`/`app` ownership pattern;
2. uses current v0.4 names, not v0.3 Pythonic names;
3. demonstrates one visual, feature, or runtime path clearly;
4. declares the backend/runtime assumptions needed to run it;
5. links to the relevant reference or how-to page;
6. has a documented validation command or metadata entry;
7. avoids hidden global state and undocumented cleanup behavior.

When a page includes a code block that is not tested or not copy-safe, it should be labeled as a
sketch or fragment. Public how-to and tutorial pages should prefer complete examples generated from
or kept in sync with source files.


## Machine-Readable Context

Machine-readable metadata is part of the AI-era documentation contract. Agents and tooling should be
able to discover the same facts that prose pages expose:

1. public examples and their stable IDs;
2. visual families and required attributes;
3. feature status and backend support;
4. validation commands;
5. public headers and generated binding symbols;
6. diagnostic codes and meanings;
7. known limitations and preferred fallbacks.

The source of truth for these facts should live in `spec/`, generated manifests, or source-controlled
metadata. Prose pages should link to those sources instead of duplicating detailed API facts.


## Agent-Repairable Diagnostics

Documentation should pressure the implementation toward diagnostics that assistants can use to fix
generated code. Good diagnostics identify the scene-level problem first, then include backend detail
only as context.

Useful diagnostics include:

1. stable code;
2. severity;
3. phase such as validation, frame planning, submission, or completion;
4. subject kind and subject identity;
5. expected versus actual values when applicable;
6. concise message;
7. actionable hint.

For example:

```text
SCENE_VISUAL_MISSING_POSITION
severity=fatal
phase=validation
subject=visual:point
message=Point visual requires position data before rendering.
hint=Bind the position attribute before submitting the frame.
```

Avoid diagnostics that only expose backend handles, Vulkan error strings, or assertion locations
when the failure can be described in scene terms.


## LLM-Friendly Page Design

Use stable, machine-retrievable structure:

1. short pages with one clear purpose;
2. stable paths and headings;
3. explicit status labels;
4. copyable complete code snippets only when they are tested or generated from examples;
5. links from examples to reference pages and from reference pages back to examples;
6. tables for attributes, object lifetimes, backend support, and limitations;
7. `Do` and `Do not` sections for ownership, callbacks, readbacks, and GPU resource lifetimes;
8. validation commands close to the relevant task;
9. no stale planning notes in public user pages.

Public pages should prefer current API names and signatures. If a feature is not part of the v0.4
Datoviz surface, route users to VisPy2/GSP or mark it `deferred`/`external/GSP`.


## Example-Driven Agent Prompts

Each visual or feature page should make it easy to tell an agent:

```text
Start from examples/c/visuals/point.c.
Keep the scene/app ownership pattern.
Only add a colorbar; do not introduce Pythonic plotting APIs.
Run the documented validation command.
```

This works only if each page links to:

1. a minimal example;
2. the exact reference entry;
3. the relevant how-to guide;
4. validation commands or release proof status.


## AI-Era Public Surface Requirements

The v0.4 public documentation should make these requirements visible:

1. one obvious beginner path through `scene` and `app`;
2. canonical minimal examples for every public visual family;
3. canonical examples for offscreen capture, updates, picking/probing, and controllers;
4. explicit ownership and destroy-order rules near every allocating example;
5. feature status labels that prevent agents from overselling incomplete features;
6. generated or machine-readable indexes for examples, features, and API symbols;
7. diagnostics and validation APIs that fail early with scene-level messages;
8. Python scope boundaries that prevent `datoviz.raw` from being treated as a plotting API.


## Contributor And Agent Boundaries

Contributor documentation should tell agents to follow the same project boundaries as human
contributors:

1. read the branch dispatch notes before implementation work;
2. avoid editing active work lanes owned by another agent;
3. do not create parallel presentation, frame-stream, Vulkan-wrapper, or renderer paths;
4. keep high-level scientific plotting out of Datoviz v0.4 docs and code;
5. keep detailed behavior contracts in `spec/`;
6. keep public start, tutorial, example, how-to, reference, explanation, and contributor pages in
   `docs/`;
7. keep execution status and handoff notes in `agents/`;
8. do not hand-edit generated reference pages;
9. update example metadata and feature-status tables when public behavior changes;
10. run `git diff --check` for documentation edits and the narrowest relevant build/test loop for
    code or generated artifacts.

Agents should treat `mkdocs.yml` as the public documentation navigation source of truth. When adding
or moving user-facing pages, update `mkdocs.yml` and then mirror the intent in `spec/docs/` instead
of relying on stale page lists in planning notes.


## Documentation Boundaries For Agents

Agents may aggressively rebuild the v0.4 `docs/` tree in this branch. They should still preserve
clear source-of-truth boundaries:

| Content | Location |
| --- | --- |
| Public start/tutorial/example/how-to/reference/explanation/contributor pages | `docs/` |
| Documentation architecture and policy | `spec/docs/` |
| Scene and visual behavior contracts | `spec/scene/` |
| DRP2 command and fixture contracts | `spec/drp2/` |
| Public API conventions and language scope | `spec/api/` |
| Current execution status and work queues | `agents/` |
| Generated API/reference output | generated docs path, never hand-edited |


## AI-Relevant Pages

The current MkDocs navigation includes these AI-relevant and agent-oriented pages:

```text
contributors/ai-agents.md
contributors/docs-authoring.md
contributors/adding-examples.md
start/choose-your-layer.md
reference/feature-status.md
reference/objects-and-lifetimes.md
```

`contributors/ai-agents.md` should be concise and operational. It should explain how to gather
context, choose the right examples, avoid known anti-patterns, and validate changes.

Canonical AI-facing contracts are split as follows:

1. this file owns documentation policy and AI-facing page design;
2. `../scene/api/API_SURFACE.md` owns the scene/app default API path;
3. `../scene/examples/` owns copy-safe example policy and scenario metadata;
4. `../scene/validation/DIAGNOSTICS.md` owns agent-repairable diagnostic shape;
5. `../bindings/` and `../api/PYTHON_GSP_SCOPE.md` own Python scope boundaries.


## Anti-Patterns To Prevent

1. Inventing high-level plotting functions such as `plot()`, `scatter()`, or `imshow()` in Datoviz
   v0.4 examples.
2. Using old v0.3 Pythonic APIs as if they are the current surface.
3. Treating raw `ctypes` as a high-level Python API.
4. Copying implementation plans into public user docs.
5. Writing showcase examples before minimal examples exist.
6. Duplicating detailed API facts across many prose pages instead of linking to reference pages.
7. Hiding backend limitations or feature status.
8. Omitting ownership and destroy-order rules from examples that allocate public objects.
