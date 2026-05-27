# Shared Example Policies

Use this file for policy that applies to many worked examples. Individual example specs should link
here instead of repeating these rules.


## Status

Informative policy for `spec/scene/examples/`. It does not override canonical scene, DRP2, or API
specs.


## API Caveat

Example specs may use provisional names to describe the intended shape of an implementation. Treat
those names as pressure tests unless the installed headers already define them.


## Data And Cache Policy

- Prefer inline or deterministic synthetic data for small fixtures.
- Use public downloads only when the real dataset is essential to the pressure test.
- Every downloaded dataset needs a source URL, license/citation note, expected files, size budget,
  cache location, and deterministic fallback or explicit "no fallback" statement.
- Runtime examples should not require network access after the cache is populated.
- Expensive preprocessing belongs in a script or documented bundle build step, not in the runtime
  example.


## FramePlan And DRP2 Notes

Worked examples should name the scene pressure they exercise, but they should not restate the
generic scene -> FramePlan -> DRP2 contract. Link to:

1. `../pipeline/FRAME_PLAN.md` for producer-side frame structure,
2. `../pipeline/RESOURCE_MODEL.md` for logical resources,
3. `../pipeline/INVALIDATION_AND_CACHING.md` for dirty update policy,
4. `../../drp2/` for protocol and runtime details.


## Agent Pickup

Do not copy a large "Agent Pickup" block into every example. A compact metadata block is enough:

```markdown
> **Example status:** informative pressure test
> **Target:** C example | Python example | fixture | API sketch | preprocessing script
> **Data:** inline | synthetic | bundled cache | public download
> **Validation:** smoke | screenshot/readback | fixture | manual checklist
```
