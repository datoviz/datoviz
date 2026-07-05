# AI-Assisted Workflow

You can ask a coding assistant to write Datoviz examples for you. The simplest approach is to send
the assistant to the documentation site and describe what you want to see.


## Start With This Prompt

Copy this prompt and replace the task with your own visualization:

```text
Use https://datoviz.org/ to write a Datoviz v0.4 example.

Task: create a scatter plot of 10,000 points, colored by a scalar value, with pan and zoom.

Use the current Datoviz v0.4 API documented on the website.
Prefer Python with `import datoviz as dvz` unless I ask for C.
If you are unsure which function or attribute name to use, say which documentation page you checked
and what is still unclear.
Return a complete example I can run.
```

For C code, change the language line:

```text
Use C instead of Python. Follow the style of the documented C examples.
```


## Make the Request Specific

A good request says what should appear on screen. For example:

| Instead of... | Write... |
| --- | --- |
| "make a plot" | "show 50,000 2D points colored by a scalar value" |
| "add controls" | "let me pan and zoom the X/Y view with the mouse" |
| "make it 3D" | "show 3D points with an arcball controller" |
| "export it" | "render one frame to a PNG without opening a window" |


## What the Assistant Should Use

The assistant should browse the same public pages you use:

- [Quickstart](quickstart.md) for the first complete Python and C examples.
- [Examples](../examples/index.md) for working visual and feature examples.
- [How-To Guides](../how-to/create-a-scene.md) for focused tasks.
- [Reference](../reference/index.md) for exact visual names, attribute names, and feature status.

For Python, the normal v0.4 starting point is:

```python
import datoviz as dvz
```

Datoviz v0.4 does not provide high-level plotting helpers such as `datoviz.scatter()` or
`datoviz.imshow()`. That higher-level plotting interface belongs to VisPy2/GSP rather than
Datoviz v0.4.


## A Useful Follow-Up

After the assistant writes code, ask:

```text
Which Datoviz documentation pages did you use, and which function or attribute names came from
each page?
```

This makes it easier to spot invented functions before running the code.
