# Agent-First Documentation

> **Status:** active v0.4 documentation direction
> **Scope:** public homepage message, LLM-readable entry points, example discovery, and honest
> capability routing

Datoviz v0.4 documentation should assume that many users will not read the full manual first. They
will install Datoviz, paste a task or an existing Matplotlib example into a coding agent, and ask
for Datoviz code. The documentation must make that workflow reliable without encouraging agents to
invent unsupported APIs.


## Homepage Promise

The public v0.4 homepage may use the short path:

```text
pip install datoviz

Then ask your coding agent to use Datoviz for the visualization you want.
```

That promise is only valid when the documentation provides stable, machine-readable context for the
agent to discover:

1. which Datoviz layer to use;
2. which examples are safe starting points;
3. which features are supported, experimental, deferred, or external/GSP;
4. which validation command proves the generated code;
5. when Datoviz cannot yet satisfy the user's request.


## Agent-First Contract

Agents should be able to answer these questions before writing code:

| Question | Required source |
| --- | --- |
| What API layer should I use? | `docs/start/choose-your-layer.md` and Python binding scope docs. |
| Is this feature available? | `docs/reference/feature-status.md` and generated capability metadata. |
| What is the nearest working example? | `examples/c/MANIFEST.yaml` and generated example pages. |
| Is this example copy-safe? | example metadata and contributor/example rules. |
| What should I do if unsupported? | known gaps, feature status, and Datoviz/GSP boundary docs. |
| How do I validate this? | example metadata, `just` commands, and reference validation pages. |

If the documentation does not answer one of these questions, the agent should either choose a
smaller supported Datoviz slice or say that Datoviz v0.4 does not yet provide the requested feature.


## Public Entry Points

The v0.4 public docs should expose these agent-facing entry points:

1. homepage first-use prompt;
2. an agent quickstart page for users who ask assistants to write Datoviz code;
3. `llms.txt` or an equivalent root-level LLM index that links to the most stable pages;
4. feature/status and known-gap pages with explicit status labels;
5. example indexes generated from manifest metadata;
6. Matplotlib-to-Datoviz task guidance;
7. Python facade docs for `import datoviz as dvz`;
8. raw `datoviz.raw` docs for exact `ctypes` use.

The public message should be direct:

```text
Datoviz is agent-friendly, not magic. Ask an agent to start from current examples, preserve the
documented API names, check feature status, and report unsupported requests honestly.
```


## LLM Index

A public `llms.txt` should be short and curated. It should point to stable pages, not duplicate API
reference content.

Recommended sections:

1. install and first-use path;
2. choose-your-layer routing;
3. agent quickstart;
4. examples index and manifest;
5. visual families reference;
6. feature status and known gaps;
7. Python facade and raw binding docs;
8. ownership and lifetime rules;
9. Matplotlib translation guidance;
10. release notes and unsupported/deferred features.

Keep this index checklist-reviewed or generated during release preparation so stale links do not
become the agent entry point.


## Example Discovery

Agent-first documentation depends on a simple example taxonomy:

| Category | Meaning | Agent use |
| --- | --- | --- |
| `visuals` | one public visual family per example | learn the required data shape for a visual. |
| `features` | one isolated capability or technique per example | add one behavior without extra composition. |
| `showcases` | one goal-oriented composed example | adapt a complete scene or user workflow. |

`workflow`, `scientific`, `technique`, `real-data`, `simulated`, `fake-data`, `interactive`,
`offscreen`, and domain labels should be tags or metadata, not new public structural categories.

This lets agents decompose requests:

1. find a showcase with a similar goal;
2. trace its tags back to atomic visual and feature examples;
3. combine only the documented pieces;
4. report any missing feature instead of inventing API calls.


## Matplotlib Translation

Datoviz v0.4 should document Matplotlib translation as intent mapping, not API compatibility.

When a user posts Matplotlib code, the agent should:

1. identify the visual intent, such as scatter, image, mesh, axes, colorbar, or offscreen capture;
2. map each intent to a Datoviz visual, feature, or showcase example;
3. preserve data preparation in Python when appropriate;
4. use `import datoviz as dvz` for the planned array-aware direct-engine path;
5. say when a Matplotlib feature belongs to GSP/VisPy2 or is deferred in Datoviz v0.4.

Do not document this as a `pyplot` clone. Datoviz v0.4 is the direct engine layer.


## Release Phasing

Recommended release phasing:

| Release point | Required agent-first docs |
| --- | --- |
| RC1 | homepage positioning, agent quickstart draft, feature/status table, known gaps, Python facade scope. |
| RC2 | example metadata cleanup, generated Python example tabs where available, Matplotlib translation guide. |
| Final | `llms.txt`, capability JSON or equivalent, polished example search, final known-gap table. |

Do not block core v0.4 engine release on the entire agent-support pack, but do not publish the
homepage promise until enough of the entry path is real.
