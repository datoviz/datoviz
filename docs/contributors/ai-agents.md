# AI Agents

Datoviz assumes contributors and users will work with coding agents. Treat agents as careful junior
contributors: give them stable context, make them start from current examples, and require the same
validation loops as human-written changes.


## Start Here

For implementation work:

1. read `AGENTS.md`;
2. read `agents/now/START.md`;
3. inspect the relevant `spec/` document before editing code;
4. use `rg` to find the current implementation and tests;
5. run the narrowest relevant validation command before finalizing.

For user-facing example or documentation work:

1. start from `spec/docs/AI_DOCUMENTATION.md`;
2. check `spec/docs/EXAMPLE_COVERAGE.md`;
3. follow the example rules in `spec/scene/examples/`;
4. update reference/how-to links when a public example or feature changes.


## Choose The Right Layer

Use this routing when asking an agent to write or modify Datoviz code:

| Need | Ask the agent to use | Do not ask for by default |
| --- | --- | --- |
| Normal C visualization | `scene` and `app` APIs | DRP2, vklite, Vulkan |
| Offscreen rendering or capture | scene/app offscreen examples | custom swapchain setup |
| Protocol fixtures or replay | DRP2/DVZR specs and examples | scene examples as protocol source |
| Raw Python FFI | `datoviz.raw` | Pythonic plotting helpers |
| High-level Python plotting | VisPy2/GSP | old Datoviz v0.3 APIs |

The normal generated-code path should be:

```text
figure -> panel -> visual or retained object -> data/resources -> render/show/capture
```


## Copy From Current Examples

Prefer examples that are:

1. complete source files;
2. covered by example metadata or a documented validation command;
3. written against the v0.4 scene/app API;
4. linked from a reference, how-to, or `spec/scene/examples/` entry.

If no copy-safe example exists, have the agent create the minimal example first or clearly mark the
code as an API sketch. Do not let a showcase example become the only source for a basic feature.

Use the example taxonomy when searching:

| Need | Start from |
| --- | --- |
| One visual family | `examples/c/visuals/` |
| One isolated feature or rendering technique | `examples/c/features/` |
| A composed workflow, real-data story, or polished goal | `examples/c/showcases/` or showcase-tagged manifest entries |

Treat `workflow`, `scientific`, `real-data`, `simulated`, `interactive`, `offscreen`, and domain
labels as tags. Do not add new public source folders for those concepts.


## Do Not Invent

Agents must not invent:

1. high-level C plotting functions such as `plot()` or `imshow()`;
2. Pythonic Datoviz v0.3 APIs;
3. `datoviz.raw` aliases that remove the `dvz_` prefix;
4. parallel Vulkan, presentation, frame-stream, or renderer paths;
5. ownership transfer rules that are not documented in public headers or specs;
6. examples that rely on hidden global state or undocumented runtime setup.


## Validate

For documentation-only changes, run:

```bash
git diff --check
```

For code or generated examples, also run the narrowest relevant command:

```bash
just build
just test scene
just test drp2
just spec-check
```

Use the command documented next to the example or subsystem when one exists. For Vulkan, canvas,
window, or live scene paths on macOS, run through the repository environment, for example
`direnv exec . just test scene`.
