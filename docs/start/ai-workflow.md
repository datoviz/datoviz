# AI-Assisted Workflow

Datoviz v0.4 works well with coding assistants. The API is flat, consistent, and fully
described in a single page you can paste as context. An LLM can write correct C or Python
code from the docs alone without needing to browse the codebase.


## Why It Works

- Every function follows one convention: `dvz_<noun>_<verb>`.
- The scene graph is shallow: scene → figure → panel → visual.
- Ownership is explicit — no hidden state or magic constructors.
- The entire API surface that most users need fits in one prompt.


## Workflow

1. Open [Quickstart](quickstart.md) and copy its full content.
2. Describe the visualization you want in plain language.
3. Specify Python or C and ask for v0.4 API only.
4. Paste the output into your project and run it.

For Python, prefer `import datoviz as dvz` over raw ctypes unless you need explicit pointer
control. For C, use the minimal pattern from Start Here as a structural template.


## Prompt Template

```
You are helping me write a Datoviz v0.4 visualization.

API reference (paste the full content of the Start Here page here):
[START HERE PAGE CONTENT]

Task: [describe what you want, e.g. "a scatter plot of 5000 points colored by a scalar value,
with a colorbar and pan/zoom"]

Requirements:
- Python: use `import datoviz as dvz` and `dvz.run(scene, figure, title="...")` to open the window
- C: follow the minimal code pattern structure from the reference above
- Use v0.4 API only — do not use v0.3 Pythonic names or any function not shown in the reference
- Do not invent function names; if something is not in the reference, ask me

Output: a complete, self-contained script I can run directly.
```


## Tips

- If the output uses raw C names in Python (`dvz_scene()`, `dvz_view_glfw()`), ask the model
  to switch to the high-level `import datoviz as dvz` wrapper.
- If the model invents functions, paste the [visual family reference](../reference/visual-families/index.md)
  for the specific visual type you need.
- For iterative work, ask the model to change one thing at a time rather than regenerating
  the full script — it's easier to spot regressions.
- For complex layouts (multiple panels, linked axes, colorbars), include a relevant
  [How-To guide](../how-to/create-multiple-panels.md) in the context.
