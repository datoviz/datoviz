# Claude Code instructions for datoviz

## Next steps (read this first)

**`docs/architecture/drp2_base64_inprocess.md`** — HIGH PRIORITY (correctness/perf debt).
Eliminate base64 encode/decode from the in-process vklite execution path.  Base64 is only for
JSON wire serialization; raw `const void*` pointers should be used for everything else.
Detailed fix approach and file list in the doc.

**`docs/architecture/next_scene_examples.md`** — next scene+app examples: `hello_scatter.c`
is done; `hello_triangle.c` and `hello_texture.c` each need a new visual constructor first.

## Git commits

Do not add `Co-Authored-By:` trailer lines to commit messages.

Group related changes into a single commit rather than making one commit per file or per concern. Prefer fewer, broader commits. Keep commit messages short — a concise subject line is usually enough; avoid long body paragraphs.
