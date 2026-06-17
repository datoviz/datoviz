# AI-Assisted Workflow

Datoviz v0.4 works well with coding assistants. The API is flat, consistent, and fully
described through a small set of stable pages you can paste as context. An LLM can write useful C
or Python code from the docs when you include the relevant task guide, example page, and reference
page.


## Why It Works

- Every function follows one convention: `dvz_<noun>_<verb>`.
- The scene graph is shallow: scene → figure → panel → visual.
- Ownership is explicit — no hidden state or magic constructors.
- The public examples are executable and can be used as copy-safe source material.


## Workflow

1. Open [Quickstart](quickstart.md) and copy the relevant pattern.
2. Add one [How-To](../how-to/create-a-scene.md) page for the task and one generated
   [Example](../examples/index.md) page for the visual or feature.
3. Specify Python or C and ask for v0.4 API only.
4. Paste the output into your project and run it.

For Python, prefer `import datoviz as dvz` over raw ctypes unless you need explicit pointer
control. For C, use the Quickstart C pattern and canonical `examples/c/...` sources as structural
templates.


## Prompt Template

```
You are helping me write a Datoviz v0.4 visualization.

Context:
- Quickstart pattern: [paste the relevant Quickstart section]
- Task guide: [paste one How-To page]
- Canonical example: [paste or link one generated Example page]
- Reference page: [paste one relevant Reference page]

Task: [describe what you want, e.g. "a scatter plot of 5000 points colored by a scalar value,
with a colorbar and pan/zoom"]

Requirements:
- Python: use `import datoviz as dvz` and `dvz.run(scene, figure, title="...")` to open the window
- C: follow the minimal code pattern structure from the Quickstart and canonical example
- Use v0.4 API only — do not use v0.3 Pythonic names or any function not shown in the reference
- Do not invent function names; if something is not in the reference, ask me

Output: a complete, self-contained script I can run directly.
```


## Tips

- If the output uses raw C names in Python without the `dvz.` module prefix, ask the model to
  switch to the top-level `import datoviz as dvz` facade or to explicit `datoviz.raw`.
- If the model invents functions, paste the [visual family reference](../reference/visual-families/index.md)
  for the specific visual type you need.
- For iterative work, ask the model to change one thing at a time rather than regenerating
  the full script — it's easier to spot regressions.
- For complex layouts (multiple panels, linked axes, colorbars), include a relevant
  [How-To guide](../how-to/create-multiple-panels.md) in the context.
