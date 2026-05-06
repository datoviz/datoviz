# Next steps

- Investigate the unrelated scene-suite failure seen in `test_app_offscreen_two_panel_points_light_both_halves` if broader suite green status is needed.
- If desired, run the full scene suite again after that unrelated failure is addressed.

## Resume commands

- `cmake --build build --target dvztest_scene`
- `direnv exec . ./build/testing/dvztest_scene test_scene_rejects_visual_destroy_while_emitted_stream_is_live`
- `direnv exec . just test scene`

## Risks / notes

- `just test scene` currently is not a clean signal for this change because of the pre-existing unrelated offscreen-app failure noted above.
- Vulkan validation warnings about tiny dedicated allocations were present during the broader scene run but did not affect the focused regression result.
