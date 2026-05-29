# Adding Examples

Examples are executable documentation. They should be useful to humans and safe for coding agents to
copy when users ask for a Datoviz visualization.


## Start From The Spec

Before adding or changing a public example, check:

1. `spec/docs/EXAMPLE_COVERAGE.md` for required coverage and metadata fields;
2. `spec/scene/examples/README.md` for example roles and agent-copyable rules;
3. `spec/scene/examples/TEMPLATE.md` for scenario notes;
4. the relevant visual, feature, runtime, or diagnostics spec.


## Minimal Examples

A minimal example should demonstrate one public visual or feature with the least surrounding code
needed to run it. It should keep setup, data binding, rendering, and cleanup visible.

Minimal examples should not:

1. depend on hidden global state;
2. introduce helper abstractions that hide ownership;
3. use old v0.3 API names;
4. mix unrelated visual polish into the basic path;
5. rely on a showcase as the only source for a feature.


## Metadata

Every documented example should eventually have metadata with:

1. stable `id`;
2. `source` path;
3. public feature or visual family;
4. `status`;
5. `role`;
6. `agent_copy_safe`;
7. backend and platform requirements;
8. validation commands;
9. reference and how-to links.

Use `agent_copy_safe: true` only for complete examples that are current enough to be copied into a
user project. Mark API sketches, pressure tests, and showcases as not copy-safe.


## Validation

At minimum, documentation-only example changes should pass:

```bash
git diff --check
```

Runnable example changes should also pass the narrow command documented in metadata, usually one of:

```bash
just build
just test scene
just spec-check
```

When a screenshot, readback, or fixture is part of the example contract, update or regenerate that
artifact through the documented workflow instead of hand-editing generated files.
