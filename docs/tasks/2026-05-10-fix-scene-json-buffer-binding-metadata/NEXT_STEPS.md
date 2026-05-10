# Next steps

- No code follow-up is currently required for this issue.
- If neighboring scene JSON expectations drift again, update them to derive values from the actual descriptor or payload size instead of hardcoding stale constants.

## Risks

- Other JSON assertions in `src/scene/tests/test_scene.c` may still embed hardcoded size values and should be reviewed if the scene buffer ABI changes again.
