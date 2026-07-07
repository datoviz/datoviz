# AI Agents Start Here

Use this page when you ask a coding agent to write Datoviz v0.4 code from the public website.
Datoviz is changing quickly before the v0.4 release, so agents must verify the current API instead
of relying on memory or older Datoviz examples.


## Required Workflow

Give the agent this workflow:

1. Read the task and decide whether the user wants Python, C, or both.
2. Start from the [Examples](examples/index.md), especially examples whose pages include complete
   source code.
3. Check the [C API reference](reference/c-api/index.md), [Python binding with NumPy arrays](reference/python-direct-engine.md),
   or [Python binding exact call form](reference/ctypes.md) before writing code.
4. Verify every Datoviz function used in the answer exists in the current reference.
5. Verify the signature: argument count, pointer/count pairs, enum names, string attribute names,
   and whether a Python call accepts a NumPy array directly.
6. Prefer documented v0.4 examples over older Datoviz v0.3 snippets or guessed plotting helpers.
7. Return complete runnable code and list the documentation pages used.


## Prompt To Copy

```text
Use https://datoviz.org/ai-agents/ and the linked Datoviz v0.4 documentation to write my example.

Task: <describe the visualization here>

Prefer Python with `import datoviz as dvz` unless I ask for C.
Start by finding the closest working example on datoviz.org.
Then check the API reference before using each Datoviz function.
Make sure every function exists and that the signature, enum names, visual attribute names, and
array shapes/counts match the current documentation.
If a Python call does not support the needed array upload, use `datoviz.raw` with explicit
pointers/counts or switch to C and explain why.
Return a complete runnable example, then list the Datoviz pages and examples you used.
```


## Important Boundaries

Datoviz v0.4 is the lower-level rendering engine. It gives direct control over scenes, figures,
panels, visuals, data arrays, windows, offscreen capture, and low-level Python bindings.

GSP/VisPy2 is the intended high-level scientific plotting layer, but it is still work in progress.
Until that layer is ready, Python users can use Datoviz directly through one generated `ctypes`
binding. The documented `import datoviz as dvz` path handles supported NumPy array uploads;
`datoviz.raw` is only for the exact pointer/count call form.

Do not invent high-level plotting functions such as `scatter()` or `imshow()` as Datoviz v0.4 API.
If the user asks for that style, explain that the high-level API belongs to GSP/VisPy2 and write the
closest current Datoviz scene example instead.


## Best Sources

- [Examples](examples/index.md): working v0.4 examples with source code.
- [Quickstart](start/quickstart.md): first Python and C examples.
- [Choose your layer](start/choose-your-layer.md): Python, C, WebGPU, exact binding calls, and GSP/VisPy2 boundaries.
- [C API reference](reference/c-api/index.md): exact native API names and signatures.
- [Python binding with NumPy arrays](reference/python-direct-engine.md): supported Python NumPy adaptation.
- [Python binding exact call form](reference/ctypes.md): explicit pointer/count calls through `datoviz.raw`.
- [WebGPU matrix](examples/webgpu-matrix.md): browser support status by example.
