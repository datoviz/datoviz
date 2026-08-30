# Release Flight Checklist

This checklist is for the maintainer cutting a v0.4 release candidate or final release. Work from
top to bottom and record skipped items as known exclusions.


## 1. Repository Hygiene

- [ ] Confirm the branch and commit.
- [ ] Run `git status --short`.
- [ ] Confirm no unintended staged changes.
- [ ] Confirm no staged `data` submodule update unless explicitly approved.
- [ ] Confirm no generated/runtime binary payloads are staged unintentionally.
- [ ] Run `git diff --check`.
- [ ] Run `just release-plan <version>`.
- [ ] Run `just release-dry-run <version> --wheel <wheel>` if a candidate wheel is available.
- [ ] Run `just release-candidate <version> --dry-run`.


## 2. Version And Release Notes

- [ ] Set the Python package version to the intended PEP 440 version.
- [ ] Confirm C runtime version and public notes agree with the release identity.
- [ ] Confirm the intended tag name, for example `v0.4.0rc2`.
- [ ] Draft RC notes with commit, tag, feature status, known issues, and validation matrix.
- [ ] Create or update the candidate-specific announcement; do not retag or redate an earlier RC post.
- [ ] Confirm migration/status notes from v0.3 are honest about breaking changes.
- [ ] Confirm `CITATION.cff` matches the intended release version.
- [ ] Confirm [Citation](../reference/citation.md) has the correct DOI status for this release.
- [ ] Confirm JOSS draft/submission status is recorded without treating JOSS acceptance as a
      software-release blocker.


## 3. Native Build And Tests

- [ ] Run `just release-candidate <version>` or record which lower-level candidate steps replaced
      it.
- [ ] Run `just release-notes <version>` and review the generated draft.
- [ ] Run `just release-docs-validate <version>`.
- [ ] Run `just release-report <version>`.
- [ ] Run `just build`.
- [ ] Run the focused release tests for touched areas.
- [ ] Run `just test` or record why a narrower loop is being used.
- [ ] Run `just spec-check`.
- [ ] Run Vulkan validation or graphics smoke checks for graphics/runtime changes.
- [ ] Record any local native build options that affect artifacts.


## 4. Examples, Docs, And Gallery

- [ ] Build the documentation site once on the designated documentation host at the exact release
      commit, including WebGPU/WASM assets; do not require this build on every physical validation
      machine.
- [ ] Record the documentation-build host and commit in release evidence.
- [ ] On other source-validation hosts, run the applicable lightweight documentation checks: API
      docs, doctests, generated-document drift, public status facts, and example manifests.
- [ ] Confirm `release-docs-validate` passed or record the accepted docs validation exclusion.
- [ ] Run docs link checks if available.
- [ ] Verify feature/status and known-issues pages.
- [ ] Verify release examples compile or run as appropriate.
- [ ] Select the RC canonical examples and confirm each has a source link, status metadata, and
      reproducible run or capture command.
- [ ] Regenerate gallery media that changed.
- [ ] Confirm gallery data attribution and licenses.
- [ ] Confirm public docs do not make stale v0.3 promises.


## 5. Wheel Build

- [ ] Run `just wheel-matrix`.
- [ ] Run `just wheel-ci-local <host-platform-tag>`.
- [ ] If native rebuild is required, run `just wheel-ci-local <host-platform-tag> 1`.
- [ ] For Linux `x86_64` RC evidence, run `just wheel-manylinux-docker x86_64`; do not treat a
      native Ubuntu wheel as manylinux release proof.
- [ ] Validate local artifact tags with `just wheel-validate --platform-tag <host-platform-tag>`.
- [ ] Validate a complete wheelhouse with `just wheel-validate`.
- [ ] Inspect wheel contents with `just wheel-inspect`.
- [ ] Inspect native dependencies with `just wheel-inspect --native-deps`.
- [ ] Confirm wheel filename version and platform tag match the intended artifact.
- [ ] Confirm stale wheels are not present in `dist/`.


## 6. Wheel Install Smokes

- [ ] Run `just release-validation-pack <version> --wheel <wheel>`.
- [ ] Run `just release-machine-plan <version> --wheel <wheel>`.
- [ ] Copy the generated validation pack to each required physical machine.
- [ ] On each required physical machine, run
      `./validate-rc.sh` or `./validate.ps1 -Profile rc` from the extracted pack.
- [ ] Run the shared [physical release-validation procedure](release-physical-validation.md)
      on each available required machine: exact-wheel Quickstart plus the fixed maintainer-guided
      interaction set. Record every pass, fail, and explicit skip.
- [ ] Record artifact and release commits. If they differ, confirm ancestry and audit every
      intervening path as artifact/runtime-neutral; otherwise regenerate wheels and repeat affected
      evidence.
- [ ] On at least one graphics-capable host, run
      `./validate-full.sh` or `./validate.ps1 -Profile full` from the extracted pack, or record why
      render smoke is covered by lower-level checks.
- [ ] Ingest returned evidence with `just release-ingest-evidence <version> <evidence-dir-or-tar>`.
- [ ] Re-run `just release-machine-plan <version>` and confirm required machine actions are clear.
- [ ] Run `just release-report <version> --strict-matrix`.
- [ ] Run `just release-gates <version> --write-artifacts --strict-matrix`.
- [ ] Run `just wheel-check --cmake-consumer --qt-probe optional`.
- [ ] Run render smoke with `--render` on at least one graphics-capable host.
- [ ] Confirm `release-machine-validate --profile rc` ran installed Python/C example smokes.
- [ ] Confirm `release-machine-validate --profile full` ran installed offscreen render examples on
      at least one graphics-capable host, or record why render coverage came from another gate.
- [ ] Review machine evidence `warnings`, command `diagnostics`, and `failures.md`.
- [ ] Confirm `import datoviz` works from an installed wheel.
- [ ] Confirm `import datoviz.raw` works from an installed wheel.
- [ ] Confirm `python -m datoviz.cli --cflags --libs --cmake-dir` behavior.
- [ ] Confirm the CMake consumer smoke builds and runs.
- [ ] Run `just vulkan-course-wheel-smoke <version>`: it installs the published wheel into a
      throwaway venv and builds the Vulkan course step programs against it, exactly as the course's
      chapter 1 instructs. `datoviz==0.4.0rc2` fails this check because it predates the
      tutorial-enabling API. When it first passes, remove the version warning from
      `docs/gpu-graphics/01-setup.md` and make the package path the primary one again.


## 7. Qt And PyQt

- [ ] Confirm base wheel install works without PyQt.
- [ ] Confirm `python -m datoviz.qt` gives a clear diagnostic when PyQt or Qt Vulkan support is
      absent.
- [ ] On a Qt-capable source host, run
      `DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build`.
- [ ] On the same host, run `python -m datoviz.qt` with `DATOVIZ_QTBRIDGE_LIBRARY` pointing at the
      built `datoviz_qtbridge` provider.
- [ ] On the same host, run `python examples/python/qt/hosted_pyqt.py --smoke-ms 1000` with the
      same `DATOVIZ_QTBRIDGE_LIBRARY`.
- [ ] On a Qt-capable host, run `just wheel-check --cmake-consumer --qt-probe required`.
- [ ] Confirm the optional Qt bridge provider is packaged only when intended.
- [ ] Record platform-specific Qt/PyQt limitations.


## 8. CI And Cross-Platform Artifacts

- [ ] Keep draft workflows outside `.github/workflows/` until explicitly enabling them.
- [ ] Run or inspect the non-live wheel workflow draft.
- [ ] Confirm Linux `x86_64` with the manylinux Docker route or equivalent manylinux CI builder.
- [ ] Confirm Linux `aarch64` wheel artifacts only after the `x86_64` Docker route is repeatably
      stable; treat cross-arch builds as inventory coverage unless a native runner is available.
- [ ] Confirm macOS `x86_64` and `arm64` wheel artifacts.
- [ ] Confirm Windows `AMD64` and `ARM64` wheel artifacts.
- [ ] Confirm host-native install smokes for Python 3.10 through 3.14, except Windows ARM64 where the current matrix covers Python 3.11 through 3.14.
- [ ] Keep Python 3.15 prerelease smoke non-blocking unless release policy changes.


## 9. TestPyPI Rehearsal

- [ ] Install and configure `twine` for TestPyPI.
- [ ] Run `just release-testpypi <version> --dry-run --dist-dir dist`.
- [ ] Run `just testpypi-check <host-platform-tag>` for a local single-platform wheel.
- [ ] For a full wheelhouse, run `just testpypi-check-all wheelhouse`.
- [ ] Upload candidate artifacts to TestPyPI through a manual workflow or local maintainer command.
- [ ] After maintainer approval, run
      `just release-testpypi <version> --dist-dir dist --confirm yes`.
- [ ] Use `just testpypi-upload <host-platform-tag> dist yes` for local single-wheel rehearsal.
- [ ] Use `just testpypi-upload-all wheelhouse yes` for full-wheelhouse rehearsal.
- [ ] Install from TestPyPI in a clean environment.
- [ ] Dispatch `package-index-verification.yml` for the exact version and canonical Wheels run.
- [ ] Confirm all six indexed wheel filenames and SHA-256 values match the canonical artifacts.
- [ ] Inspect the consolidated package-index HTML report and confirm all lightweight install smokes
      passed.
- [ ] Confirm clean-index import, raw binding, version, and CLI metadata checks passed; rely on the
      byte-identical canonical-wheel campaign for CMake, Qt, rendering, and example conformance.
- [ ] Confirm dependency metadata and optional extras.
- [ ] Record TestPyPI artifact URLs.


## 10. Publication

- [ ] Re-run `git status --short`.
- [ ] Re-run `git diff --check`.
- [ ] Run `just release-dry-run <version> --wheel <wheel> --dist-dir dist --write-report`.
- [ ] Run `just release-create-tag <version> --dry-run`.
- [ ] After maintainer approval, run `just release-create-tag <version> --confirm yes`.
- [ ] Upload artifacts to PyPI only after TestPyPI rehearsal is accepted.
- [ ] Run `just release-github-draft <version> --dry-run`.
- [ ] Run `just release-pypi <version> --dry-run`.
- [ ] After maintainer approval, run `just release-pypi <version> --confirm yes`.
- [ ] Create or update the GitHub release.
- [ ] Run `just release-github-publish <version> --dry-run`.
- [ ] After maintainer approval, run `just release-github-publish <version> --confirm yes`.
- [ ] Attach source archive, wheels, checksums, and release notes.
- [ ] Run `just release-docs-publish <version> --dry-run`.
- [ ] Publish documentation with an explicit docs deployment command.
- [ ] Confirm GitHub issue forms, labels, and milestone are ready for RC feedback triage.
- [ ] Announce known issues and feedback channels.
- [ ] For final `v0.4.0`, confirm GitHub-Zenodo archiving created a version DOI and concept DOI.
- [ ] For final `v0.4.0`, update `CITATION.cff`, [Citation](../reference/citation.md), and
      release notes with the exact Zenodo DOI and release date.


## 11. Post-Release

- [ ] Verify public artifacts are downloadable.
- [ ] Install the published wheel in a fresh environment.
- [ ] Verify documentation and release links.
- [ ] Update `agents/now/STATUS.md`.
- [ ] Open issues for deferred or newly reported blockers.
- [ ] Start the next RC or post-release patch queue.
