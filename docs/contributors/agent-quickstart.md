# Agent Quickstart

Use this page when asking a coding agent to write Datoviz v0.4 example or user-code snippets. The
source of truth for public examples is `docs/examples/examples.json`, generated from
`examples/c/MANIFEST.yaml`.

Use [AI Agents](ai-agents.md) for broader implementation work. Use this page only for example and
snippet selection.


## Pick A Starting Example

Read `docs/examples/examples.json` and filter examples in this order:

1. match the requested `category`, primary field, or `tags`;
2. prefer `agent_copy_safe: true`;
3. prefer `status: supported`;
4. prefer atomic examples in `visual`, `feature`, or `composite`;
5. use `showcase` only when the user asks for a composed workflow, real-data story, or polished
   demo.

Use these category routes:

| User need | Manifest route |
| --- | --- |
| One visual family | `category: visual`, then `primary_visual` |
| One isolated feature | `category: feature`, then `primary_feature` |
| One semantic scene object | `category: composite`, then `primary_composite` |
| A full workflow or scientific demo | `category: showcase`, then `tags` and `data.kind` |

For capability-first lookup, use [Example Selection By Capability](example-selection-by-capability.md)
with `docs/examples/capabilities.json`.


## Respect Data Metadata

Treat `data.kind` as a constraint:

| `data.kind` | Agent behavior |
| --- | --- |
| `synthetic` | Safe to recreate in generated snippets. |
| `simulated` | Keep simulation setup visible; do not present it as measured data. |
| `prepared` | Use the documented preparation path or dataset metadata. |
| `real` | Preserve source, license, and provenance notes. |

Do not copy showcase dataset-loading code into a minimal user snippet unless the user explicitly
needs that dataset or workflow.


## Use Manifest Commands

Each JSON entry provides:

1. `source` for the current C file;
2. `page` for generated documentation;
3. `build_command`;
4. `smoke_command`;
5. `validation`;
6. `source_url` for public citation.

Run the entry's build or smoke command when changing runnable example code. For documentation-only
changes, run:

```bash
git diff --check
```

Regenerate the machine-readable manifest after editing `examples/c/MANIFEST.yaml`:

```bash
python3 tools/build_examples_manifest.py
```


## Do Not Infer Missing APIs

Generated snippets should use current v0.4 scene/app APIs and the active runtime path:

```text
scene frame plans -> drp2 command streams -> vklite runtime -> canvas/app presentation
```

Do not invent high-level plotting wrappers, v0.3-style Python APIs, parallel Vulkan wrappers, or
ownership rules not present in the current headers or specs.
