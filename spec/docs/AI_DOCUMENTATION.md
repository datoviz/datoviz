# AI-Friendly Documentation And Contributor Rules

Datoviz users and contributors are expected to use LLMs and coding agents. The documentation should
be structured so agents can retrieve the right context, write correct C examples, respect module
boundaries, and run the correct validation loops.


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
