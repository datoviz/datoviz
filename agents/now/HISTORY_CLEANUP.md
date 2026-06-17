# Datoviz v0.4 Git History Cleanup

Status: preparation only. Do not rewrite or force-push without explicit final maintainer approval
for the exact refs.

This checklist supports the optional pre-RC repository-size cleanup described in
[RELEASE.md](RELEASE.md#0-pre-rc-git-history-cleanup). The source tree cleanup has already removed
the large runtime/generated payloads from current `v0.4-dev`; shrinking clone size requires a
coordinated Git history rewrite or deletion of every ref that still keeps those objects reachable.


## Dry-Run Tool

Run from the repository root:

```sh
tools/git_history_cleanup_dry_run.sh
```

The script:

1. creates a fresh mirror clone under `/tmp`;
2. runs `git filter-repo` only inside that disposable mirror;
3. removes the agreed heavy paths from the mirror;
4. reports before/after pack size, `HEAD` tree size, ref counts, and refs still containing the
   cleanup paths;
5. never rewrites the working checkout and never pushes.

To choose the temporary base directory:

```sh
DVZ_HISTORY_CLEANUP_TMP=/tmp/datoviz-history-cleanup tools/git_history_cleanup_dry_run.sh
```


## Candidate Cleanup Paths

```text
docs/assets/references/
bin/vulkan/
libs/vulkan/
libs/shaderc/
libs/swiftshader/
v0.3/
external/vulkan/*.hpp
external/vulkan/*.cppm
```


## Maintainer Checklist

Before the rewrite:

1. Confirm no v0.4 RC tag exists yet.
2. Announce a temporary push freeze.
3. Decide the exact ref policy: active refs to rewrite, old refs to delete, and old refs to archive
   in a legacy mirror.
4. Create and verify a legacy backup of current refs.
5. Run `tools/git_history_cleanup_dry_run.sh` and save the output.
6. Inspect the disposable mirror: important branches, important tags, size, and commit graph.
7. Fresh-clone the disposable mirror and run a narrow smoke build if practical.

During the rewrite:

1. Work from a fresh mirror clone, not the everyday checkout.
2. Use the same path list as the dry-run script unless the maintainer explicitly approves changes.
3. Rewrite only the approved refs.
4. Push with `--force-with-lease` only after final approval.
5. Do not rewrite public release refs after `v0.4.0-rc1` except for emergencies.

After the rewrite:

1. Verify GitHub default branch, branch list, and tag list.
2. Make a fresh recursive clone from GitHub.
3. Run the narrow release smoke checks appropriate for the day.
4. Publish the reclone notice before cutting `v0.4.0-rc1`.
5. Keep the legacy backup until after `v0.4.0` final is stable.


## User Migration Notice Draft

Existing clones from before the v0.4 history cleanup should reclone:

```sh
mv datoviz datoviz-pre-v0.4-history-cleanup
git clone --recursive https://github.com/datoviz/datoviz.git
cd datoviz
```

For uncommitted local work:

```sh
git diff > /tmp/datoviz-local.patch
# reclone, then:
git apply /tmp/datoviz-local.patch
```

For committed local work:

```sh
git format-patch origin/main..HEAD -o /tmp/datoviz-patches
# reclone, then:
git am /tmp/datoviz-patches/*.patch
```
