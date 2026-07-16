---
date: 2026-07-16
slug: specifications-before-code-rebuilding-datoviz-with-coding-agents
categories:
  - Development
---

# Specifications before code: rebuilding Datoviz with coding agents

Datoviz v0.4 is the culmination of ideas I have been exploring for about fifteen years. It is also
the first version developed through sustained collaboration with coding agents—not just to write
code, but to help with architecture, API design, and the search for the right abstractions.

This changed what I could realistically attempt as the maintainer of a mostly one-person project.
It did not remove the need for expertise, judgment, or close review. Instead, it changed how I used
my time and how architectural decisions could be carried consistently through implementation,
tests, examples, documentation, bindings, and packaging.

The resulting release is now available as the
[first Datoviz v0.4 release candidate](datoviz-v0.4-release-candidate.md).

<!-- more -->


## A little history

The story started long before Datoviz. Frustrated by the performance of existing visualization
libraries, I began investigating GPU rendering for fast 2D scientific visualization in 2011 and
released an experimental project called Galry in 2012. The following year, Nicolas Rougier, Almar
Klein, Luke Campagnola, and I joined forces to create VisPy.

By 2015, I had become convinced that Python and OpenGL imposed fundamental limits on the kind of
library I wanted to build. When Vulkan was announced, I wrote
[a post about a possible compiler infrastructure for data visualization](https://cyrille.rossant.net/compiler-data-visualization/).
It described a low-level, language-independent runtime with modular APIs, desktop and browser
targets, and GPU compute working directly with visualization data. Many details have changed, but
the main vision is recognizable in Datoviz v0.4.

Vulkan was released in 2016, but it took me nearly four years to experiment with it seriously. I
knew OpenGL but had no experience with low-level GPU APIs, and the learning curve was daunting. In
late 2019, I finally started connecting the dots and quickly obtained promising results using the
raw Vulkan C API.

I worked intensely on the experiment throughout 2020, learning Vulkan and becoming more familiar
with C. In February 2021, I
[released the first experimental version of Datoviz](https://cyrille.rossant.net/datoviz/): a C
library built directly on Vulkan for high-performance 2D and 3D visualization, multiple
programming languages, GUIs, and GPU compute.

Over the next few years, the project went through several internal refactors while I continued
learning Vulkan and working out the architecture. Version 0.2 was released in 2024 with a reworked
architecture. Version 0.3 followed in May 2025, after a major effort to make the library easy to
package and install.

These ideas took shape over about fifteen years. Modern graphics APIs finally provided the
foundation I wanted for a fast, scalable, high-quality scientific visualization engine. The
remaining constraint was the time and manpower needed to turn that foundation into a coherent,
portable library. Datoviz remained mostly a one-person project, and progress was necessarily slow.


## Working with coding agents

I began using LLMs for software development in 2023, mostly through the ChatGPT web interface and
only to a limited extent for coding. In early 2026, I decided to use OpenAI Codex and Claude Code
more seriously—not just to write code, but to help with architecture, API design, and the search
for the right abstractions. I did most of this work in Codex with GPT-5.5, then GPT-5.6 Sol, and
used Claude Code for a smaller part.


### Specifications before code

At first, I did not ask the agents to implement anything. I worked with them on specifications.
The `spec/` directory grew to hundreds of Markdown files covering the system layer by layer and
module by module. We rewrote, split, merged, and reorganized them as decisions in one area affected
another. They became our shared working memory. Only after the architecture became coherent did we
turn the specifications into implementation plans and code.

The questions I tackled with the agents covered every level of the project. At the architectural
level, how should the public API be divided into layers? Which layer should own each resource and
its lifetime? How should the scene be separated from the rendering runtime? What should belong to
the scene, the rendering protocol, or a specific GPU backend? How could native Vulkan and browser
WebGPU share the same scene semantics without reducing everything to the lowest common denominator?
How could we decouple low-level GPU rendering from the more stable scientific visualization logic?
How much flexibility should be exposed, and at what cost in complexity?

Then there was the API as a whole. How could a large C API remain consistent? Where should we draw
the line between C and Python, and what should the Python layer adapt automatically? How could we
support other languages without letting generated bindings diverge? How could we keep the code
readable enough for non-GPU experts and Python programmers to contribute without hiding the power
of the underlying engine?

Portability and the long-term life of the project raised another set of questions. How could we
preserve modularity and a clear separation of concerns as the project grew? How could we test
ownership, lifetimes, and rendering behavior across different platforms and GPUs? How could we
automatically create self-contained packages for Linux, macOS, and Windows? How could we keep
examples, documentation, bindings, and the public API synchronized as the library evolved?

These questions were intertwined. A decision that made one layer cleaner could make another harder
to use, test, package, or port. The real difficulty was finding a coherent set of choices that
satisfied all these requirements at once. This is why I spent so much time discussing them with
high-reasoning agents before asking for implementation.

The quality of those discussions surprised me. In several cases, the agents proposed concepts I
had not considered or module boundaries cleaner than my initial designs—even after I had spent
fifteen years thinking about these problems.


### A concrete architectural example

One example is the canvas, stream, and sink system that emerged from these discussions. A canvas
produces GPU frames and sends them through a stream. Window presentation, offscreen rendering,
video recording, live-image export, and application-defined consumers are implemented as pluggable
sinks. Sinks can be registered and combined, so the same frame can, for example, be displayed and
recorded without adding special cases to the scene or renderer. The scene does not even know
whether its output will be displayed, saved, or discarded.

This separation sounds simple once it exists, but I had not found such a clean organization on my
own. Reaching it required careful decisions about ownership, borrowed GPU handles, synchronization,
resizing, resource lifetimes, and module boundaries. The agents helped identify an architecture
that could support more capabilities while making the overall system more coherent.


### From specifications to code

Once we agreed on a specification and plan, the agents could carry the work through implementation,
tests, examples, documentation, bindings, and build infrastructure. This reduced the gap between
an architectural decision and all the places where it had to be applied consistently. The result
still required close review, but the context behind each decision remained available throughout
implementation.

The implementation itself was often remarkably good: clear, consistent C code that fit naturally
into the existing project. That observation needs some qualification. Much of the implementation
was relatively straightforward once the architecture and contracts had been settled; it was not
dominated by novel, complex algorithms. The hardest problems were choosing the abstractions,
ownership rules, module boundaries, and relationships between layers.

The GPU code was highly specialized, but it did not start from zero. It drew on roughly fifteen
years of prior work, notably Nicolas Rougier's research on GPU-based scientific visualization,
described in several computer graphics journal publications, together with the accumulated
experiments behind VisPy and Datoviz. The agents could build on that specialized foundation rather
than inventing the techniques from scratch.


### A highly interactive process

This process was highly interactive and iterative, with me in the loop at every step.

The usual loop was:

1. discuss the architecture and API;
2. inspect the existing code and constraints;
3. agree on a written and committed plan;
4. implement the plan;
5. review the code and run focused tests;
6. look for inconsistencies, omissions, ownership problems, and shortcuts;
7. refactor and repeat.

Sometimes an agent took a local shortcut; sometimes I found a naming inconsistency, an omission, or
a missing case. Each discovery could trigger another audit or refactoring pass. We successively
refactored presentation, resource ownership, the low-level rendering layers, and finally the entire
public API. We could still be aggressive because Datoviz did not yet have too many users.


## What changed for me

The balance of my work changed. I spent far less time typing code and much more on architecture,
API decisions, review, testing, and steering. I could concentrate on deciding what should be built
and verifying that the result was sound.

This iterative process is much slower than pure vibe coding. Without close review and repeated
refactoring, that approach risks producing unmaintainable code—the kind of output often called AI
slop. It may be reasonable for some prototypes, but it was not acceptable for a core library with
a long-lived C API and explicit ownership rules. Implementing the entire scope manually, however,
would have been unrealistic for a mostly one-person project.

Datoviz is a library I want to maintain for many years. Other programs may come to depend on its
API and ownership rules; a quick demonstration is not enough.

Datoviz v0.4 is not the product of an autonomous coding agent. It is the result of close human-AI
collaboration built on years of prior work, explicit specifications, continuous review, and repeated
refactoring. That collaboration changed what I could realistically attempt. Version 0.4 is broader
and more coherent than I expected a mostly one-person project to become in this amount of time. It
remains imperfect, but coding agents helped turn years of ideas into a working system that is now
ready to be tested by users.

You can read the [v0.4 release-candidate announcement](datoviz-v0.4-release-candidate.md), explore
the [examples](../../examples/index.md), or try the release and share feedback.
