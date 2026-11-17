# Vulkan Course Writing Style

Applies to `docs/gpu-graphics/`. The model chapter is [12-uniform-buffers.md](../../docs/gpu-graphics/12-uniform-buffers.md). The durable course contract is [spec/docs/VKLITE_GRAPHICS_TUTORIAL.md](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md).

## Voice

Simple, plain and friendly; never terse. The reader knows C but not graphics. Explain what we are doing, why, why this way, and where we are headed, one concept at a time, so the reader can generalize beyond Datoviz. Cut fluff, filler, hype, repetition and defensive caveats, never explanation. Prefer short sentences and concrete words. Use "we" for shared reasoning and "you" for instructions.

## Chapter shape

1. Title, italic line count, preview image (unchanged).
2. Opener, no heading, two or three short paragraphs: where we are (what the program does now, recalling the chapters that built it), what is missing and why it matters, and what this chapter adds and what the reader will see at the end. Then "In this chapter you will:" with three to five numbered steps.
3. Concept sections before code: the one or two new ideas, intuition first, then the Vulkan term in bold at first definition, then how it maps to the program. One analogy at most per concept, only when it helps. Keep or add a Mermaid diagram (`flowchart TD`, short labels) when a flow is easier to see than read.
4. Code sections in the order the reader edits, headings that name the goal ("Declare the slot in the pipeline layout"), not the API.
5. Unchanged boxes and sections, keeping their order: `## Build and run`, `!!! tip "Try it"`, `??? info "Under the hood: ..."`, `!!! warning "When it goes wrong"`.
6. `## What carries over`: two or three bullets on ideas that hold beyond Datoviz (raw Vulkan, WebGPU, other APIs, general GPU reasoning). Do not repeat another chapter's bullets.
7. `## Checkpoint`, listings, `Next:` line (unchanged).

## Around code

- Before each code block, say in plain words what the step does and why it is needed now. Say where it goes.
- After a block whose arguments are bare numbers, flags or NULLs, read the call out in words once ("in set 0, binding 0, there is 1 descriptor, visible to the fragment shader").
- Recall earlier concepts by chapter when they return ("chapter 6's vertex buffer with a different usage flag").
- Say what is copied and what is referenced, and how long each object must live, where it matters.

## Hard constraints

- Every fenced `c` or `glsl` block must remain byte-identical to its step source; `just vulkan-course-check` verifies it. Do not change, split, merge or reorder code blocks unless the step order in the prose was wrong.
- Do not change the Try it, Under the hood, When it goes wrong, Checkpoint and listing content unless it is factually wrong.
- Verify every claim against `examples/c/vulkan/stepNN.c`, its shaders and the vklite headers. Do not invent API behavior.
- One source line per paragraph or list item; no hard wrapping.
- Expect a chapter to grow by roughly a third. Growth must come from explanation, not from restating.
