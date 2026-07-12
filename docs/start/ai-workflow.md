# AI-Assisted Workflow

You can ask a coding assistant to write Datoviz examples for you. The simplest approach is to send
the assistant to the documentation site and describe what you want to see.

For agents, the canonical public entry point is [AI Agents Start Here](../ai-agents.md). Use that
page when you want the assistant to follow the API-checking workflow rather than guessing from
memory.


## Start with this prompt

Copy this prompt and replace the task with your own visualization:

```text
Use https://datoviz.org/ai-agents/ and the linked Datoviz v0.4 documentation to write my example.

Task: create a scatter plot of 10,000 points, colored by a scalar value, with pan and zoom.

Prefer Python with `import datoviz as dvz` unless I ask for C.
Start by finding the closest working example on datoviz.org.
Then check the API reference before using each Datoviz function.
Make sure every function exists and that the signature, enum names, visual attribute names, and
array shapes/counts match the current documentation.
If a Python call does not support the needed array upload, use `datoviz.raw` with explicit
pointers/counts or switch to C and explain why.
Return a complete runnable example, then list the Datoviz pages and examples you used.
```

If the assistant cannot browse the website, give it the relevant page contents or attach the
[Quickstart](quickstart.md), one [How-To guide](../how-to/index.md), the closest
[example](../examples/index.md), and the required [reference page](../reference/index.md). Ask it to
state which API details it could not verify instead of guessing.

For C code, change the language line:

```text
Use C instead of Python. Follow the style of the documented C examples.
```


## Make the request specific

A good request says what should appear on screen. For example:

| Instead of... | Write... |
| --- | --- |
| "make a plot" | "show 50,000 2D points colored by a scalar value" |
| "add controls" | "let me pan and zoom the X/Y view with the mouse" |
| "make it 3D" | "show 3D points with an arcball controller" |
| "export it" | "render one frame to a PNG without opening a window" |


## Optional pages for better precision

The simple prompt above is enough to get started. For better precision, point the assistant to the
same public pages you would use:

- [Quickstart](quickstart.md) for the first complete Python and C examples.
- [Examples](../examples/index.md) for working visual and feature examples.
- [How-To guides](../how-to/index.md) for focused tasks.
- [Reference](../reference/index.md) for exact visual names, attribute names, and feature status.
- [AI Agents Start Here](../ai-agents.md) for the full agent workflow and verification checklist.

For Python, the normal v0.4 starting point is:

```python
import datoviz as dvz
```

Datoviz v0.4 starts from scenes, visuals, and explicit data arrays. For high-level plotting helpers
such as `scatter()` or `imshow()`, use GSP/VisPy2 when that layer is available. That layer is still
work in progress; in the meantime, use the generated Datoviz Python binding directly through the
documented Python entry points.


## A useful follow-up

After the assistant writes code, ask:

```text
Which Datoviz documentation pages did you use, and which function or attribute names came from
each page?
```

This makes it easier to spot invented functions before running the code.
