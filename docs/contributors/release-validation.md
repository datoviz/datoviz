# Release Validation

Use this page as the maintainer index for v0.4 release-candidate checks. Durable policy lives in
`spec/release/`; this page links the runnable contributor procedures.


## Core Checks

Run the smallest relevant loop while iterating, then record the broader checks used for each RC:

```sh
git diff --check
just build
just test
just spec-check
```


## Packaging

Wheel build, inspection, installed smoke tests, platform matrices, and the non-live GitHub Actions
draft are documented in [Release wheels](release-wheels.md).


## Release Records

Each RC note should include:

1. exact commit and tag;
2. feature/status table;
3. known issues;
4. platform validation matrix;
5. test and static-analysis summary;
6. docs and gallery build links;
7. wheel and source artifacts;
8. migration/status notes from v0.3 and development snapshots;
9. targeted feedback request.
