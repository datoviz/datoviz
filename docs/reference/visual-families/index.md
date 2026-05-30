# Visual Families

Status: planned structured reference.

Each public visual family should have one reference entry. This page should combine structured
facts with short choice guidance.

Use this template for each visual:

```text
## <Visual Name>

Status:
Backends:
Use when:
Avoid when:
Data model:
Required attributes:
Optional attributes:
Controllers:
Picking/probing:
Backend notes:
Minimal example:
Related how-to:
```

The exact API facts may come from source-controlled metadata or generated reference data. The choice
guidance should be authored prose:

1. `Use when` explains the data shape and task where the visual is the best fit.
2. `Avoid when` points to neighboring visual families or deferred features.
3. `Backend notes` calls out native support, WebGPU/WASM gaps, and release limitations.
4. `Minimal example` links to the copy-safe C example that demonstrates valid setup and cleanup.

This reference should stay concise. Longer decision prose belongs in
[`Choose a visual family`](../../how-to/choose-a-visual-family.md).
