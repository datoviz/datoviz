# Claude Code instructions for datoviz

## Next steps (read this first)

**`docs/architecture/next_scene_examples.md`** — next scene+app examples: `hello_scatter.c`
is done; `hello_triangle.c` and `hello_texture.c` each need a new visual constructor first.

For C/C++ distribution (pip wheel, conda, brew, apt, MSVC, `datoviz-config`, `DVZ_VENDORED_DEPS`,
`extern "C"` guard fixes), see `agents/now/C_DISTRIBUTION.md` — items 1–4 in the implementation
order are unblocked and ready to pick up.

## Documentation examples

In documentation code examples, comments are encouraged to orient readers. Place comments on their own line above the code they describe — not inline to the right of a line of code.

## Git commits

Do not add `Co-Authored-By:` trailer lines to commit messages.

Group related changes into a single commit rather than making one commit per file or per concern. Prefer fewer, broader commits. Keep commit messages short — a concise subject line is usually enough; avoid long body paragraphs.
