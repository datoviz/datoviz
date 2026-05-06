# Next steps

- If broader confidence is desired, run adjacent DRP2/canvas integration loops or a broader repository test sweep.
- Consider adding a future direct runtime-level regression that inspects multiple `LOAD` render passes on borrowed canvas targets if this area evolves further.

## Resume commands

- `cmake --build build --target dvztest`
- `direnv exec . just test scene`
- `direnv exec . ./build/testing/dvztest_scene test_scene_multiple_panels_multiple_point_visuals_emit`
- `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_two_panel_points_light_both_halves`

## Risks / notes

- Vulkan validation still reports existing small dedicated-allocation warnings during scene tests, but the suite passes and no validation errors remain for this fix.
- Repo still has unrelated untracked entries outside this task's write set.
