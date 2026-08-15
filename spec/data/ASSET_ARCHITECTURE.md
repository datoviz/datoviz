# Datoviz Binary Asset Architecture

> **Status:** target architecture and migration plan
> **Scope:** runtime resources, test fixtures, example datasets, WebGPU bundles, gallery media, release evidence, caches, and the retirement of the active `data` submodule

## Decision

Datoviz must build, test, package, install, and support ordinary library use without initializing `datoviz/data`, downloading optional assets, or accessing a network. The active architecture will not use Git LFS.

Small stable resources that are required for the library or hermetic tests belong in ordinary Git in `datoviz/datoviz`. Large optional datasets and published gallery snapshots use full cryptographic bundle identities recorded in the parent catalog, with immutable GitHub Release assets in `datoviz/assets` as the initial primary origin. Raw sources, preparation intermediates, downloaded bundles, and generated outputs belong in local caches or their authoritative upstream repositories. The current `datoviz/data` repository becomes a frozen legacy source during migration and is removed as a submodule from the active Datoviz line.

Bundle authority is the parent catalog plus the full SHA-256 of exact archive bytes, not a hosting account or URL. GitHub Actions caches and workflow artifacts may accelerate transfers, but neither is an authoritative store. A cache miss must be recoverable from a digest-pinned origin, an exact-byte mirror, an independent backup, or deterministic regeneration according to the bundle's declared reproducibility class, and core CI must not need any of those paths.

## Why The Existing Boundary Failed

The current `data` submodule mixes five lifecycles: build-critical fonts, generic test fixtures, prepared scientific datasets, generated gallery media, and historical release data. Those classes differ in stability, licensing, publication cadence, consumer audience, reproducibility, and retention needs.

The active checked-out data tree is about 81 MiB: approximately 8 MiB of reusable assets, 47 MiB of prepared example datasets, and 26 MiB of gallery media. The current tree has 161 LFS files totaling about 84 MiB with a largest file of about 24 MiB, while reachable LFS objects across repository refs total about 0.9 GiB. GitHub billing nevertheless reports a much larger historical LFS footprint because replaced and orphaned binary versions remain associated with the repositories. The storage and bandwidth failure is therefore history amplification, not an active payload-size problem.

The source build currently depends on two fonts from the submodule, generic file-I/O tests depend on an Earth texture and an Allen Institute NumPy array, release-source construction reaches into the submodule for ten fonts, WebGPU publishing stages selected data bundles from the submodule, and gallery capture writes generated PNG files back into it. These are ownership defects rather than reasons to preserve the submodule.

## Required Properties

1. A clean source checkout with `data` absent must configure, build, test, and create release artifacts offline.
2. Installed C and Python library use must never download optional data implicitly.
3. Core tests must use deterministic generated data or tiny committed fixtures, not scientific showcase datasets.
4. Every remote bundle must have a constrained stable identifier, full archive SHA-256, one or more immutable origins, compressed and expanded byte limits, per-file hashes, provenance, redistribution basis, producer commit, and independent backup policy.
5. Example data must be fetched explicitly or through an explicitly enabled example convenience path, verified before use, and stored outside the source checkout by default.
6. Native and WebGPU examples must share one semantic bundle contract and must not create backend-specific scientific datasets.
7. Generated gallery media must not accumulate as mutable Git history.
8. Old Datoviz releases and their exact data gitlinks must remain documented and recoverable during a deliberate legacy-retention period.
9. The asset provider must be replaceable by an exact-byte mirror without changing example semantics or cache identity.
10. No active workflow may require Git LFS after migration completion.

## Storage And Ownership Map

| Binary class | Identity or authoritative source | Local/runtime location | Delivery rule |
| --- | --- | --- | --- |
| Built-in scene and GUI fonts | Ordinary Git under `assets/runtime/fonts/` in `datoviz/datoviz` | Embedded defaults or installed runtime resources | Admit one complete reviewed family preserving Regular/Bold/Italic/BoldItalic/Mono roles plus a scientific fallback; do not copy obsolete legacy faces as an intermediate state |
| Future optional font faces | User path or optional published bundle | User-selected path or verified asset cache | Never an undeclared dependency of core text rendering |
| Tiny decoder and format fixtures | Ordinary Git under `testing/fixtures/` | Source/test checkout only | Generated where practical; otherwise intentionally minimal |
| Declarative colormap sources | Ordinary Git under `assets/runtime/colormaps/` | Generated/embedded runtime atlas | Generated atlases are build products unless an exact shipped derivative is justified |
| Tutorial geometry and textures | Source code or deterministic generator | Build/test output | No committed binary prerequisite |
| Prepared scientific example bundles | Full archive digest in the parent catalog; initial primary origin is an immutable release in `datoviz/assets` | Verified user cache | Explicit fetch; no core-library dependency |
| Full-scale benchmarks | Authoritative upstream or full digest with immutable optional origins | Configurable local dataset/cache root | Never downloaded by normal CI or normal examples |
| Reviewed website gallery snapshots | Full archive digest with an immutable release in `datoviz/assets` as primary origin | Local gallery output and generated website tree | Publish only after validation and approval |
| Exact RC/final media evidence | Matching software release in `datoviz/datoviz`, optionally mirrored byte-for-byte | Release evidence download | Keep evidence with the exact software candidate it proves |
| Small selected documentation images | Ordinary Git under `docs/images/` | Documentation source and site | Existing optimization and size policy applies |
| WebGPU-served data | Same immutable bundle as native examples | Versioned generated site tree and browser cache | Site deployment is a derivative, not the authority |
| Wheels, source archives, checksums, SBOMs, and release evidence | Releases in `datoviz/datoviz` | Installer/download cache | Software release lifecycle remains separate from optional assets |
| Raw third-party scientific data | Original upstream source | `.cache/datoviz/sources/` or user-provided path | Never mirrored without recorded redistribution permission |
| Preparation intermediates | None | `.cache/datoviz/work/` | Disposable and reproducible |
| CI transfer state | None | Actions cache or per-run artifact | Acceleration only; bounded retention |
| Stable generated core payloads such as embedded atlases or GUI fonts | Ordinary Git only when explicitly admitted with source hash and regeneration check; otherwise generated for source/release artifacts | Build tree or compiled library | Must be classified individually rather than exempted as generated source |
| Shader binaries and packaged native libraries | Generated source/release artifacts | Build, wheel, or installation tree | Never general asset-catalog content |
| Crash dumps, profiles, traces, and raw validation output | No durable default authority; selected evidence may enter a software release | Local ignored output | May contain sensitive host or user data and requires review before publication |

## Repository Roles

### `datoviz/datoviz`

The main repository owns code, public examples, preparation and validation tools, bundle schemas, catalog descriptors, provenance, licenses, small runtime resources, tiny test fixtures, and the exact asset identifiers selected by each code revision.

The final tree should contain boundaries similar to:

```text
assets/
  runtime/
    fonts/
      SourceSans3-Regular.ttf
      SourceSans3-Bold.ttf
      SourceSans3-It.ttf
      SourceSans3-BoldIt.ttf
      SourceCodePro-Regular.ttf
      NotoSansMath-Regular.ttf
    colormaps/
      colormaps.csv
  catalog/
    examples/
      cortical_activity/
        asset.json
        PROVENANCE.md
      terrain_relief/
        asset.json
        PROVENANCE.md
testing/
  fixtures/
    jpeg/
    npy/
tools/
  assets/
```

The font independence slice admits the complete reviewed Source family and Noto scientific fallback directly while preserving the five primary roles, offline behavior, visible fallback, and custom file-font escape hatch. It does not copy the ten mixed-version legacy files into active parent Git first. Committing exact upstream font files is preferable to committing only generated C arrays because the originals preserve provenance, avoid generated-source churn, and allow release tooling to regenerate exact embedded payloads.

The long-term built-in family, custom-font ownership, coverage policy, generated products, and staged default migration are specified in [FONT_ARCHITECTURE.md](FONT_ARCHITECTURE.md).

### `datoviz/assets`

The new public repository is the initial publication control plane for optional binary assets. Its Git history contains only text policy, schemas if needed, and a small publication ledger linking each asset release to its producer repository and commit. The actual dataset and gallery archives are GitHub Release assets, not Git blobs and not LFS objects. GitHub is the primary origin, while identity remains the full digest recorded by the parent catalog.

The `datoviz-` prefix is intentionally omitted because the organization-qualified name is already `datoviz/assets`.

Immutable releases are publication batches containing independently downloadable content-addressed archives. Unchanged bundles remain referenced from older batches and are not duplicated. Representative release tags and asset names are:

```text
tag:   assets-v0.4.0rc3
asset: cortical-activity-sha256-<full-archive-digest>.tar.gz
asset: terrain-relief-sha256-<full-archive-digest>.tar.gz
```

Gallery publication uses release-level snapshots because the images form one reviewed evidence set:

```text
tag:   gallery-v0.4.0rc3-linux
asset: gallery-v0.4.0rc3-linux-sha256-<full-archive-digest>.tar.gz
```

The provider boundary must remain abstract. Catalog descriptors contain an ordered origin list, but cache keys and example identity depend on the bundle id and full archive digest, not on GitHub. Mirrors must serve exactly the same archive bytes; re-encoding creates a new bundle digest. A future mirror or CDN can therefore be added without changing extracted paths. Before legacy LFS cleanup, every referenced archive also needs an independent cold copy; curated final scientific bundles may additionally be archived through Zenodo or another durable research repository when appropriate.

GitHub's current service limits permit up to 1,000 assets per release, require each asset to remain below 2 GiB, and state no total release-size or bandwidth limit. Publication tooling must preflight the current API rather than assume those limits are permanent, handle both direct and redirected downloads, and keep batches comfortably bounded. Immutable releases protect tags and assets from ordinary mutation and provide attestations, but GitHub is not treated as an archival SLA because repository-level administrative or service events can still affect availability. See [About releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases) and [Immutable releases](https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases).

### `datoviz/data`

The existing repository remains frozen during migration so historical parent commits retain their recorded gitlink destination. It receives no new active v0.4 datasets or gallery output. It is not renamed as part of the parent branch cutover and is not an authority for new development once the catalog path is operational.

Before LFS cleanup, export one verified snapshot archive for every distinct data gitlink referenced by a released parent tag. These snapshots live in a clearly separate legacy namespace and do not become supported active catalog bundles. The legacy inventory records parent tag, data commit, required LFS OIDs and sizes, archive digest, backup location, and reconstruction instructions, and at least one old release must be reconstructed without the live LFS endpoint. If any required payload cannot be recovered, record a blocking gap and do not proceed with cleanup. Audit `data-OLD` separately. Do not delete or recreate either repository until snapshot exports, legal records, historical reproducibility, redirects, and rollback have been reviewed explicitly. This preservation lane may begin immediately but is not a parent branch-cutover prerequisite.

## Bundle And Catalog Contract

Each bundle has one identity: the full SHA-256 of exact deterministic archive bytes. This is also the cache key and the identity pinned by native, WebGPU, documentation, and release workflows. A short prefix may be displayed to humans but is never an integrity or uniqueness boundary.

Archive construction is normative and tested: one normalized root directory, sorted member paths, normalized file modes, zero timestamps and ownership, a fixed archive format and compression configuration, no links or special files, and no undeclared members. Re-encoding or changing provenance produces a new digest and therefore a new bundle revision. This intentionally favors one supportable representation over semantic equivalence between multiple physical encodings.

### Payload manifest

Every archive contains a canonical `bundle.json` plus provenance, licenses, and prepared artifacts. The payload manifest declares:

```json
{
  "schema": "datoviz.asset-bundle.v1",
  "id": "cortical_activity",
  "kind": "example-data",
  "producer": {
    "repository": "datoviz/datoviz",
    "commit": "<full-commit>",
    "command": "python tools/assets/prepare_cortical_activity.py"
  },
  "artifacts": [
    {
      "path": "prepared/cortical_activity.bin",
      "byte_size": 23956769,
      "sha256": "<artifact-sha256>",
      "format": "datoviz-cortical-activity-v1"
    }
  ]
}
```

`bundle.json` lists every other meaningful archive member, including provenance and license records, with path, byte size, SHA-256, and semantic role. The manifest does not contain the archive digest because that would be self-referential; the parent catalog pins the archive. The manifest schema, deterministic packer, and test fixtures define the representation without requiring cross-language canonical JSON hashing.

### Catalog descriptor

The main repository contains a small transport descriptor selected by the current code revision:

```json
{
  "schema": "datoviz.asset-catalog-entry.v1",
  "id": "cortical_activity",
  "archive_sha256": "<full-64-hex-archive-digest>",
  "archive_format": "tar.gz",
  "byte_size": 24001234,
  "expanded_byte_size": 23960000,
  "max_members": 32,
  "origins": [
    {
      "provider": "github-release",
      "url": "https://github.com/datoviz/assets/releases/download/assets-v0.4.0rc3/cortical-activity-sha256-<full-archive-digest>.tar.gz"
    }
  ]
}
```

The downloader accepts HTTPS default origins, follows a bounded redirect policy without forwarding credentials across origins, honors standard proxy and CA configuration, uses bounded timeouts and retries, streams into a private temporary file, and verifies byte size and full digest before extraction. Safe extraction rejects absolute paths, `..`, forward- or backslash ambiguity, drive and UNC paths, alternate data stream syntax, links, special and sparse files, unsupported archive extensions, duplicate or case-folding-colliding paths, Unicode-normalization collisions, reserved platform names, excessive path/member counts, and compressed or expanded size violations. It then verifies the payload manifest and every member before an atomic platform-compatible promotion with a verified completion marker.

Bundle IDs use a constrained portable grammar and are never interpolated as unchecked filesystem paths. Catalog updates are ordinary reviewed source changes. An asset release must already exist and pass unauthenticated remote re-download verification before a catalog entry may reference it. A catalog entry never targets an editable `latest` URL. Mirror overrides map a known full digest to exact bytes; they do not allow an unpinned replacement.

## Local Cache Contract

The default cache follows the platform cache convention through one Datoviz resolver and may be overridden explicitly. Conceptually it contains:

```text
<cache-root>/datoviz/
  bundles/<id>/sha256-<full-archive-digest>/
  downloads/sha256-<full-archive-digest>.partial
  sources/<id>/
  work/<id>/
  gallery/<code-commit>/<platform>/
```

The public tooling should expose at least:

```text
datoviz assets list
datoviz assets fetch <id>
datoviz assets verify <id>
datoviz assets path <id>
datoviz assets purge [<id>]
```

The exact executable or Python-module spelling may be selected during implementation. The resolver contract matters more than the initial command surface. The initial fetcher should be Python/source tooling such as `python -m datoviz.assets`; networking does not belong in `libdatoviz`. Native examples receive an explicit resolved root from a launcher, command-line argument, or environment.

Library APIs do not fetch implicitly. Example launchers may offer an explicit `--fetch-assets` convenience or print the exact fetch command. An explicitly supplied path always wins, followed by the verified cache. Missing required data fails before scene creation with a precise diagnostic. Offline mode must be explicit and deterministic.

The resolver follows platform conventions: `XDG_CACHE_HOME` on Unix, the native macOS caches directory, and `LOCALAPPDATA` on Windows, with explicit cache and shared/HPC asset-root overrides. Concurrent fetches use a lock with defined ownership and stale-lock recovery, private temporary directories, and an atomic platform-compatible final promotion. Interrupted downloads remain non-authoritative partial files. Offline import/export supports air-gapped and cluster use. Cache garbage collection operates only on resolver-owned verified directories and never traverses user-supplied paths.

## CI And Release Behavior

Core CI checks out only source dependencies, builds offline, and uses committed tiny fixtures. It neither initializes `data` nor downloads a release bundle. The documentation contract is split: `docs-check` validates source, links, generated Markdown, API reference, and placeholders hermetically, while `site-release` resolves catalog-pinned datasets and media to construct the deployable site.

Optional example, WebGPU, gallery, site-release, and release-candidate jobs resolve only catalog-pinned bundles. CI may restore a cache keyed by full archive digest, but on a miss it downloads the immutable release asset once in a preparation job and fans out a verified artifact to dependent jobs. Pull-request jobs must not gain write credentials merely to read public bundles.

Downloaded data is treated as untrusted input even when hosted by Datoviz. CI and local tooling enforce the transport and extraction rules above and a non-executable payload policy. Privileged cross-repository publication is owned by `datoviz/assets`, is manual or protected-environment only, has no pull-request-triggered write path, and uses an approved fine-grained credential or maintainer `gh` flow scoped to that repository. It creates a draft batch release, uploads assets and checksums, re-downloads and verifies exact bytes, then publishes once. The parent catalog changes only after public unauthenticated verification, and immutable release policy prevents later tag movement or asset replacement.

Software releases in `datoviz/datoviz` continue to hold source archives, wheels, checksums, SBOMs, and exact-candidate evidence. Optional dataset and gallery releases in `datoviz/assets` use a separate lifecycle and must never be prerequisites for importing or linking Datoviz.

## Native And WebGPU Convergence

The current WebGPU fetched-bundle design becomes the common model rather than a browser exception. Native examples resolve a verified bundle into a real cache path through an example-level asset-root interface, not a library network API. Generated WebGPU descriptors embed the expected full digest selected by the parent catalog. Site preparation downloads and verifies the archive, safely extracts it, then stages loose web files. The browser compares the fetched manifest or staged bundle index with that expected identity before trusting its per-file hashes and mounting files under the canonical virtual root expected by the portable example.

The documentation deployment may unpack selected web-eligible bundles into `site/webgpu-data/<id>/sha256-<full-archive-digest>/` for efficient static serving. That site tree and the browser cache are derivatives. The parent catalog and full archive digest define authority; the immutable release is the initial primary origin.

Backend-specific container formats are permitted only when the runtime requirements genuinely differ and the manifest records their relationship to one semantic source. JavaScript must not reinterpret scientific content merely to compensate for an incoherent bundle boundary.

## Gallery And Documentation Media

Gallery capture writes raw captures, repeats, diffs, and reports to a local ignored output root, not a Git submodule. A reviewed promotion command validates dimensions, hashes, example coverage, platform identity, renderer facts, code commit, and media budgets, then constructs one website gallery snapshot archive and publication manifest for `datoviz/assets`.

Exact screenshots and videos that prove an RC or final candidate belong with the matching `datoviz/datoviz` software release, or are mirrored there byte-for-byte from a gallery snapshot under the same digest. The public site may copy reviewed gallery media during deployment. Small intentionally curated thumbnails, logos, diagrams, and social cards may be committed under `docs/images/` when they satisfy the documentation-image policy; raw captures and broad regenerated sets remain outside Git.

Cross-platform or cross-GPU render differences must not be hidden by one universal golden-image claim. Gallery manifests record the producing platform and validation purpose. Small deterministic image baselines may remain ordinary Git fixtures only when they are genuinely suitable for automated regression and meet the fixture policy.

## Binary Admission Policy

Ordinary Git is not prohibited from containing binary files; it is reserved for binaries whose stability and offline necessity make Git the simplest correct authority.

Default admission rules:

1. Required runtime resources should normally be no larger than 256 KiB per file, must have license and provenance records, and require an explicit allowlist entry.
2. Committed test fixtures should normally be no larger than 64 KiB, should be generated where practical, and must test a format property that cannot be represented more clearly in source.
3. Documentation raster images retain the existing 200 KiB normal limit and optimization policy.
4. Exceptions require an explicit architectural reason, size review, licensing review, and approval of the exact file.
5. Generated binaries, scientific arrays, gallery captures, benchmark payloads, and frequently replaced media do not qualify merely because each individual file is below a threshold.
6. No file extension is automatically classified; ownership and lifecycle determine storage.
7. Stable generated core payloads such as the default MSDF atlas or embedded GUI fonts must declare their source hashes, generator, regeneration command, deterministic-drift check, and whether the generated representation or its source is the release input.
8. Platform libraries, shader binaries, wheels, and other package outputs are release artifacts, never source-tree exceptions.

Repository checks should reject unauthorized large blobs and active-tree LFS pointers, validate runtime/test/generated-resource allowlists, verify reproducible derivatives, and verify that release bundles are absent from the source tree. Existing generated payloads and vendored GUI font data must be inventoried rather than grandfathered implicitly.

## Licensing And Provenance

Publication eligibility is decided per bundle, not per repository. Separate gates cover redistribution of raw sources, redistribution of prepared derivatives, publication of generated media, required attribution and citation, and privacy, human-subject, sensitive-location, export, or other ethical restrictions. A bundle must record upstream source, authors or institution, source version/date, license, redistribution basis, required attribution or citation, preparation command, clean producer commit, exact input hashes, dependency lock or environment, artifact hashes, reproducibility classification, and whether derived gallery media may be published.

If redistribution is not clearly permitted, the catalog contains preparation instructions or a blocker record but no mirrored binary. Credentials, manually accepted terms, private source URLs, personal data, and sensitive host metadata never enter a public descriptor or release. Raw sources remain upstream or in the user's private source cache. A preserved exact prepared bundle may honestly be classified as archived-but-not-byte-reproducible; deterministic regeneration must not be claimed without proof.

## Branch Cutover Relationship

The parent branch cutover and asset migration are related but must not become one irreversible operation.

Before renaming `v0.4-dev` to `main`, land the minimum core-independence slice on the active v0.4 line: admit the complete reviewed Source family and Noto scientific fallback, preserve the five primary font roles and custom-font behavior, replace external generic test inputs, split hermetic documentation checks from full site publication, remove `data` from core build/test/package requirements, and prove a fresh checkout with the submodule uninitialized. This prevents the branch-cutover fresh-clone gate from depending on a blocked LFS service. The exact font admission and resolver are part of that independence slice; optional asset downloading remains outside it.

Then execute the existing audited parent transition without changing data history:

```text
datoviz/datoviz: old main -> v0.3-maintenance
datoviz/datoviz: v0.4-dev -> main
datoviz/data:    unchanged and frozen during migration
```

Catalog implementation, asset publication, gallery migration, and final submodule removal land as reviewed commits on the new `main`. Historical `v0.4-dev` wording and exact release records remain historical facts; live asset documentation uses content identifiers rather than branch names.

## Migration Plan

### Phase 0: freeze and inventory

1. Record the exact parent gitlink, data refs, released parent tags, current-tree file inventory, LFS object inventory, manifest/provenance coverage, licenses, direct source-code consumers, generated gallery consumers, and current WebGPU bundle mapping.
2. Record every distinct data gitlink referenced by a released parent tag and the LFS objects required for later snapshot export, without making complete legacy reconstruction a branch-cutover gate.
3. Classify every current binary as runtime-required, test fixture, stable generated core payload, package artifact, redistributable example bundle, upstream-only dataset, gallery media, release evidence, local diagnostic output, or obsolete/duplicate.
4. Audit current fonts, textures, prepared datasets, and generated media for source/license gaps, privacy or scientific-data restrictions, dirty producer states, and honest reproducibility status.
5. Preserve hashes for the pinned `a9542d2` v0.4 data snapshot so later publication can prove byte identity.

Exit gate: every current path has one declared destination or explicit retirement decision, every released data gitlink is enumerated for the parallel preservation lane, and no file is migrated solely because it exists.

### Phase 1: make core source independent

1. Admit the exact approved Source Sans 3 Regular, Bold, Italic, and Bold Italic faces, Source Code Pro Regular, and Noto Sans Math scientific fallback with license, provenance, coverage, and digest records in `assets/runtime/fonts/`.
2. Replace the temporary legacy admission manifest atomically, preserve Regular, Bold, Italic, Bold Italic, Mono, visible fallback, and custom file-font behavior, and keep the old mixed-version files only in the frozen legacy snapshot rather than active parent Git.
3. Replace the Earth JPEG and Allen NumPy generic file-I/O tests with intentionally tiny fixtures.
4. Generate colormap/runtime derivatives from committed declarative sources where applicable.
5. Define hermetic `docs-check` behavior and move data-backed WebGPU staging and reviewed gallery consumption to `site-release` or equivalent publication validation.
6. Remove data materialization from CMake, source-bundle construction, wheel generation, core CI, and hermetic documentation checks.
7. Add a no-data source/build/test/package/docs-check smoke that leaves the submodule uninitialized.

Exit gate: ordinary source and installed-library use is offline and self-contained; core CI has no Git LFS step or asset download.

### Phase 2: perform the parent branch cutover

1. Refresh and execute the existing audited branch transition after Phase 1 is green.
2. Reconcile live workflow filters, badges, clone instructions, source links, and release guidance.
3. Keep `datoviz/data` untouched and explicitly frozen.

Exit gate: v0.4 is the default `main`, v0.3 is preserved as `v0.3-maintenance`, and fresh main clones pass without data hydration.

### Parallel lane: preserve legacy release snapshots

This lane may start before the branch cutover and continue independently, but it is required only before Phase 6 cleanup or any destructive legacy action.

1. Export one verified snapshot for every distinct released data gitlink, including all required LFS payloads available from local stores, backups, or recoverable origins.
2. Record parent tags, data commits, LFS OIDs and sizes, snapshot digest, independent backup location, reconstruction instructions, and unresolved recovery gaps.
3. Verify at least one old parent release from its snapshot without consulting the live LFS endpoint.
4. Keep legacy snapshots outside the active catalog namespace and do not represent them as supported v0.4 bundles.

Exit gate: every released data gitlink has an independently backed snapshot and reconstruction record, or an explicit blocker prevents Phase 6 cleanup without delaying the parent branch rename.

### Phase 3: establish `datoviz/assets`

1. Create the public repository only after explicit approval of the repository settings, policy files, and initial publication plan.
2. Enable immutable releases, a text publication ledger, and a workflow owned by `datoviz/assets` with manual dispatch, protected review, and a credential scoped only to that repository.
3. Implement one full deterministic archive SHA-256 identity, batch-release publication, catalog origin lists, independent exact-byte backup, safe fetch/verify/cache behavior, and fixtures for corruption, traversal, platform path hazards, interruption, concurrency, mirror changes, air-gapped import, and offline diagnostics.
4. Select a compact pilot only after its raw-source, prepared-derivative, generated-media, attribution, privacy, and reproducibility gates pass.
5. Upload the pilot to a draft batch release, perform an authenticated draft re-download and exact-byte verification, publish immutably, perform an unauthenticated public re-download and verification, then land the parent catalog entry.

Exit gate: a native example and WebGPU smoke consume the same pilot content identity from independent caches, and core Datoviz remains unaffected when the provider is unavailable.

### Phase 4: migrate examples and gallery

1. Publish eligible prepared datasets incrementally, starting with compact bundles already used by native and WebGPU examples.
2. Move manifests, provenance, blockers, and preparation ownership into the parent catalog and update tools from `tools/data/` to the final asset terminology where useful.
3. Convert examples from hard-coded `data/...` paths to one example-level asset-root interface with explicit path plus verified-cache resolution while retaining precise preparation/fetch diagnostics and no networking in `libdatoviz`.
4. Add the full expected digest to generated WebGPU descriptors, verify the archive before site staging, and require the browser to match the parent-selected identity before trusting per-file hashes.
5. Redirect gallery generation to local output, publish reviewed website snapshot releases, attach or mirror exact candidate evidence to matching software releases, and update site-release deployment to consume pinned media.
6. Leave nonredistributable datasets upstream-only and generate synthetic examples only when synthetic data is the declared source.

Exit gate: every active example is procedural, uses a tiny committed resource, resolves a published bundle, or declares an external source; gallery publication writes no source-controlled binary tree.

### Phase 5: remove the active submodule

This is the final architectural target but is not automatically an RC3 or RC4 release gate. It may proceed incrementally during RC4 when stable; otherwise complete it after v0.4 rather than allowing broad path churn to delay the declared release scope.

1. Remove the `data` entry from `.gitmodules`, parent gitlink, CI checkout, release tooling, examples, docs generation, and live specifications.
2. Replace canonical runtime paths with resolver-owned bundle roots while allowing explicit paths for advanced users and reproducibility.
3. Delete the temporary LFS cache workflow and materialization helper after all consumers are gone.
4. Validate source archives, wheels, CMake consumers, Python imports, native examples, WebGPU routes, gallery builds, and fresh clones with no `data` directory.

Exit gate: no active code, test, package, documentation build, or workflow references the submodule as an authority.

### Phase 6: legacy retention and quota cleanup

1. Freeze and archive `datoviz/data`; audit `data-OLD` independently, noting that archival alone does not reduce LFS billing.
2. Publish the parallel-lane legacy inventory mapping parent release tags, data commits, LFS OIDs, verified snapshot archives, digests, and reconstruction instructions.
3. Re-verify old-release reconstruction from independent snapshots before any cleanup request.
4. Ask GitHub Support whether unreachable/orphaned LFS objects can be purged while preserving reachable historical release objects.
5. Consider deletion or recreation only after backups, release reconstruction proof, redirect impact, fork impact, and explicit destructive-action approval.

Exit gate: active development has no dependency on legacy availability, and any cleanup action has a tested recovery path.

## Rollback Boundaries

Phase 1 is reversible through ordinary parent commits and does not modify data history. The branch cutover retains its existing no-force-push rollback policy. Asset publication is append-only: a bad draft is discarded, while a bad immutable release is never referenced and receives a superseding release rather than mutation. Example migrations are bundle-by-bundle and may temporarily retain explicit legacy-path fallback until their published replacement passes native and WebGPU validation. Submodule removal occurs only after all active consumers are independently green.

## Decisions And Preferences

The target decisions are:

1. Use `datoviz/assets`, not `datoviz/datoviz-assets`.
2. Keep regular Datoviz library use completely independent of optional assets.
3. Preserve the five primary font roles, scientific fallback, and custom-font behavior during core extraction; admit the reviewed Source/Noto family directly rather than preserving every historical font file.
4. Use ordinary Git for small stable required resources and tiny test fixtures.
5. Make the parent catalog plus full deterministic archive SHA-256 authoritative; use immutable GitHub Release assets as the initial primary origin with exact-byte mirror and backup support.
6. Batch independently content-addressed dataset archives into immutable publication releases; use one release per reviewed gallery snapshot.
7. Keep catalog descriptors and provenance in the parent repository while including a manifest that hashes every other meaningful member inside each archive.
8. Do not use Git LFS in the active final architecture.
9. Keep networking out of `libdatoviz`; fetch through source/Python tooling and pass native examples an explicit resolved asset root.
10. Complete only narrow core independence before the parent branch cutover; migrate bundles and gallery afterward without making full submodule removal an automatic RC3/RC4 gate.

Implementation details still requiring focused design are the exact catalog schema location, deterministic archive format and compression parameters, CLI spelling, example asset-root interface, platform cache resolver, mirror and backup operations, generated-core-resource allowlist, long-term font contract, and retention duration for the frozen legacy repository. None changes the ownership boundaries above.

## Completion Criteria

The migration is complete when a fresh default-branch checkout with no `data` directory builds and passes core tests and hermetic documentation checks; source archives and wheels are self-contained; regular C and Python use performs no asset network access; every optional binary has one declared upstream or full digest with verified origin and backup; native and WebGPU consumers share parent-pinned bundle identities; gallery media is published without mutable Git history and exact candidate evidence remains with software releases; CI caches are optional accelerators; no active workflow invokes Git LFS; legacy tagged releases have independently verified data snapshots; and `datoviz/data` is absent from the active repository graph and documented only as legacy history.
