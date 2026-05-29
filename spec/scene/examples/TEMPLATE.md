# Example Title

> **Example status:** informative pressure test
> **Target:** C example | Python example | fixture | API sketch | preprocessing script
> **Data:** inline | synthetic | bundled cache | public download
> **Validation:** smoke | screenshot/readback | fixture | manual checklist
> **Agent copy-safe:** yes | no
> **Role:** minimal | update | offscreen | interaction | technique | showcase | sketch


## Summary

Describe the user-visible result in one short paragraph. Make clear whether this is a runnable
example, an API sketch, a fixture target, or a larger showcase.

If the example is agent copy-safe, state the exact public API layer it demonstrates. Most user-facing
C examples should start from the scene/app path rather than DRP2, vklite, or Vulkan internals.


## Feature Pressure

List the scene, visual, interaction, resource, or runtime behavior this example is meant to test.
Link to canonical specs instead of restating generic policy.

For agent-copyable examples, name the one primary feature or visual that a generated-code assistant
should learn from this file.


## Data And Resources

Describe only the data requirements that are specific to this scenario. For cache, download,
fallback, and preprocessing policy, link to [POLICIES.md](POLICIES.md).


## Scene Shape

Describe panels, cameras, visual families, resources, transforms, scales, labels, and interaction
only when they are essential to the example.


## Runtime Behavior

Describe animation, streaming, interaction, picking, probing, update frequency, or fallback behavior
specific to this example.


## Minimal Target

State the smallest useful implementation. Keep staged plans short; do not include long pseudocode.
Put release stage and priority in [PLANNING.md](PLANNING.md), not in individual scenarios.

For copy-safe examples, this should be a complete source file with visible setup, data binding,
rendering or capture, and cleanup. For sketches and showcases, explain why they are not safe as a
minimal starting point.


## Validation

Define acceptance criteria:

1. smoke command,
2. expected visual result,
3. screenshot/readback or fixture check,
4. performance target when relevant,
5. manual interaction checklist if automated validation is not practical.

Include the command that a coding agent should run after copying or modifying this example.


## Open Questions

List only unresolved questions that block or materially change the example.
