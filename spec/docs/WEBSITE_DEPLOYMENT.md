# Website Deployment And Versioning

This document defines how Datoviz documentation moves from source changes to the public website.
It records the post-RC1 target workflow; it does not make documentation publication a release-
candidate blocker by itself.


## Current State After RC1

The RC1 website cutover is complete:

1. `datoviz.org/` serves the v0.4 release-candidate documentation.
2. `datoviz.org/v0.3/` preserves the former v0.3 website.
3. Authored documentation, generated-reference inputs, gallery metadata, and deployment tooling
   live in `datoviz/datoviz`.
4. Generated static website files live in the deployment-only `datoviz/datoviz.github.io`
   repository.
5. GitHub Pages publishes the root of `datoviz.github.io/main` to `datoviz.org`.

A push to `datoviz/main` or `datoviz/v0.4-dev` does not update the public website. The current
publisher must build and validate the site, commit the generated output to
`datoviz.github.io/main`, and push that deployment commit.

The local post-RC1 command for the active release-candidate branch is:

```sh
just docs-publish yes v0.4-dev no
```

It requires clean source and website checkouts, builds and validates the WASM scene module, runs a
strict MkDocs build, composes the v0.4 site with the preserved v0.3 archive, checks the staged
deployment, commits it, and pushes `datoviz.github.io/main`.


## Decisions

1. Validate documentation automatically on every relevant pull request.
2. Keep production publication separate from ordinary source commits.
3. Build a release documentation artifact once, then promote those exact bytes.
4. Keep `datoviz.github.io` deployment-only; do not author documentation there.
5. Preserve versioned documentation independently of the production-root selection.
6. Require an explicit production approval even when every automated gate is green.
7. Treat public media and WASM payloads as first-class deployment artifacts with recorded hashes.


## Branch And Version Policy

Through v0.4.0 final, `v0.4-dev` remains the active release-candidate source branch. After the
controlled final merge:

1. `main` becomes the single long-lived integration branch.
2. feature branches remain short-lived and merge through pull requests;
3. `dev` and `v0.4-dev` are retired or frozen rather than kept as parallel integration branches;
4. a `release/0.4` maintenance branch is created only when v0.4 patch work overlaps with v0.5
   development; and
5. immutable release tags remain the authority for released source and documentation snapshots.


## Documentation Channels

The target layout separates continuously changing documentation from promoted releases:

| Channel | Source | Publication policy |
| --- | --- | --- |
| `/dev/` | latest validated `main` commit | automatic after merge |
| `/v0.4.0/`, `/v0.4.1/`, ... | exact release artifact | immutable |
| `/v0.4/`, `/v0.5/`, ... | selected release in that series | manually advanced alias |
| `/v0.3/` | archived legacy site | preserved |
| `/` | explicitly selected release or RC | manual promotion |

The root may point to an RC when the maintainer explicitly chooses that public posture. Updating
`/dev/` must never update `/`, and publishing a versioned snapshot must not implicitly promote it.


## Target Pipeline

```text
feature branch or pull request
    -> build and validate a preview artifact
    -> merge to main
    -> publish the validated development channel
    -> create an immutable release tag
    -> build one release documentation artifact and manifest
    -> approve production deployment
    -> publish the versioned snapshot and promote the same bytes to the root
```

### Pull Requests

Relevant pull requests must run:

1. strict MkDocs build;
2. authored/generated documentation drift checks;
3. internal-link and stable-route checks;
4. gallery reference and media-presence checks;
5. WASM module export and deployment checks;
6. forbidden-file and maximum-size checks; and
7. preview artifact upload.

Pull-request workflows may publish an isolated preview URL, but they must not modify the production
website repository or custom domain.

### Main

A green merge to `main` may automatically publish `/dev/`. Use concurrency control so a newer
deployment supersedes an older queued deployment and two writers cannot race in the website
repository.

### Release Tags

The release workflow must build documentation from the exact tag and emit one immutable artifact.
The accompanying machine-readable manifest must record at least:

1. source repository and full commit hash;
2. release version and channel;
3. documentation toolchain versions;
4. every deployed file path, byte count, and SHA-256 digest;
5. the preserved-version inputs; and
6. the validation results associated with the artifact.

The production job consumes that artifact. It must not rebuild from the tag after approval, because
a second build could change generated output or dependency resolution.

### Production Promotion

Production publication uses a protected GitHub environment with a required maintainer approval.
The approved job:

1. checks out the current deployment repository head;
2. verifies the expected predecessor and deployment concurrency lock;
3. stages the versioned snapshot without changing older version directories;
4. updates the root from the same approved artifact when root promotion was requested;
5. verifies the exact staged set and artifact manifest;
6. creates an auditable bot-authored deployment commit; and
7. pushes `datoviz.github.io/main`, allowing GitHub Pages to publish it.


## Media And WASM Policy

Canonical screenshots and capture inputs belong in the `data` repository under its explicit
commit policy. Optimized WebP, MP4, and WASM website outputs are deployment artifacts, not authored
documentation sources.

Before publication:

1. every HTML gallery-media reference must resolve within the staged site;
2. video files must decode completely and match declared dimensions, duration, frame rate, and
   size budgets;
3. representative posters and animated captures require visual review for release promotion;
4. WASM JavaScript imports and exports must match the validated module contract;
5. no `.DS_Store`, unexpected generated payload, or file over the repository limit may be staged;
   and
6. public bytes must match the deployment manifest after GitHub Pages completes.

Missing optional source media may be tolerated while authoring only when generated pages fall back
to an existing static asset. Generated HTML must never reference an absent public asset.


## Credentials And Repository Protection

Use a GitHub App or narrowly scoped deploy credential that can write only the website repository.
Do not use a maintainer's broad personal token for unattended publication.

Protect `datoviz.github.io/main` so that:

1. normal human development does not occur directly on the branch;
2. only the reviewed deployment workflow or an explicit emergency-maintainer path can push;
3. force pushes and history deletion are disabled; and
4. the deployment commit, workflow run, source commit, and artifact manifest remain cross-linked.


## Post-RC1 Migration

This infrastructure work is desirable before v0.4.0 final but is not an RC2 feature blocker.

1. Keep the current guarded local publisher as the recovery path.
2. Add a manually dispatched `Publish documentation` workflow with inputs for source ref, channel,
   and root promotion.
3. Move validation and artifact construction into CI while retaining manual production approval.
4. Add `/dev/` publication after `main` becomes the integration branch.
5. Add the deployment manifest and public-byte verification.
6. Protect the website repository and replace maintainer credentials with scoped automation.
7. Retire the local command as the primary path only after the hosted workflow has completed at
   least one rehearsed non-production deployment and one approved production deployment.


## Completion Criteria

The migration is complete when:

1. a pull request produces a validated preview without production write access;
2. a maintainer can select an exact source ref and channel from a hosted manual workflow;
3. production approval promotes an existing immutable artifact rather than rebuilding it;
4. the deployed manifest and public website hashes agree;
5. `/dev/`, versioned snapshots, the v0.3 archive, and the production root can be updated
   independently; and
6. the guarded local publisher remains documented and tested as an emergency fallback.
