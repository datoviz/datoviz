# Release Candidate Process

Use explicit release candidates. Each RC should have a written scope, known issues, validation
matrix, generated artifacts, migration/status notes, and feedback request.


## RC1: API And Architecture Candidate

Exit criteria:

1. RC1 tag and notes exist;
2. build/test/spec validation is recorded;
3. feature table, visible parity table, known gaps, raw `ctypes` scope, and WebGPU/WASM scope are
   published or linked;
4. release examples are documented enough for early testers;
5. public headers, ownership rules, and lower-layer support labels have been reviewed.

Success criteria:

1. a small number of serious external testers, roughly 5-10, try the release candidate;
2. installation feedback arrives from Linux, macOS, and Windows;
3. at least several release examples run outside the main development machine;
4. reports distinguish build/install failures, rendering or driver bugs, example issues, API
   feedback, documentation gaps, and platform/GPU details;
5. public discussion does not confuse Datoviz v0.4 RC1 with VisPy 2.0 or with the final v0.4
   release.


## RC2: Documentation And Gallery Candidate

Exit criteria:

1. documentation and gallery structure are mostly final;
2. generated C reference or complete outline exists;
3. captured artifacts prove the declared feature set;
4. RC1 feedback is triaged;
5. gallery examples include data attribution, source links, licenses, and reproducible capture
   commands where relevant;
6. candidate real-dataset outreach examples and gallery media are reviewed for scientific
   usefulness, not only visual polish.


## RC3: Packaging And Quality Candidate

Exit criteria:

1. only blocker fixes remain;
2. packaging, licenses, generated artifacts, release notes, and docs are final candidates;
3. source archives and wheels build, install, and pass installed smoke tests on supported platforms;
4. static-analysis, memory/UB, Vulkan validation, long-running loop, docs link, gallery smoke, and
   example smoke results are either clean or recorded as known issues;
5. checksums/signing policy and required third-party notices are decided.


## Final v0.4.0

Exit criteria:

1. `v0.4.0` is tagged and published with reproducible artifacts;
2. documentation and release notes are public;
3. launch screenshots, short clips, README/website assets, and announcement text are generated from
   current gallery examples;
4. direct feedback channels are open for early users, especially scientists whose public datasets
   are used in showcase examples;
5. the active queue resets for v0.4 patch work and v0.5 planning.


## Required RC Notes

Every RC note should include:

1. exact commit and tag;
2. feature status table;
3. known issues;
4. platform validation matrix;
5. test/static-analysis summary;
6. docs/gallery build links;
7. wheel/source artifacts;
8. migration/status notes from v0.3 and development snapshots;
9. feedback request targeted at users and contributors.


## Feedback Triage

Each RC should have issue labels or project fields that separate:

1. installation and packaging failures;
2. rendering, driver, and platform-specific bugs;
3. example and gallery failures;
4. API and ownership/lifetime feedback;
5. documentation issues;
6. final-release blockers.

After one to two weeks of RC1 feedback, summarize the findings and decide whether RC2 is a blocker
fix release, a documentation/gallery candidate, or unnecessary before the next planned RC gate.
