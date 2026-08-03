# Retained Visual Item Ranges

Status: promoted implemented v0.4 slice. Updated: 2026-08-03.

Datoviz exposes retained contiguous item ranges through `DvzItemRange`, `dvz_visual_set_item_range()`, `dvz_visual_clear_item_range()`, and `dvz_visual_get_item_range()`. The range is visual draw state, does not rewrite attribute data, supports empty and restored-full ranges, validates against the family logical item count, lowers point offsets/counts through native GLSL and WebGPU/WGSL paths, and preserves global item identity in point queries.

Higher layers own semantic time, event, particle, track, and filtering models. Datoviz receives only `[first_item, first_item + item_count)` and does not infer domain meaning. Arbitrary predicates, GPU compaction, indirect drawing, and broader family-specific range behavior remain separate future work.

Current authority is the installed scene API, `src/scene/visuals/attr_data.c`, family draw descriptors, FramePlan metadata, and the focused scene/query tests. The former proposal checklist remains in Git history.
