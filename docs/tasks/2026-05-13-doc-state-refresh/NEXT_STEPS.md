# Next Steps

Recommended follow-up after this documentation refresh:

1. Re-run `just test scene` and `just test app` before making stronger validation claims in
   `agents/now/NEXT_STEPS.md`.
2. Add one manual-interactive task record for the next GLFW smoke pass instead of mixing it into
   general docs.
3. If WebGPU feasibility starts next, create a dedicated task directory before editing runtime code so
   browser-contract findings stay separate from native scene work.
4. If a safety/static-analysis pass starts next, scope it to `src/scene`, `src/drp2`, and `src/app`
   files touched by recent commits before expanding to the whole tree.
