# Datoviz Blog

The Datoviz website should host the canonical blog for release communication, project updates, and
technical articles. The project blog is preferred over a personal website so that posts remain close
to the documentation, examples, installation instructions, gallery, and release notes.

## Goals

The blog should help people discover Datoviz, understand where it fits, and decide whether to try it
or contribute. In particular, posts should:

- explain why Datoviz exists and what problems it solves in scientific visualization;
- show concrete examples that users can run with the current release or prerelease wheels;
- document the technical architecture for contributors and advanced users;
- connect Datoviz to the broader VisPy, Scientific Python, Vulkan, and open-source visualization
  ecosystems;
- be honest about prerelease status, API stability, supported platforms, and known limitations.

## Primary audiences

The first posts should speak to several overlapping audiences:

1. **Scientific Python users** who need interactive visualization for large 2D or 3D datasets.
2. **Existing VisPy and PyData users** interested in the next rendering layer for scientific
   visualization.
3. **Graphics and systems developers** interested in Vulkan, command streams, retained scene state,
   and future multi-backend rendering.
4. **Potential contributors and testers** who can exercise Datoviz on different platforms, GPUs, and
   scientific workloads.

## Recommended publication model

Use the Datoviz website as the source of truth, then adapt or syndicate selected posts elsewhere.

1. Publish complete posts on `datoviz.org/blog/`.
2. Share shorter announcements that link back to the canonical Datoviz post.
3. Adapt ecosystem-oriented articles for the Scientific Python Blog when the framing is educational
   rather than promotional.
4. Pitch deep Vulkan or rendering-architecture follow-ups to Khronos when there is a concrete
   graphics/API angle.
5. Use Hacker News, Reddit, Mastodon/Bluesky, LinkedIn, PyData channels, and VisPy discussions for
   distribution after there is a smooth install-and-demo path.

For Hacker News, use a regular link submission for blog posts. Reserve **Show HN** for a runnable
release or prerelease that people can install, try, and give feedback on immediately.

## Suggested first series

### 1. From VisPy to Datoviz

A history and motivation post that explains the path from VisPy and OpenGL-era scientific
visualization to a modern Vulkan-based rendering library. This should be the most accessible article
in the series.

Possible angles:

- why interactive scientific visualization still needs a fast rendering layer;
- what Datoviz inherits from more than a decade of VisPy and GPU visualization experience;
- why Vulkan became an attractive foundation;
- how Datoviz fits with VisPy 2.0 and backend-agnostic plotting efforts;
- what users should try today, and what remains experimental.

### 2. Inside Datoviz v0.4

A technical architecture post for contributors and advanced users. It should describe the v0.4
refactor without assuming readers know the codebase.

Possible angles:

- modular C library targets and focused test binaries;
- the Vulkan/vklite/canvas runtime stack;
- the scene layer as retained user-facing visualization state;
- DRP2 command streams as the rendering contract;
- why scene code should emit frame plans and DRP2 streams instead of calling Vulkan directly;
- how this boundary keeps a future WebGPU lane possible without forking scene semantics.

### 3. Datoviz v0.4 prerelease: help us test

A practical launch post tied to the prerelease. This should be written for users more than
contributors.

Include:

- a short summary of what is new;
- installation commands for release or prerelease wheels;
- two or three minimal examples with screenshots or GIFs;
- a benchmark or scale demonstration when available;
- supported platforms and GPU/driver expectations;
- known limitations and API-stability caveats;
- a clear request for bug reports, platform reports, examples, and scientific use cases.

## Distribution checklist

Before broadly sharing a post, make sure the linked user path is ready:

- the install command works for the intended audience;
- at least one short example can be copied and run;
- screenshots, GIFs, or benchmark figures are current;
- the README, gallery, and quickstart do not contradict the post;
- prerelease caveats are visible;
- issue-reporting links are easy to find.

## Tone and positioning

Avoid presenting Datoviz as a direct Matplotlib replacement. The stronger message is:

> Use Matplotlib for polished static figures; use Datoviz when interactivity, GPU rendering, and
> large scientific datasets matter.

Prefer posts that teach something useful even to readers who do not immediately adopt Datoviz. This
will make external venues more likely to accept adapted versions and will build long-term trust in
the project.
