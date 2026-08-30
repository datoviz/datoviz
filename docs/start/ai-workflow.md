# AI-assisted workflow

You can ask a coding assistant to draft a Datoviz example, but treat the result as code that still needs verification. Check it against the same examples and reference pages you would use when writing the program yourself. This matters because the direct Python API is lower-level than a plotting library and assistants may invent familiar-looking plotting calls that Datoviz does not provide.

The canonical instructions and copyable prompt live on [AI Agents Start Here](../ai-agents.md). This page explains how to make your request precise and review the result without duplicating that agent contract.


## Start in three steps

1. Open [AI Agents Start Here](../ai-agents.md) and copy its canonical prompt.
2. Replace the task placeholder with the visual result, interaction, output, and language you want.
3. Give the assistant access to `datoviz.org` so it can find an example and verify the current API.

For example, the task portion can be:

```text
Task: show 50,000 two-dimensional points. Read positions and one scalar per point from NumPy
arrays, map the scalar to a perceptually uniform colormap, add XY panzoom, and save one 1600 x 900
PNG without opening a window. Return a complete Python program.
```

If the assistant cannot browse, provide these four inputs:

- the [Quickstart](quickstart.md) for the complete program structure;
- the closest page from [Examples](../examples/index.md);
- the focused [How-To guide](../how-to/index.md) for the task;
- the relevant [Reference](../reference/index.md) contract.

Ask the assistant to mark any API detail it could not verify instead of guessing.


## Make the request specific

A useful request states the data, representation, interaction, output, and preferred language:

| Instead of... | Write... |
| --- | --- |
| "make a plot" | "show 50,000 2D points colored by a scalar value" |
| "add controls" | "let me pan and zoom the X/Y view with the mouse" |
| "make it 3D" | "show 3D points with an arcball controller" |
| "export it" | "render one frame to a PNG without opening a window" |

Include array shapes and dtypes when you know them. Otherwise, ask the assistant to state the contract it chose before showing the code.


## Review the answer before running it

Check that the answer:

- says whether the code is a complete program or an excerpt;
- imports `datoviz as dvz` for the normal Python path, unless it explains a need for `datoviz.raw`;
- uses a documented visual family and exact attribute names;
- gives each array's dtype, shape, and item-count relationship;
- creates the scene objects, attaches the visual to a panel, and opens a window or captures output;
- checks feature and backend status rather than assuming native/WebGPU parity;
- lists the example and reference pages used.

For Python, the normal v0.4 starting point is:

```python
import datoviz as dvz
```

Datoviz v0.4 starts from scenes, visuals, and explicit data arrays. It does not provide the old high-level Python plotting API. If an answer invents methods such as `scatter()` or `imshow()`, ask the assistant to restart from a current v0.4 example. High-level plotting belongs to GSP/VisPy2 and remains external work in progress.


## Useful follow-up requests

After the assistant writes code, ask:

```text
Which Datoviz documentation pages did you use, and which function or attribute names came from
each page?
```

For a larger result, also ask:

```text
Separate the minimal verified Datoviz program from optional extensions. For every extension, state
its feature status and the example or reference page that supports it.
```

This leaves you with a verified base even when an optional backend or integration feature is uncertain.
