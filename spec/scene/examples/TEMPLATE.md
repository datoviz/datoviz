# Example Title

> **Example status:** informative pressure test
> **Target:** C example | Python example | fixture | API sketch | preprocessing script
> **Data:** inline | synthetic | bundled cache | public download
> **Validation:** smoke | screenshot/readback | fixture | manual checklist


## Summary

Describe the user-visible result in one short paragraph. Make clear whether this is a runnable
example, an API sketch, a fixture target, or a larger showcase.


## Feature Pressure

List the scene, visual, interaction, resource, or runtime behavior this example is meant to test.
Link to canonical specs instead of restating generic policy.


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


## Validation

Define acceptance criteria:

1. smoke command,
2. expected visual result,
3. screenshot/readback or fixture check,
4. performance target when relevant,
5. manual interaction checklist if automated validation is not practical.


## Open Questions

List only unresolved questions that block or materially change the example.
