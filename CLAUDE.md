# Claude Code instructions for datoviz

## Next steps (read this first)

**`docs/architecture/next_raw_triangle_examples.md`** — HIGH PRIORITY.  Detailed brief for
implementing two C examples: `raw_triangle.c` (vklite draw commands into DvzCanvas, with
offscreen/GLFW/video backends) and `raw_triangle_drp2.c` (manual DRP2 stream, no canvas).
Includes all required API additions, exact code patterns, and pointers to existing reference
code.  This is the main next step in the examples/ roadmap.

## Git commits

Do not add `Co-Authored-By:` trailer lines to commit messages.

Group related changes into a single commit rather than making one commit per file or per concern. Prefer fewer, broader commits. Keep commit messages short — a concise subject line is usually enough; avoid long body paragraphs.
