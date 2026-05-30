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
