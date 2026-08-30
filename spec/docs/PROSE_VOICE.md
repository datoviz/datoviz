# Public documentation prose voice

Status: approved authoring policy for the Datoviz v0.4 public documentation. Updated: 2026-08-31.

This guide defines how authored public prose should sound. It complements the documentation architecture, factual sources, generated-content rules, and release-status vocabulary. It never overrides an API contract, specification, source file, test, manifest, or release record.

## Core voice

Write like an experienced engineer teaching another capable person. Use plain verbs, concrete subjects, and enough explanation to make the next action or concept clear. Keep the prose direct, technically exact, and confident without sounding promotional.

Do not flatten good writing merely because it is polished. Preserve an occasional memorable sentence, dry aside, or restrained joke when it clarifies the point and fits the page. Avoid giving every paragraph the same rhythm.

Use American English. Address the reader as "you" where it makes instructions or consequences clearer. Tutorials, how-to pages, troubleshooting, and blog posts may use natural contractions. Architecture and reference prose should usually remain neutral and use few contractions.

Keep useful em dashes that belong to the author's voice, but do not add them as a default rhythm. Use sentence-case headings. Follow the repository rule that Markdown paragraphs and list items remain on one source line.

## Preserve meaning

Humanization is a claim-preserving edit. Keep every fact, status, number, date, name, link, citation, API identifier, ownership rule, platform boundary, release boundary, and limitation unless authoritative repository evidence proves that it is wrong.

Do not invent a rationale, performance claim, compatibility promise, user outcome, source, or historical interpretation to make a paragraph flow better. When a sentence lacks the detail needed for a safe rewrite, simplify it or record the question for review.

Code blocks, command lines, YAML metadata, HTML structure, link targets, generated markers, and machine-readable values are not prose. Preserve them unless the assigned task and their owning workflow explicitly require a change.

After rewriting a passage, ask:

1. What still sounds generated or formulaic?
2. Did the rewrite add, remove, strengthen, weaken, or generalize any claim?

Treat an unsupported addition or lost distinction as a defect.

## Page modes

### Tutorials

Use a conversational teacher's voice. Address the reader directly, give each paragraph one job, and explain why when the reason helps the reader predict behavior or diagnose a failure. Keep strong, accurate phrases when they make a difficult concept easier to remember.

Example:

> Vulkan is an excellent way to talk to a GPU and a terrible way to learn graphics. Before you can draw the first triangle, a raw Vulkan program needs an instance, a physical device, a queue family, a logical device, a window surface, a swapchain and its image views, a command pool, and synchronization for every frame in flight. By then you have written around 1000 lines, and none of them is about the triangle.

An occasional phrase such as "complain, in words" is welcome when it adds character without weakening the technical explanation.

### How-to and troubleshooting pages

Lead with the task or decision. Use imperatives for actions and "you" when explaining a consequence. Keep routine steps compact, but explain choices that affect correctness, performance, ownership, or diagnosis.

Example:

> Start with the smallest scene that still reproduces the problem. Build it once, keep the output size fixed, warm up a few frames, and then time a fixed number of frames. Measure data generation, uploads, drawing, and screenshot or query work separately. Change one variable at a time so you know what affected the result.

When advice is central to using Datoviz correctly, state it early and give it structural emphasis. Do not hide it among equally weighted tips or inflate it with adjectives.

Example:

> Datoviz batches items inside a visual. The intended fast path is a small number of visuals, each containing many items. Put related points in one point visual, not one visual per point. Check and correct the scene structure before investigating lower-level rendering costs.

Reserve `important` admonitions for architectural usage rules such as batching, ownership, callback lifetime, and borrowed Vulkan handles. Ordinary reminders belong in the surrounding prose.

### Explanations and architecture

Use direct technical prose. Preserve distinctions, dependency order, ownership, and failure boundaries. Prefer a clear subject and verb over specification-like noun chains, but do not compress away information.

Example:

> Each figure frame has one scene-level plan, even when several panels contribute local nodes. The plan records its logical targets and stable resources, together with ordered upload, compute, render, copy, and readback nodes. Dependencies enforce upload before use, compute writes before render reads, rendering before readback, panel order, and the ordering of shared resources.

Introduce an unfamiliar term after the reader understands the behavior it names.

Example:

> After drawing a frame, Datoviz keeps the scene and its visualization objects. When positions, colors, the camera, visibility, or style changes, update the existing object instead of rebuilding the scene. This is what retained state means in Datoviz.

### Reference pages

Keep reference prose compact, neutral, and exact. State what an object or operation does, then its constraints, ownership, lifetime, and failure behavior. Do not add personality to generated symbol documentation, tables, signatures, or field descriptions.

### Landing and getting-started pages

Explain the product without advertising it. Say what Datoviz is, what readers can build, which environments are supported, and when its level of control is useful. Avoid unexplained internal terms in the opening.

Example:

> Datoviz is an open-source C engine with Python/NumPy bindings. Use it to build interactive 2D and 3D visualizations, native applications, and offscreen renderers on Linux, macOS, and Windows. Experimental WebGPU support brings a selected subset to web browsers.

> Datoviz provides direct control over scene objects and rendering rather than a high-level plotting interface. It is intended for applications that handle large datasets, update them frequently, need precise interaction, or integrate visualization into a native application.

### Blog posts

Use first person where the author is describing personal work, judgment, uncertainty, or history. Keep concrete details and uneven human rhythm. Remove grand framing, staged candor, and generic conclusions, but do not turn a personal account into institutional release prose.

Example:

> Datoviz v0.4 brings together ideas I have been working on for about fifteen years. It is also the first version I developed in sustained collaboration with coding agents. I used them to write code and to think through the architecture, design the API, and find better abstractions.

> That changed what I could realistically attempt as the maintainer of a mostly one-person project. The agents did not replace expertise, judgment, or close review. They changed where I spent my time and helped carry architectural decisions through the code, tests, examples, documentation, bindings, and packaging.

Personal claims deserve conservative edits. If a rewrite might change what the author believes, remembers, or wants to emphasize, keep the original meaning and record the passage for later review.

## Status and limitations

State what works first. Then name the boundary plainly and point to the authoritative status source. Avoid apologies, vague warnings, repeated caveats, and language that implies parity beyond the documented subset.

Example:

> Selected examples run in the browser through the experimental WebGPU/WASM path. Use these live routes to try the supported subset or test portability. Browser support is not equivalent to native Vulkan support: each example is labelled live, planned, deferred, or native-only.

Use the established status terms exactly where they apply: `supported`, `experimental`, `advanced/unstable`, `deferred`, and `external/GSP`. Do not replace them with optimistic synonyms.

## Patterns to remove

Rewrite passages that combine several of these patterns:

- inflated claims about importance, transformation, legacy, or a broader landscape;
- sales language, generic praise, or enthusiasm without evidence;
- vague sources or unnamed expert opinion;
- repeated `-ing` clauses that pretend to explain significance;
- stock transitions such as "let's dive in" or "here is what you need to know";
- unnecessary "not X but Y" contrasts, fake alternatives, or objections no reader raised;
- forced groups of three and repetitive sentence openings;
- elaborate substitutes for `is`, `are`, `has`, or another plain verb;
- headings repeated by the first sentence;
- bold mini-headings on every list item;
- generic conclusions about future promise or continued progress;
- several dramatic fragments in a row;
- chatbot greetings, offers, praise, or closing invitations that do not belong to the page.

One watched word or construction is not proof of generated prose. Preserve a real contrast, useful caveat, established technical term, deliberate repetition, or strong sentence when it earns its place.

## Scope of a prose pass

The ordinary humanization scope includes authored public prose in the Vulkan course, landing and getting-started pages, how-to guides, explanations, advanced guides, conceptual reference introductions, and blog posts.

Exclude generated C API pages, generated gallery entries, manifests, release evidence, historical release notes, agent execution records, and normative specifications from a broad rewrite. Edit those only for a concrete task under their owning workflow.

When prose review exposes a possible code defect, generated-content problem, or unsupported factual claim, report it to the owning lane instead of silently changing the implementation or evidence.
